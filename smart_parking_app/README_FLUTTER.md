# Smart Parking Flutter — Proyecto final

## Funciones

- Descubrimiento automático del servidor en la misma red Wi-Fi.
- Reconexión y dirección manual como respaldo.
- Mapa superior en vivo por piso.
- Posición exacta, orientación, velocidad y plaza del Mazda.
- Ruta Dijkstra y distancia restante.
- Estados verde, rojo, amarillo y azul.
- Botones **Buscar lugar** y **Buscar salida**.

## Preparación

1. Ejecuta `flutter doctor`.
2. Ejecuta `PREPARAR_FLUTTER.bat` una sola vez.
3. Conecta el teléfono con depuración USB.
4. Ejecuta `EJECUTAR_FLUTTER.bat` para instalar y abrir.

## Wi-Fi

El teléfono y la PC deben estar en la misma red. El backend debe estar abierto
y el firewall configurado. La app intenta encontrarlo por UDP 40404. Para
conexión manual escribe la IPv4 de la PC seguida de `:8000`.
