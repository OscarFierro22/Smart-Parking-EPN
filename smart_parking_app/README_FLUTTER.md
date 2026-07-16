# Smart Parking Flutter — Fase 4

## Funciones

- Estado en tiempo real de 169 espacios físicos mediante WebSocket.
- Cuatro pestañas: PB, Piso 1, Piso 2 y Piso 3.
- Tocar una casilla verde para seleccionarla como destino.
- Botón **Simular carros aleatoriamente**: cambia las posiciones y modelos de los carros en OpenGL.
- Botón **Espacio más cercano**: Dijkstra elige el mejor espacio disponible.
- Botón **Ir a la salida**: calcula la ruta usando exclusivamente la rampa derecha de bajada.
- Botón **Cancelar guía activa**.

## Preparación

1. Instala Flutter y Android Studio.
2. Ejecuta `flutter doctor`.
3. Ejecuta `PREPARAR_FLUTTER.bat` una sola vez.
4. Activa depuración USB en Android.
5. Ejecuta `EJECUTAR_FLUTTER.bat`.

## Conexión

1. PC y teléfono en la misma red.
2. Ejecuta OpenGL.
3. Ejecuta `backend/INICIAR_SERVIDOR.bat`.
4. Ejecuta `ipconfig` en Windows.
5. En la app escribe la IPv4 y puerto, por ejemplo `192.168.1.25:8000`.

No uses `localhost` desde un teléfono físico.
