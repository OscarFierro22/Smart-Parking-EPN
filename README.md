# 🚗 Smart Parking

> Sistema de estacionamiento inteligente desarrollado con **C++**, **OpenGL**, **Flutter** y **FastAPI**.

## 📌 Descripción

Smart Parking es un proyecto que integra una simulación tridimensional de un parqueadero inteligente con una aplicación móvil y un backend para la gestión del estado de los estacionamientos.

El objetivo es proporcionar navegación interna en tiempo real, visualización de plazas disponibles y una representación 3D interactiva del parqueadero.

---

# ✨ Características

- 🏢 Simulación 3D de un parqueadero multinivel.
- 🚗 Vehículo conducible dentro del escenario.
- 🚧 Sistema de colisiones con paredes, pisos y rampas.
- 📱 Aplicación móvil desarrollada con Flutter.
- 🌐 Backend desarrollado con FastAPI.
- 📡 Comunicación mediante Wi-Fi.
- 🧭 Algoritmo de búsqueda de rutas (Dijkstra).
- 🅿 Gestión de plazas disponibles, ocupadas y reservadas.
- 🌙 Cambio entre modo día y noche.
- 🎥 Múltiples cámaras dentro del simulador.

---

# 🖥️ Tecnologías

| Tecnología | Uso |
|------------|-----|
| C++ | Simulador |
| OpenGL | Renderizado 3D |
| GLFW / GLAD | Contexto OpenGL |
| GLM | Matemáticas 3D |
| Assimp | Carga de modelos |
| stb_image | Texturas |
| Flutter | Aplicación móvil |
| FastAPI | Backend |
| Python | API REST |
| Git | Control de versiones |

---

# 🏗️ Arquitectura

```
           Flutter App
                │
          HTTP / JSON
                │
          FastAPI Backend
                │
     Estado del parqueadero
                │
      Simulador OpenGL (C++)
                │
          Algoritmo Dijkstra
```

---

# 📱 Aplicación móvil

La aplicación permite:

- visualizar la disponibilidad de plazas
- seleccionar el piso
- buscar un lugar libre
- buscar la salida
- visualizar un mapa interno
- conectarse automáticamente mediante Wi-Fi al servidor

---

# 🖥️ Simulador OpenGL

El simulador incluye:

- parqueadero de cuatro niveles
- rampas
- plazas de estacionamiento
- iluminación
- texturas
- modelos 3D
- vehículo conducible
- cámaras
- colisiones
- navegación

---

# 📂 Estructura

```
SmartParking
│
├── OpenGL/
│   ├── Vehicle/
│   ├── model/
│   ├── shaders/
│   └── Proyecto_Final.cpp
│
├── backend/
│   ├── server.py
│   └── requirements.txt
│
├── smart_parking_app/
│
├── OpenGL_Stuff/
│
└── OpenGL.sln
```

---

# 🚀 Instalación

## Backend

```bash
cd backend
pip install -r requirements.txt
python server.py
```

## Flutter

```bash
cd smart_parking_app
flutter pub get
flutter run
```

## OpenGL

Abrir la solución:

```
OpenGL.sln
```

Compilar y ejecutar desde Visual Studio.






---

# 🧭 Funcionamiento

1. Se inicia el backend.
2. La aplicación se conecta mediante Wi-Fi.
3. El simulador obtiene el estado del parqueadero.
4. El usuario selecciona una plaza.
5. Dijkstra calcula la ruta óptima.
6. El simulador guía al conductor.
7. La aplicación actualiza el estado en tiempo real.

---


# 📄 Licencia

Proyecto académico desarrollado con fines educativos.
