#pragma once

#include <string>

#include <glm/glm.hpp>

#include <learnopengl/model.h>
#include <learnopengl/shader.h>

class Wheel
{
public:
    Wheel(
        const std::string& modelPath,
        const glm::vec3& ignoredLocalPosition
    );

    void draw(
        Shader& shader,
        const glm::mat4& vehicleTransform,
        float rollingAngle,
        float steeringAngle,
        bool steerable
    );

private:
    Model model;

    // Centro geométrico del OBJ.
    glm::vec3 pivotCenter;

    // Eje real sobre el que debe rodar la rueda.
    glm::vec3 rollingAxis;

    void calculateGeometryData();
};