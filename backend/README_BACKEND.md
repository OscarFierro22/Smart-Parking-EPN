# Backend Smart Parking — Proyecto final

Conecta OpenGL con Flutter por la red Wi-Fi local.

## Puertos

- TCP 8000: API HTTP y WebSocket.
- UDP 40404: descubrimiento automático.

## Inicio

1. Ejecuta `CONFIGURAR_WIFI_ADMIN.bat` una vez como administrador.
2. Ejecuta `INICIAR_SERVIDOR.bat`.
3. Comprueba `http://127.0.0.1:8000/api/health`.

## Endpoints

- `GET /api/health`
- `GET /api/state`
- `POST /api/reserve/{slot_id}`
- `POST /api/reserve-best`
- `POST /api/simulate-random`
- `POST /api/route-exit`
- `POST /api/cancel-reservation`
- `POST /api/release/{slot_id}`
- `POST /api/occupy/{slot_id}`
- `WS /ws`
