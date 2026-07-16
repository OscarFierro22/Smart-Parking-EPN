#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using namespace std;

// =========================================================
// INFORMACIÓN DE CADA VÉRTICE
// =========================================================

struct Vertex
{
    glm::vec3 Position{ 0.0f };
    glm::vec3 Normal{ 0.0f };
    glm::vec2 TexCoords{ 0.0f };
    glm::vec3 Tangent{ 0.0f };
    glm::vec3 Bitangent{ 0.0f };
};

// =========================================================
// INFORMACIÓN DE CADA TEXTURA
// =========================================================

struct Texture
{
    unsigned int id = 0;

    string type;
    string path;
};

// =========================================================
// INFORMACIÓN DEL MATERIAL LEÍDO DESDE EL ARCHIVO MTL
// =========================================================

struct MeshMaterial
{
    // Color difuso del material.
    glm::vec3 diffuse{ 0.5f };

    // Color especular del material.
    glm::vec3 specular{ 0.5f };

    // Brillo Ns definido en el archivo MTL.
    float shininess = 32.0f;

    // Opacidad del material.
    // 1.0 = opaco.
    // 0.0 = completamente transparente.
    float opacity = 1.0f;

    // Nombre del material leído desde el archivo MTL.
    string name;

    // Indica si se aplicará un tratamiento metálico.
    bool metallic = false;

    // Indica si se dibujará en la pasada transparente.
    bool transparent = false;
};

// =========================================================
// CLASE MESH
// =========================================================

class Mesh
{
public:

    // Datos de la malla.
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    MeshMaterial material;

    unsigned int VAO = 0;

    // =====================================================
    // CONSTRUCTOR
    // =====================================================

    Mesh(
        vector<Vertex> vertices,
        vector<unsigned int> indices,
        vector<Texture> textures,
        MeshMaterial material
    )
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->material = material;

        setupMesh();
    }

    // =====================================================
    // DIBUJAR LA MALLA
    // =====================================================

    void Draw(Shader& shader)
    {
        // -------------------------------------------------
        // 1. Enviar las propiedades del material al shader
        // -------------------------------------------------

        shader.setVec3(
            "materialDiffuse",
            material.diffuse
        );

        shader.setVec3(
            "materialSpecular",
            material.specular
        );

        shader.setFloat(
            "materialShininess",
            std::max(
                material.shininess,
                1.0f
            )
        );

        shader.setFloat(
            "materialOpacity",
            glm::clamp(
                material.opacity,
                0.0f,
                1.0f
            )
        );

        shader.setBool(
            "materialMetallic",
            material.metallic
        );

        // -------------------------------------------------
        // 2. Vincular las texturas
        // -------------------------------------------------

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        for (
            unsigned int i = 0;
            i < textures.size();
            i++
        )
        {
            glActiveTexture(
                GL_TEXTURE0 + i
            );

            string number;
            string textureType = textures[i].type;

            if (
                textureType ==
                "texture_diffuse"
            )
            {
                number =
                    std::to_string(
                        diffuseNr++
                    );
            }
            else if (
                textureType ==
                "texture_specular"
            )
            {
                number =
                    std::to_string(
                        specularNr++
                    );
            }
            else if (
                textureType ==
                "texture_normal"
            )
            {
                number =
                    std::to_string(
                        normalNr++
                    );
            }
            else if (
                textureType ==
                "texture_height"
            )
            {
                number =
                    std::to_string(
                        heightNr++
                    );
            }

            string uniformName =
                textureType +
                number;

            glUniform1i(
                glGetUniformLocation(
                    shader.ID,
                    uniformName.c_str()
                ),
                static_cast<GLint>(i)
            );

            glBindTexture(
                GL_TEXTURE_2D,
                textures[i].id
            );
        }

        // -------------------------------------------------
        // 3. Dibujar la malla
        // -------------------------------------------------

        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(
                indices.size()
            ),
            GL_UNSIGNED_INT,
            nullptr
        );

        glBindVertexArray(0);

        // -------------------------------------------------
        // 4. Restaurar la unidad de textura
        // -------------------------------------------------

        glActiveTexture(
            GL_TEXTURE0
        );
    }

private:

    // Datos de renderizado.
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    // =====================================================
    // CONFIGURAR VAO, VBO Y EBO
    // =====================================================

    void setupMesh()
    {
        glGenVertexArrays(
            1,
            &VAO
        );

        glGenBuffers(
            1,
            &VBO
        );

        glGenBuffers(
            1,
            &EBO
        );

        glBindVertexArray(VAO);

        // -------------------------------------------------
        // Almacenar los vértices en el VBO
        // -------------------------------------------------

        glBindBuffer(
            GL_ARRAY_BUFFER,
            VBO
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertices.size() *
                sizeof(Vertex)
            ),
            vertices.empty()
                ? nullptr
                : vertices.data(),
            GL_STATIC_DRAW
        );

        // -------------------------------------------------
        // Almacenar los índices en el EBO
        // -------------------------------------------------

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            EBO
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                indices.size() *
                sizeof(unsigned int)
            ),
            indices.empty()
                ? nullptr
                : indices.data(),
            GL_STATIC_DRAW
        );

        // -------------------------------------------------
        // Atributo 0: posición
        // -------------------------------------------------

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                offsetof(
                    Vertex,
                    Position
                )
            )
        );

        // -------------------------------------------------
        // Atributo 1: normal
        // -------------------------------------------------

        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                offsetof(
                    Vertex,
                    Normal
                )
            )
        );

        // -------------------------------------------------
        // Atributo 2: coordenadas de textura
        // -------------------------------------------------

        glEnableVertexAttribArray(2);

        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                offsetof(
                    Vertex,
                    TexCoords
                )
            )
        );

        // -------------------------------------------------
        // Atributo 3: tangente
        // -------------------------------------------------

        glEnableVertexAttribArray(3);

        glVertexAttribPointer(
            3,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                offsetof(
                    Vertex,
                    Tangent
                )
            )
        );

        // -------------------------------------------------
        // Atributo 4: bitangente
        // -------------------------------------------------

        glEnableVertexAttribArray(4);

        glVertexAttribPointer(
            4,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                offsetof(
                    Vertex,
                    Bitangent
                )
            )
        );

        glBindVertexArray(0);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            0
        );
    }
};

#endif