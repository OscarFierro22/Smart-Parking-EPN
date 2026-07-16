#pragma once

#include <string>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <learnopengl/model.h>
#include <learnopengl/shader.h>

#include "Wheel.h"
#include <vector>

class Vehicle
{
public:
    Vehicle(
        const std::string& bodyPath,
        const std::string& wheelFLPath,
        const std::string& wheelFRPath,
        const std::string& wheelRLPath,
        const std::string& wheelRRPath
    );

    // Lee W, A, S, D, espacio y R.
    void processInput(
        GLFWwindow* window,
        float deltaTime
    );

    // Actualiza dirección, velocidad, orientación y posición.
    void update(float deltaTime);

    // Dibuja la carrocería y las ruedas.
    void draw(Shader& shader);

    // Configuración inicial.
    void setPosition(const glm::vec3& newPosition);
    // Cambia solo la posición actual, sin modificar el punto de reinicio.
    void setCurrentPosition(const glm::vec3& newPosition);
    void setRotation(float newRotation);
    // Cambia solo la rotacion actual, sin modificar el reinicio.
    void setCurrentRotation(float newRotation);
    void setScale(float newScale);
    void setForwardOffset(float offset);

    // Permite una corrección visual adicional si fuera necesaria.
    void setModelPivotOffset(const glm::vec3& offset);

    // Consultas.
    glm::vec3 getPosition() const;
    float getRotation() const;
    float getScale() const;
    float getSpeed() const;
    float getSteeringAngle() const;
    glm::vec3 getForwardVector() const;
    float getPitch() const;

    //Conectar palanca
    void processControllerInput(
        float throttle,
        float steering,
        bool brake,
        bool reset
    );
    // Detiene inmediatamente el automóvil después de una colisión.
    void stop();

    // Reinicia el automóvil.
    void reset();

    private:
    // =====================================================
    // MODELOS
    // =====================================================

    Model body;

    Wheel frontLeftWheel;
    Wheel frontRightWheel;
    Wheel rearLeftWheel;
    Wheel rearRightWheel;

    // =====================================================
    // DEFINICIÓN DE UNA RAMPA
    // =====================================================

    struct Ramp
    {
        glm::vec3 start;
        glm::vec3 end;
        float halfWidth;
    };
    std::vector<Ramp> ramps;

    void initializeRamps();
    void updateRamp(float deltaTime);

    // =====================================================
    // TRANSFORMACIÓN GENERAL
    // =====================================================

    /*
        position representa el origen visual del modelo,
        actualmente ubicado cerca del centro del automóvil.
    */
    glm::vec3 position;
    glm::vec3 initialPosition;

    float rotationY;
    float initialRotationY;

    // Inclinación frontal del vehículo al subir o bajar rampas.
    float rotationX;

    // Inclinación que debe alcanzar según la superficie.
    float targetRotationX;


    /*
        Corrige la dirección frontal del archivo 3D.

        Si el modelo no está orientado hacia el eje +Z,
        este valor permite alinear su frente visual con
        la dirección matemática.
    */
    float forwardOffset;

    float modelScale;

    /*
        Corrección visual opcional del pivote.

        Debe permanecer en cero si el origen de Blender
        ya quedó correctamente situado.
    */
    glm::vec3 modelPivotOffset;

    // =====================================================
    // DIMENSIONES DEL AUTOMÓVIL
    // =====================================================

    /*
        Distancia entre el centro del eje trasero
        y el centro del eje delantero.
    */
    float wheelBase;

    /*
        Distancia longitudinal entre el origen visual
        del modelo y el centro del eje trasero.

        Si el origen está aproximadamente en el centro
        del automóvil, normalmente será wheelBase / 2.
    */
    float rearAxleOffset;

    // =====================================================
    // MOVIMIENTO
    // =====================================================

    float speed;
    float acceleration;

    float maxForwardSpeed;
    float maxReverseSpeed;

    float brakeForce;
    float friction;

    // =====================================================
    // DIRECCIÓN
    // =====================================================

    float maxSteeringAngle;
    float steeringResponse;

    // =====================================================
    // ENTRADAS
    // =====================================================

    float throttleInput;
    float steeringInput;

    bool braking;
    bool resetKeyPressed;

    // =====================================================
    // ESTADO VISUAL
    // =====================================================

    float wheelRollingAngle;
    float steeringAngle;

    

};