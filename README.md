# 🚗 Smart Parking EPN

<p align="center">
  <img src="images/portada.png" alt="Smart Parking EPN" width="100%">
</p>

<p align="center">

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![Flutter](https://img.shields.io/badge/Flutter-02569B?style=for-the-badge&logo=flutter&logoColor=white)
![FastAPI](https://img.shields.io/badge/FastAPI-009688?style=for-the-badge&logo=fastapi&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)
![Git](https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white)

</p>

---

# 📌 Descripción

**Smart Parking EPN** es un sistema inteligente de estacionamiento desarrollado como proyecto académico para la **Escuela Politécnica Nacional (EPN)**.

El proyecto integra un simulador 3D desarrollado con **OpenGL**, una aplicación móvil construida con **Flutter** y un backend implementado con **FastAPI**, permitiendo gestionar espacios de estacionamiento y visualizar rutas óptimas dentro del parqueadero.

---

# ✨ Características

- 🚗 Simulación 3D del parqueadero.
- 📱 Aplicación móvil desarrollada en Flutter.
- 🌐 Backend REST desarrollado con FastAPI.
- 🧭 Cálculo de rutas mediante el algoritmo de Dijkstra.
- 🚦 Gestión de espacios disponibles, ocupados y reservados.
- 🚧 Restricciones de circulación dentro del parqueadero.
- 🧱 Detección de colisiones con la infraestructura.
- 🅿️ Sistema de navegación hacia espacios disponibles.

---

# 🏗 Arquitectura del sistema

```text
                    Flutter App
                         │
                         │
                    HTTP / REST
                         │
                         ▼
                  FastAPI Backend
                         │
                         │
                         ▼
               OpenGL Smart Parking
                         │
                         ▼
      Dijkstra • Parking Logic • Collision System
```

---

# 📸 Simulación 3D

### Vista general del sistema

<p align="center">
<img src="images/simulation.png" width="90%">
</p>

El simulador representa el parqueadero utilizando gráficos 3D desarrollados en OpenGL.

Incluye infraestructura, espacios de estacionamiento y navegación del vehículo.

---

# 📱 Aplicación móvil

<p align="center">
<img src="images/app_flutter.png" width="70%">
</p>

La aplicación Flutter permite visualizar el estado del parqueadero y constituye la interfaz móvil del sistema.

---

# 🧭 Algoritmo de Dijkstra

<p align="center">
<img src="images/ruta_dijkstra.jpeg" width="90%">
</p>

El algoritmo de Dijkstra se utiliza para calcular la ruta más corta desde la posición actual del vehículo hasta un espacio de estacionamiento disponible.

---

# 🚧 Restricciones y colisiones

<p align="center">
<img src="images/restriction.png" width="90%">
</p>

El sistema implementa restricciones de circulación y colisiones para impedir que el vehículo atraviese paredes, rampas y otros elementos estructurales del parqueadero.

---

# 🛠 Tecnologías utilizadas

| Tecnología | Uso |
|------------|-----|
| C++ | Simulación principal |
| OpenGL | Renderizado 3D |
| GLFW | Ventanas y entrada |
| GLAD | Carga de OpenGL |
| GLM | Matemática 3D |
| Assimp | Importación de modelos |
| Flutter | Aplicación móvil |
| FastAPI | Backend REST |
| Python | Servicios backend |
| Git | Control de versiones |
| GitHub | Hospedaje del proyecto |

---

# 📂 Estructura del proyecto

```text
Smart-Parking-EPN
│
├── backend/
├── docs/
├── images/
├── OpenGL/
├── smart_parking_app/
├── README.md
└── ...
```

---

# 🚀 Cómo ejecutar el proyecto

## 1. Clonar el repositorio

```bash
git clone https://github.com/OscarFierro22/Smart-Parking-EPN.git
```

---

## 2. Ejecutar el Backend

```bash
cd backend
```

Instalar dependencias necesarias.

Ejecutar el servidor FastAPI.

---

## 3. Ejecutar OpenGL

Abrir la solución de Visual Studio.

Compilar el proyecto.

Ejecutar el simulador.

---

## 4. Ejecutar Flutter

```bash
cd smart_parking_app
flutter pub get
flutter run
```

---

# 📄 Documentación técnica

El informe técnico completo del proyecto puede descargarse aquí.

📥 **[Descargar Informe Técnico](docs/Informe_Tecnico_Smart_Parking.docx)**

---

# 🎯 Objetivos del proyecto

- Aplicar algoritmos de grafos.
- Desarrollar simulaciones mediante Computación Gráfica.
- Integrar aplicaciones móviles con simuladores.
- Implementar una arquitectura cliente-servidor.
- Aplicar principios de Ingeniería de Software.

---


# 📄 Licencia

Proyecto académico desarrollado con fines educativos.

Escuela Politécnica Nacional (EPN).
