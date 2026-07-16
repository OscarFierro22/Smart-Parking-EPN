from __future__ import annotations

import asyncio
import json
import os
import socket
import time
import uuid
from contextlib import asynccontextmanager, suppress
from pathlib import Path
from typing import Any

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SYNC_DIR = Path(
    os.environ.get(
        "SMART_PARKING_SYNC_DIR",
        PROJECT_ROOT / "OpenGL" / "sync",
    )
).resolve()
STATE_PATH = SYNC_DIR / "parking_state.json"
COMMAND_PATH = SYNC_DIR / "parking_command.json"
API_PORT = 8000
DISCOVERY_PORT = 40404
DISCOVERY_REQUEST = "SMART_PARKING_DISCOVER_V1"




class DiscoveryProtocol(asyncio.DatagramProtocol):
    """Responde a la app Flutter para que encuentre la PC en la red local."""

    def __init__(self) -> None:
        self.transport: asyncio.DatagramTransport | None = None

    def connection_made(self, transport: asyncio.BaseTransport) -> None:
        self.transport = transport  # type: ignore[assignment]

    def datagram_received(self, data: bytes, address: tuple[str, int]) -> None:
        try:
            message = data.decode("utf-8").strip()
        except UnicodeDecodeError:
            return

        if message != DISCOVERY_REQUEST or self.transport is None:
            return

        payload = {
            "service": "smart-parking",
            "name": socket.gethostname(),
            "port": API_PORT,
            "version": 6,
        }
        self.transport.sendto(
            json.dumps(payload, ensure_ascii=False).encode("utf-8"),
            address,
        )


class ConnectionManager:
    def __init__(self) -> None:
        self._clients: set[WebSocket] = set()
        self._lock = asyncio.Lock()

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        async with self._lock:
            self._clients.add(websocket)

    async def disconnect(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._clients.discard(websocket)

    async def broadcast_json(self, payload: dict[str, Any]) -> None:
        message = json.dumps(payload, ensure_ascii=False)
        async with self._lock:
            clients = list(self._clients)

        disconnected: list[WebSocket] = []
        for websocket in clients:
            try:
                await websocket.send_text(message)
            except Exception:
                disconnected.append(websocket)

        for websocket in disconnected:
            await self.disconnect(websocket)


manager = ConnectionManager()
watch_task: asyncio.Task[None] | None = None
discovery_transport: asyncio.DatagramTransport | None = None


def read_state() -> dict[str, Any]:
    if not STATE_PATH.exists():
        return {
            "version": 6,
            "total_slots": 169,
            "available": 161,
            "occupied": 0,
            "reserved": 0,
            "disabled": 8,
            "guidance_active": False,
            "guidance_type": "NONE",
            "route_target_id": "",
            "reserved_slot_id": "",
            "selected_slot_id": "",
            "last_command_id": "",
            "vehicle": {
                "x": 0.0, "y": 0.0, "z": 0.0, "floor": -1,
                "floor_name": "Calle", "heading_degrees": 0.0,
                "speed_kmh": 0.0, "parked_slot_id": ""
            },
            "navigation": {
                "active": False, "type": "NONE", "destination": "",
                "destination_floor": 0, "distance_remaining": 0.0,
                "instruction": "Sin ruta activa", "route": []
            },
            "floors": [],
            "slots": [],
            "backend_waiting_for_opengl": True,
        }

    last_error: Exception | None = None
    for _ in range(4):
        try:
            with STATE_PATH.open("r", encoding="utf-8") as file:
                data = json.load(file)
            data["backend_waiting_for_opengl"] = False
            return data
        except (OSError, json.JSONDecodeError) as error:
            last_error = error
            time.sleep(0.04)

    raise RuntimeError(f"No se pudo leer {STATE_PATH}: {last_error}")


def find_slot(state: dict[str, Any], slot_id: str) -> dict[str, Any] | None:
    normalized = slot_id.strip().upper()
    for slot in state.get("slots", []):
        if str(slot.get("id", "")).upper() == normalized:
            return slot
    return None


def write_command(action: str, slot_id: str = "") -> dict[str, str]:
    SYNC_DIR.mkdir(parents=True, exist_ok=True)
    command = {
        "command_id": str(uuid.uuid4()),
        "action": action,
        "slot_id": slot_id.strip().upper(),
        "created_at_unix": str(time.time()),
    }

    temporary_path = COMMAND_PATH.with_suffix(".tmp")
    with temporary_path.open("w", encoding="utf-8") as file:
        json.dump(command, file, ensure_ascii=False, indent=2)
        file.flush()
        os.fsync(file.fileno())
    os.replace(temporary_path, COMMAND_PATH)
    return command


async def watch_state_file() -> None:
    previous_serialized = ""
    while True:
        try:
            state = await asyncio.to_thread(read_state)
            serialized = json.dumps(state, sort_keys=True, ensure_ascii=False)
            if serialized != previous_serialized:
                previous_serialized = serialized
                await manager.broadcast_json(state)
        except Exception as error:
            await manager.broadcast_json(
                {
                    "type": "backend_error",
                    "message": str(error),
                }
            )
        await asyncio.sleep(0.20)


@asynccontextmanager
async def lifespan(_: FastAPI):
    global watch_task, discovery_transport
    SYNC_DIR.mkdir(parents=True, exist_ok=True)
    watch_task = asyncio.create_task(watch_state_file())

    loop = asyncio.get_running_loop()
    try:
        transport, _ = await loop.create_datagram_endpoint(
            DiscoveryProtocol,
            local_addr=("0.0.0.0", DISCOVERY_PORT),
            allow_broadcast=True,
        )
        discovery_transport = transport
    except OSError as error:
        # La API sigue funcionando aunque el puerto de descubrimiento esté ocupado.
        print(f"ADVERTENCIA: descubrimiento Wi-Fi no disponible: {error}")

    try:
        yield
    finally:
        if discovery_transport is not None:
            discovery_transport.close()
            discovery_transport = None
        if watch_task is not None:
            watch_task.cancel()
            with suppress(asyncio.CancelledError):
                await watch_task


app = FastAPI(
    title="Smart Parking API",
    version="6.0.0",
    lifespan=lifespan,
)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/api/health")
def health() -> dict[str, Any]:
    return {
        "ok": True,
        "sync_directory": str(SYNC_DIR),
        "state_file_exists": STATE_PATH.exists(),
        "api_port": API_PORT,
        "discovery_port": DISCOVERY_PORT,
        "computer_name": socket.gethostname(),
    }


@app.get("/api/state")
def get_state() -> dict[str, Any]:
    try:
        return read_state()
    except RuntimeError as error:
        raise HTTPException(status_code=503, detail=str(error)) from error


@app.post("/api/reserve/{slot_id}")
def reserve_slot(slot_id: str) -> dict[str, Any]:
    state = read_state()
    slot = find_slot(state, slot_id)
    if slot is None:
        raise HTTPException(status_code=404, detail="El espacio no existe")
    if slot.get("state") != "AVAILABLE":
        raise HTTPException(status_code=409, detail="El espacio ya no está disponible")

    command = write_command("RESERVE", slot_id)
    return {"accepted": True, "command": command}


@app.post("/api/reserve-best")
def reserve_best() -> dict[str, Any]:
    state = read_state()
    if int(state.get("available", 0)) <= 0:
        raise HTTPException(status_code=409, detail="No existen espacios disponibles")

    command = write_command("RESERVE_BEST")
    return {"accepted": True, "command": command}


@app.post("/api/cancel-reservation")
def cancel_reservation() -> dict[str, Any]:
    command = write_command("CANCEL_RESERVATION")
    return {"accepted": True, "command": command}


@app.post("/api/simulate-random")
def simulate_random() -> dict[str, Any]:
    command = write_command("SIMULATE_RANDOM")
    return {"accepted": True, "command": command}


@app.post("/api/route-exit")
def route_exit() -> dict[str, Any]:
    command = write_command("ROUTE_EXIT")
    return {"accepted": True, "command": command}


@app.post("/api/release/{slot_id}")
def release_slot(slot_id: str) -> dict[str, Any]:
    state = read_state()
    slot = find_slot(state, slot_id)
    if slot is None:
        raise HTTPException(status_code=404, detail="El espacio no existe")
    if slot.get("state") == "DISABLED":
        raise HTTPException(status_code=409, detail="El espacio accesible no se libera desde la app")

    command = write_command("RELEASE", slot_id)
    return {"accepted": True, "command": command}


@app.post("/api/occupy/{slot_id}")
def occupy_slot(slot_id: str) -> dict[str, Any]:
    state = read_state()
    slot = find_slot(state, slot_id)
    if slot is None:
        raise HTTPException(status_code=404, detail="El espacio no existe")
    if slot.get("state") == "DISABLED":
        raise HTTPException(status_code=409, detail="El espacio accesible no se ocupa desde este control")

    command = write_command("OCCUPY", slot_id)
    return {"accepted": True, "command": command}


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket) -> None:
    await manager.connect(websocket)
    try:
        await websocket.send_json(read_state())
        while True:
            # La app puede enviar "ping". Las actualizaciones reales las emite
            # watch_state_file cuando OpenGL modifica parking_state.json.
            message = await websocket.receive_text()
            if message.strip().lower() == "ping":
                await websocket.send_text('{"type":"pong"}')
    except WebSocketDisconnect:
        pass
    finally:
        await manager.disconnect(websocket)
