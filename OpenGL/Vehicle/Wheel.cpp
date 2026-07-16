#include "Wheel.h"

#include <algorithm>
#include <iostream>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

Wheel::Wheel(
    const std::string& modelPath,
    const glm::vec3& ignoredLocalPosition
)
    : model(modelPath),
    pivotCenter(0.0f),
    rollingAxis(1.0f, 0.0f, 0.0f)
{
    (void)ignoredLocalPosition;

    calculateGeometryData();
}

void Wheel::calculateGeometryData()
{
    glm::vec3 minimum(
        std::numeric_limits<float>::max()
    );

    glm::vec3 maximum(
        std::numeric_limits<float>::lowest()
    );

    bool foundVertex = false;

    for (const Mesh& mesh : model.meshes)
    {
        for (const Vertex& vertex : mesh.vertices)
        {
            minimum.x = std::min(
                minimum.x,
                vertex.Position.x
            );

            minimum.y = std::min(
                minimum.y,
                vertex.Position.y
            );

            minimum.z = std::min(
                minimum.z,
                vertex.Position.z
            );

            maximum.x = std::max(
                maximum.x,
                vertex.Position.x
            );

            maximum.y = std::max(
                maximum.y,
                vertex.Position.y
            );

            maximum.z = std::max(
                maximum.z,
                vertex.Position.z
            );

            foundVertex = true;
        }
    }

    if (!foundVertex)
    {
        pivotCenter = glm::vec3(0.0f);

        rollingAxis = glm::vec3(
            0.81915f,
            0.0f,
            0.57358f
        );

        return;
    }

    pivotCenter =
        (minimum + maximum) * 0.5f;

    const glm::vec3 dimensions =
        maximum - minimum;

    /*
        El modelo fue exportado con una rotación diagonal.
        El eje de las ruedas no coincide exactamente con X
        ni con Z global.

        Este vector se obtuvo usando la dirección entre
        los centros de las ruedas izquierda y derecha.
    */
    rollingAxis = glm::normalize(
        glm::vec3(
            0.81915f,
            0.0f,
            0.57358f
        )
    );

    std::cout
        << "Wheel pivot: "
        << pivotCenter.x << ", "
        << pivotCenter.y << ", "
        << pivotCenter.z
        << " | Dimensions: "
        << dimensions.x << ", "
        << dimensions.y << ", "
        << dimensions.z
        << " | Rolling axis: "
        << rollingAxis.x << ", "
        << rollingAxis.y << ", "
        << rollingAxis.z
        << std::endl;
}




void Wheel::draw(
    Shader& shader,
    const glm::mat4& vehicleTransform,
    float rollingAngle,
    float steeringAngle,
    bool steerable
)
{
    glm::mat4 wheelTransform =
        vehicleTransform;

    // Llevar el origen al centro real de la rueda.
    wheelTransform = glm::translate(
        wheelTransform,
        pivotCenter
    );

    /*
        Las ruedas delanteras primero se orientan
        hacia izquierda o derecha.

        Como esta rotación se aplica antes del rodamiento,
        el eje de rodamiento también queda orientado
        junto con la rueda.
    */
    if (steerable)
    {
        wheelTransform = glm::rotate(
            wheelTransform,
            glm::radians(steeringAngle),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    

    // Rodamiento sobre el eje detectado automáticamente.
    wheelTransform = glm::rotate(
        wheelTransform,
        glm::radians(-rollingAngle),
        rollingAxis
    );

    // Regresar al sistema local del automóvil.
    wheelTransform = glm::translate(
        wheelTransform,
        -pivotCenter
    );

    shader.setMat4(
        "model",
        wheelTransform
    );

    model.Draw(shader);
}