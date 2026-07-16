#include "Vehicle.h"
#include "Vehicle.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

Vehicle::Vehicle(
    const std::string& bodyPath,
    const std::string& wheelFLPath,
    const std::string& wheelFRPath,
    const std::string& wheelRLPath,
    const std::string& wheelRRPath
    )
    : body(bodyPath),

    frontLeftWheel(
        wheelFLPath,
        glm::vec3(0.0f)
    ),

    frontRightWheel(
        wheelFRPath,
        glm::vec3(0.0f)
    ),

    rearLeftWheel(
        wheelRLPath,
        glm::vec3(0.0f)
    ),

    rearRightWheel(
        wheelRRPath,
        glm::vec3(0.0f)
    ),

    position(0.0f),
    initialPosition(0.0f),

    rotationY(0.0f),
    initialRotationY(0.0f),

    rotationX(0.0f),
    targetRotationX(0.0f),

    forwardOffset(0.0f),
    modelScale(1.0f),

    modelPivotOffset(0.0f),

    // Distancia aproximada entre el eje delantero y trasero.
    // Un valor mayor genera curvas más amplias.
    wheelBase(4.0f),
    rearAxleOffset(2.0f),


    speed(0.0f),
    acceleration(8.0f),
    maxForwardSpeed(12.0f),
    maxReverseSpeed(-10.0f),
    brakeForce(12.0f),
    friction(2.5f),

    // Distancia aproximada entre el eje delantero y trasero.
    // Un valor mayor genera curvas más amplias.
    

    // Giro máximo de las ruedas delanteras.
    maxSteeringAngle(35.0f),

    // Velocidad con la que las ruedas giran y regresan al centro.
    steeringResponse(4.5f),

    throttleInput(0.0f),
    steeringInput(0.0f),
    braking(false),

    wheelRollingAngle(0.0f),
    steeringAngle(0.0f),

    resetKeyPressed(false)
    {
        initializeRamps();
}


void Vehicle::initializeRamps()
{
    ramps.clear();

    // =====================================================
    // ALTURAS NORMALIZADAS DE LOS CUATRO NIVELES
    // =====================================================

    constexpr float FLOOR_0_Y = -1.52000f;
    constexpr float FLOOR_1_Y = 6.37726f;
    constexpr float FLOOR_2_Y = 14.45185f;
    constexpr float FLOOR_3_Y = 22.54915f;

    // =====================================================
    // GEOMETRÍA HORIZONTAL DEL LADO IZQUIERDO
    //
    // Todas las rampas izquierdas se consideran apiladas
    // verticalmente y con la misma forma horizontal.
    // =====================================================

    constexpr float LEFT_CENTER_X = -37.34445f;
    constexpr float LEFT_START_Z = 16.19410f;
    constexpr float LEFT_END_Z = -16.51065f;
    constexpr float LEFT_HALF_WIDTH = 5.15f;

    // =====================================================
// RAMPA DE ENTRADA
//
// Centro inferior calculado:
// X = 12.203005
// Z = 46.53415
//
// Centro superior calculado:
// X = 12.290345
// Z = 33.08785
//
// El desnivel medido es aproximadamente 2.10221.
// Hacemos que su extremo superior conecte exactamente
// con la altura del piso 0.
// =====================================================

    constexpr float ENTRANCE_LOWER_X = 12.20301f;
    constexpr float ENTRANCE_LOWER_Z = 46.53415f;

    constexpr float ENTRANCE_UPPER_X = 12.29035f;
    constexpr float ENTRANCE_UPPER_Z = 33.08785f;

    constexpr float ENTRANCE_HEIGHT_DIFFERENCE = 2.10221f;

    constexpr float ENTRANCE_LOWER_Y =
        FLOOR_0_Y - ENTRANCE_HEIGHT_DIFFERENCE;

    constexpr float ENTRANCE_HALF_WIDTH = 8.50f;

    ramps.push_back(
        {
            glm::vec3(
                ENTRANCE_LOWER_X,
                ENTRANCE_LOWER_Y,
                ENTRANCE_LOWER_Z
            ),
            glm::vec3(
                ENTRANCE_UPPER_X,
                FLOOR_0_Y,
                ENTRANCE_UPPER_Z
            ),
            ENTRANCE_HALF_WIDTH
        }
    );


    // Izquierda: nivel 0 -> nivel 1
    ramps.push_back(
        {
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_0_Y,
                LEFT_START_Z
            ),
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_1_Y,
                LEFT_END_Z
            ),
            LEFT_HALF_WIDTH
        }
    );

    // Izquierda: nivel 1 -> nivel 2
    ramps.push_back(
        {
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_1_Y,
                LEFT_START_Z
            ),
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_2_Y,
                LEFT_END_Z
            ),
            LEFT_HALF_WIDTH
        }
    );

    // Izquierda: nivel 2 -> nivel 3
    ramps.push_back(
        {
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_2_Y,
                LEFT_START_Z
            ),
            glm::vec3(
                LEFT_CENTER_X,
                FLOOR_3_Y,
                LEFT_END_Z
            ),
            LEFT_HALF_WIDTH
        }
    );

    // =====================================================
    // GEOMETRÍA HORIZONTAL DEL LADO DERECHO
    //
    // La dirección es inversa respecto al lado izquierdo.
    // =====================================================

    constexpr float RIGHT_CENTER_X = 37.18023f;
    constexpr float RIGHT_START_Z = -16.42415f;
    constexpr float RIGHT_END_Z = 15.75120f;
    constexpr float RIGHT_HALF_WIDTH = 5.45f;

    // Derecha: nivel 0 -> nivel 1
    ramps.push_back(
        {
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_0_Y,
                RIGHT_START_Z
            ),
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_1_Y,
                RIGHT_END_Z
            ),
            RIGHT_HALF_WIDTH
        }
    );

    // Derecha: nivel 1 -> nivel 2
    ramps.push_back(
        {
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_1_Y,
                RIGHT_START_Z
            ),
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_2_Y,
                RIGHT_END_Z
            ),
            RIGHT_HALF_WIDTH
        }
    );

    // Derecha: nivel 2 -> nivel 3
    ramps.push_back(
        {
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_2_Y,
                RIGHT_START_Z
            ),
            glm::vec3(
                RIGHT_CENTER_X,
                FLOOR_3_Y,
                RIGHT_END_Z
            ),
            RIGHT_HALF_WIDTH
        }
    );

    std::cout
        << "Rampas configuradas: "
        << ramps.size()
        << std::endl;
}

// ---------------------------------------------------------
// Lectura de teclado
// ---------------------------------------------------------
void Vehicle::processInput(
    GLFWwindow* window,
    float deltaTime
    )
    {
    (void)deltaTime;

    throttleInput = 0.0f;
    steeringInput = 0.0f;
    braking = false;

    // W: avanzar.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        throttleInput = 1.0f;
    }

    // S: retroceder.
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        throttleInput = -1.0f;
    }

    /*
        En un sistema derecho con Y hacia arriba,
        disminuir rotationY produce un giro visual
        hacia la izquierda.
    */

    // A: izquierda.
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        steeringInput = 1.0f;
    }

    // D: derecha.
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        steeringInput = -1.0f;
    }

    // Espacio: frenar.
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        braking = true;
    }

    // R: reiniciar una sola vez por pulsación.
    const bool resetPressed =
        glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

    if (resetPressed && !resetKeyPressed)
    {
        reset();
    }

    resetKeyPressed = resetPressed;
}

// ---------------------------------------------------------
// Actualización del vehículo
// ---------------------------------------------------------

void Vehicle::update(float deltaTime)
{
    // Evita movimientos exagerados cuando un frame tarda demasiado.
    deltaTime = std::min(deltaTime, 0.05f);

    // =====================================================
    // 1. ACELERACIÓN, REVERSA Y CAMBIO DE SENTIDO
    // =====================================================

    if (throttleInput > 0.0f)
    {
        if (speed < 0.0f)
        {
            // Si el automóvil estaba retrocediendo,
            // primero reduce la velocidad hasta detenerse.
            speed += brakeForce * deltaTime;

            if (speed > 0.0f)
            {
                speed = 0.0f;
            }
        }
        else
        {
            speed += acceleration * deltaTime;
        }
    }
    else if (throttleInput < 0.0f)
    {
        if (speed > 0.0f)
        {
            // Si el automóvil avanzaba,
            // primero reduce la velocidad hasta detenerse.
            speed -= brakeForce * deltaTime;

            if (speed < 0.0f)
            {
                speed = 0.0f;
            }
        }
        else
        {
            speed -= acceleration * deltaTime;
        }
    }
    else
    {
        // Fricción natural cuando no se presiona W ni S.
        if (speed > 0.0f)
        {
            speed -= friction * deltaTime;

            if (speed < 0.0f)
            {
                speed = 0.0f;
            }
        }
        else if (speed < 0.0f)
        {
            speed += friction * deltaTime;

            if (speed > 0.0f)
            {
                speed = 0.0f;
            }
        }
    }

    // =====================================================
    // 2. FRENO
    // =====================================================

    if (braking)
    {
        if (speed > 0.0f)
        {
            speed -= brakeForce * deltaTime;

            if (speed < 0.0f)
            {
                speed = 0.0f;
            }
        }
        else if (speed < 0.0f)
        {
            speed += brakeForce * deltaTime;

            if (speed > 0.0f)
            {
                speed = 0.0f;
            }
        }
    }

    speed = std::clamp(
        speed,
        maxReverseSpeed,
        maxForwardSpeed
    );

    // =====================================================
    // 3. DIRECCIÓN
    // =====================================================

    /*
        El radio de giro debe depender principalmente
        del ángulo de las ruedas y de wheelBase.

        No reducimos el ángulo según la velocidad porque,
        si A o D permanecen presionadas, queremos que el
        vehículo conserve una circunferencia estable.
    */

    const float targetSteeringAngle =
        steeringInput * maxSteeringAngle;

    /*
        Velocidad angular de la dirección en grados/segundo.

        steeringResponse = 4.5
        maxSteeringAngle = 35

        Resultado aproximado:
        157.5 grados por segundo.
    */
    const float maximumSteeringChange =
        steeringResponse *
        maxSteeringAngle *
        deltaTime;

    const float steeringDifference =
        targetSteeringAngle - steeringAngle;

    steeringAngle += std::clamp(
        steeringDifference,
        -maximumSteeringChange,
        maximumSteeringChange
    );

    // Elimina residuos numéricos cuando se suelta A o D.
    if (
        steeringInput == 0.0f &&
        std::abs(steeringAngle) < 0.05f
        )
    {
        steeringAngle = 0.0f;
    }
    // =====================================================
    // 4. MODELO CINEMÁTICO DESDE EL CENTRO DEL VEHÍCULO
    // =====================================================

    const float oldHeading =
        glm::radians(rotationY + forwardOffset);

    const glm::vec3 oldForward(
        std::sin(oldHeading),
        0.0f,
        std::cos(oldHeading)
    );

    /*
        position es el origen central del modelo.

        Calculamos primero dónde está el centro
        del eje trasero.
    */
    glm::vec3 rearAxlePosition =
        position - oldForward * rearAxleOffset;

    const float steeringRadians =
        glm::radians(steeringAngle);

    const float minimumSpeed = 0.0001f;
    const float minimumSteering =
        glm::radians(0.01f);

    float newHeading = oldHeading;

    if (
        std::abs(speed) > minimumSpeed &&
        std::abs(steeringRadians) > minimumSteering
        )
    {
        const float angularVelocity =
            (speed / wheelBase) *
            std::tan(steeringRadians);

        newHeading =
            oldHeading +
            angularVelocity * deltaTime;

        const float turningRadius =
            wheelBase /
            std::tan(steeringRadians);

        rearAxlePosition.x +=
            turningRadius *
            (
                std::cos(oldHeading) -
                std::cos(newHeading)
                );

        rearAxlePosition.z +=
            turningRadius *
            (
                std::sin(newHeading) -
                std::sin(oldHeading)
                );
    }
    else
    {
        rearAxlePosition.x +=
            speed *
            std::sin(oldHeading) *
            deltaTime;

        rearAxlePosition.z +=
            speed *
            std::cos(oldHeading) *
            deltaTime;
    }

    /*
        Reconstruimos la posición central del automóvil
        desde la nueva posición del eje trasero.
    */
    const glm::vec3 newForward(
        std::sin(newHeading),
        0.0f,
        std::cos(newHeading)
    );

    position =
        rearAxlePosition +
        newForward * rearAxleOffset;

    rotationY =
        glm::degrees(newHeading) -
        forwardOffset;

    while (rotationY > 180.0f)
    {
        rotationY -= 360.0f;
    }

    while (rotationY < -180.0f)
    {
        rotationY += 360.0f;
    }
    

    // =====================================================
    // 5. ROTACIÓN VISUAL DE LAS RUEDAS
    // =====================================================

    wheelRollingAngle +=
        speed *
        120.0f *
        deltaTime;

    while (wheelRollingAngle > 360.0f)
    {
        wheelRollingAngle -= 360.0f;
    }

    while (wheelRollingAngle < -360.0f)
    {
        wheelRollingAngle += 360.0f;
    }
    static float printTimer = 0.0f;

    printTimer += deltaTime;

    if (printTimer >= 0.5f)
    {
        std::cout
            << "Vehicle position -> X: "
            << position.x
            << " Y: "
            << position.y
            << " Z: "
            << position.z
            << std::endl;

        printTimer = 0.0f;
    }


    updateRamp(deltaTime);

}
// ---------------------------------------------------------
// Renderizado
// ---------------------------------------------------------
void Vehicle::draw(Shader& shader)
{
    glm::mat4 vehicleTransform =
        glm::mat4(1.0f);

    // La posición física corresponde al centro del eje trasero.
    vehicleTransform = glm::translate(
        vehicleTransform,
        position
    );

    // Rotación general del vehículo.
    vehicleTransform = glm::rotate(
        vehicleTransform,
        glm::radians(rotationY),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    /*
    Eje transversal real del Mazda.

    Se obtuvo a partir de los centros de las ruedas.
    Esto evita que un costado quede más alto que el otro.
*/
    const glm::vec3 vehiclePitchAxis =
        glm::normalize(
            glm::vec3(
                0.81915f,
                0.0f,
                0.57358f
            )
        );

    vehicleTransform = glm::rotate(
        vehicleTransform,
        glm::radians(rotationX),
        vehiclePitchAxis
    );

    // Escala visual.
    vehicleTransform = glm::scale(
        vehicleTransform,
        glm::vec3(modelScale)
    );

    /*
        modelPivotOffset debe representar la distancia
        desde el origen del OBJ hasta el centro del eje trasero.
    */
    vehicleTransform = glm::translate(
        vehicleTransform,
        -modelPivotOffset
    );

    shader.setMat4(
        "model",
        vehicleTransform
    );

    body.Draw(shader);

    frontLeftWheel.draw(
        shader,
        vehicleTransform,
        wheelRollingAngle,
        steeringAngle,
        true
    );

    frontRightWheel.draw(
        shader,
        vehicleTransform,
        wheelRollingAngle,
        steeringAngle,
        true
    );

    rearLeftWheel.draw(
        shader,
        vehicleTransform,
        wheelRollingAngle,
        0.0f,
        false
    );

    rearRightWheel.draw(
        shader,
        vehicleTransform,
        wheelRollingAngle,
        0.0f,
        false
    );
}



// ---------------------------------------------------------
// Setters
// ---------------------------------------------------------
void Vehicle::setPosition(
    const glm::vec3& newPosition
)
{
    position = newPosition;
    initialPosition = newPosition;
}

void Vehicle::setRotation(float newRotation)
{
    rotationY = newRotation;
    initialRotationY = newRotation;
}

void Vehicle::setScale(float newScale)
{
    modelScale = newScale;
}

void Vehicle::setForwardOffset(float offset)
{
    forwardOffset = offset;
}

void Vehicle::setModelPivotOffset(
    const glm::vec3& offset
)
{
    modelPivotOffset = offset;
}

// ---------------------------------------------------------
// Getters
// ---------------------------------------------------------
glm::vec3 Vehicle::getPosition() const
{
    return position;
}

float Vehicle::getRotation() const
{
    return rotationY;
}

float Vehicle::getScale() const
{
    return modelScale;
}

float Vehicle::getSpeed() const
{
    return speed;
}

float Vehicle::getSteeringAngle() const
{
    return steeringAngle;
}

glm::vec3 Vehicle::getForwardVector() const
{
    const float heading =
        glm::radians(
            rotationY +
            forwardOffset
        );

    return glm::normalize(
        glm::vec3(
            std::sin(heading),
            0.0f,
            std::cos(heading)
        )
    );
}

float Vehicle::getPitch() const
{
    return rotationX;
}


// ---------------------------------------------------------
// Reinicio
// ---------------------------------------------------------
void Vehicle::reset()
{
    position = initialPosition;
    rotationY = initialRotationY;

    speed = 0.0f;

    wheelRollingAngle = 0.0f;
    steeringAngle = 0.0f;

    throttleInput = 0.0f;
    steeringInput = 0.0f;
    braking = false;

    rotationX = 0.0f;
    targetRotationX = 0.0f;
}

void Vehicle::updateRamp(float deltaTime)
{
    // Alturas planas normalizadas del estacionamiento.
    constexpr float FLOOR_HEIGHTS[] =
    {
        -1.52000f,
         6.37726f,
        14.45185f,
        22.54915f
    };

    constexpr int FLOOR_COUNT =
        sizeof(FLOOR_HEIGHTS) /
        sizeof(FLOOR_HEIGHTS[0]);

    constexpr float suspensionHeight = 0.1f;

    // =====================================================
    // 1. DIRECCIÓN DEL VEHÍCULO
    // =====================================================

    const float heading =
        glm::radians(
            rotationY +
            forwardOffset
        );

    const glm::vec3 vehicleForward(
        std::sin(heading),
        0.0f,
        std::cos(heading)
    );

    const float frontAxleOffset =
        wheelBase - rearAxleOffset;

    const glm::vec3 rearAxlePosition =
        position -
        vehicleForward * rearAxleOffset;

    const glm::vec3 frontAxlePosition =
        position +
        vehicleForward * frontAxleOffset;

    // =====================================================
    // 2. BUSCAR LA RAMPA ACTIVA
    // =====================================================

    const Ramp* activeRamp = nullptr;

    float bestRampScore =
        std::numeric_limits<float>::max();

    for (const Ramp& ramp : ramps)
    {
        glm::vec2 rampDirection(
            ramp.end.x - ramp.start.x,
            ramp.end.z - ramp.start.z
        );

        const float rampLength =
            glm::length(rampDirection);

        if (rampLength <= 0.0001f)
        {
            continue;
        }

        rampDirection /=
            rampLength;

        const glm::vec2 rampPerpendicular(
            -rampDirection.y,
            rampDirection.x
        );

        auto getRampCoordinates =
            [&](const glm::vec3& point)
            {
                const glm::vec2 relativePosition(
                    point.x - ramp.start.x,
                    point.z - ramp.start.z
                );

                const float longitudinal =
                    glm::dot(
                        relativePosition,
                        rampDirection
                    );

                const float lateral =
                    std::abs(
                        glm::dot(
                            relativePosition,
                            rampPerpendicular
                        )
                    );

                return glm::vec2(
                    longitudinal,
                    lateral
                );
            };

        const glm::vec2 frontCoordinates =
            getRampCoordinates(
                frontAxlePosition
            );

        const glm::vec2 rearCoordinates =
            getRampCoordinates(
                rearAxlePosition
            );

        /*
            La rampa se considera candidata si al menos uno
            de los dos ejes está dentro de su superficie.
        */
        const bool frontTouchesRamp =
            frontCoordinates.x >= 0.0f &&
            frontCoordinates.x <= rampLength &&
            frontCoordinates.y <= ramp.halfWidth;

        const bool rearTouchesRamp =
            rearCoordinates.x >= 0.0f &&
            rearCoordinates.x <= rampLength &&
            rearCoordinates.y <= ramp.halfWidth;

        if (
            !frontTouchesRamp &&
            !rearTouchesRamp
            )
        {
            continue;
        }

        /*
            Como las rampas superiores están exactamente
            encima de las inferiores, debemos decidir cuál
            corresponde a la altura actual del automóvil.

            Calculamos la altura esperada de esta rampa en
            la posición central del Mazda.
        */
        const glm::vec2 centerCoordinates =
            getRampCoordinates(position);

        const float centerProgress =
            glm::clamp(
                centerCoordinates.x /
                rampLength,
                0.0f,
                1.0f
            );

        const float expectedHeight =
            glm::mix(
                ramp.start.y,
                ramp.end.y,
                centerProgress
            );

        const float rampScore =
            std::abs(
                position.y -
                suspensionHeight -
                expectedHeight
            );

        if (rampScore < bestRampScore)
        {
            bestRampScore = rampScore;
            activeRamp = &ramp;
        }
    }

    // =====================================================
    // 3. SI ESTÁ EN UNA RAMPA
    // =====================================================

    if (activeRamp != nullptr)
    {
        glm::vec2 rampDirection(
            activeRamp->end.x -
            activeRamp->start.x,

            activeRamp->end.z -
            activeRamp->start.z
        );

        const float rampLength =
            glm::length(rampDirection);

        rampDirection /=
            rampLength;

        const glm::vec2 rampPerpendicular(
            -rampDirection.y,
            rampDirection.x
        );

        auto getSurfaceHeight =
            [&](const glm::vec3& samplePosition)
            -> float
            {
                const glm::vec2 relativePosition(
                    samplePosition.x -
                    activeRamp->start.x,

                    samplePosition.z -
                    activeRamp->start.z
                );

                const float longitudinal =
                    glm::dot(
                        relativePosition,
                        rampDirection
                    );

                const float lateral =
                    std::abs(
                        glm::dot(
                            relativePosition,
                            rampPerpendicular
                        )
                    );

                /*
                    Si el punto está fuera lateralmente,
                    mantenemos la altura más cercana entre
                    el piso inferior y el superior.
                */
                if (
                    lateral >
                    activeRamp->halfWidth
                    )
                {
                    const float lowerDistance =
                        std::abs(
                            position.y -
                            suspensionHeight -
                            activeRamp->start.y
                        );

                    const float upperDistance =
                        std::abs(
                            position.y -
                            suspensionHeight -
                            activeRamp->end.y
                        );

                    return upperDistance < lowerDistance
                        ? activeRamp->end.y
                        : activeRamp->start.y;
                }

                // Antes del inicio: piso inferior.
                if (longitudinal <= 0.0f)
                {
                    return activeRamp->start.y;
                }

                // Después del final: piso superior.
                if (longitudinal >= rampLength)
                {
                    return activeRamp->end.y;
                }

                const float t =
                    glm::clamp(
                        longitudinal /
                        rampLength,
                        0.0f,
                        1.0f
                    );

                return glm::mix(
                    activeRamp->start.y,
                    activeRamp->end.y,
                    t
                );
            };

        const float rearGroundHeight =
            getSurfaceHeight(
                rearAxlePosition
            );

        const float frontGroundHeight =
            getSurfaceHeight(
                frontAxlePosition
            );

        const float targetVehicleHeight =
            (
                rearGroundHeight +
                frontGroundHeight
                ) * 0.5f;

        position.y =
            targetVehicleHeight +
            suspensionHeight;

        /*
            La diferencia de altura entre ambos ejes hace
            que el coche comience a inclinarse cuando las
            ruedas delanteras entran en la rampa y se
            enderece cuando llegan al piso superior.
        */
        const float axleHeightDifference =
            frontGroundHeight -
            rearGroundHeight;

        targetRotationX =
            glm::degrees(
                std::atan2(
                    axleHeightDifference,
                    wheelBase
                )
            );
    }
    else
    {
        // =================================================
        // 4. FUERA DE RAMPAS: FIJAR AL PISO MÁS CERCANO
        // =================================================

        float nearestFloor =
            FLOOR_HEIGHTS[0];

        float nearestDistance =
            std::abs(
                position.y -
                suspensionHeight -
                FLOOR_HEIGHTS[0]
            );

        for (
            int i = 1;
            i < FLOOR_COUNT;
            ++i
            )
        {
            const float currentDistance =
                std::abs(
                    position.y -
                    suspensionHeight -
                    FLOOR_HEIGHTS[i]
                );

            if (
                currentDistance <
                nearestDistance
                )
            {
                nearestDistance =
                    currentDistance;

                nearestFloor =
                    FLOOR_HEIGHTS[i];
            }
        }

        position.y =
            nearestFloor +
            suspensionHeight;

        targetRotationX = 0.0f;
    }

    // =====================================================
    // 5. SUAVIZADO DE INCLINACIÓN
    // =====================================================

    constexpr float pitchResponse =
        10.0f;

    const float interpolationFactor =
        glm::clamp(
            pitchResponse *
            deltaTime,
            0.0f,
            1.0f
        );

    rotationX =
        glm::mix(
            rotationX,
            targetRotationX,
            interpolationFactor
        );
}

void Vehicle::processControllerInput(
    float throttle,
    float steering,
    bool brake,
    bool reset
)
{
    throttleInput =
        glm::clamp(
            throttle,
            -1.0f,
            1.0f
        );

    steeringInput =
        glm::clamp(
            steering,
            -1.0f,
            1.0f
        );

    braking =
        brake;

    if (
        reset &&
        !resetKeyPressed
        )
    {
        this->reset();
    }

    resetKeyPressed =
        reset;
}