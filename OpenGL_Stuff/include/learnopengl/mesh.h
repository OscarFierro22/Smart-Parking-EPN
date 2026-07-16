#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader.h>

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

struct Vertex {
    // position
    glm::vec3 Position{0.0f};
    // normal
    glm::vec3 Normal{0.0f};
    // texCoords
    glm::vec2 TexCoords{0.0f};
    // tangent
    glm::vec3 Tangent{0.0f};
    // bitangent
    glm::vec3 Bitangent{0.0f};
};

struct Texture {
    unsigned int id = 0;
    string type{};
    string path{};
};

struct MeshMaterial
{
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{0.5f};
    glm::vec3 emissive{0.0f};

    float shininess = 32.0f;
    float opacity = 1.0f;

    /*
        Nombre del material leído desde el archivo MTL.

        Ejemplos:
        car_paint
        glass
        chrome
        leather
    */
    std::string name;

    /*
        Indica si el material representa la pintura
        metálica principal del automóvil.
    */
    bool metallic = false;

    /*
        Indica si la malla debe renderizarse en la
        segunda pasada de transparencia.
    */
    bool transparent = false;
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    MeshMaterial 	 material;
    unsigned int VAO;

    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, MeshMaterial material)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
	this->material = material;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader& shader)
{
    // =====================================================
    // 1. ENVIAR PROPIEDADES DEL MATERIAL AL SHADER
    // =====================================================

    shader.setVec3(
        "materialDiffuse",
        material.diffuse
    );

    shader.setVec3(
        "materialSpecular",
        material.specular
    );

    shader.setVec3(
        "materialEmissive",
        material.emissive
    );

    /*
        Blender puede exportar valores Ns de hasta 1000.

        El shader acepta ese valor directamente, aunque
        establecemos un mínimo para evitar resultados
        extraños con shininess igual a cero.
    */
    shader.setFloat(
        "materialShininess",
        std::max(
            material.shininess,
            1.0f
        )
    );

    /*
        Este era el valor que faltaba enviar.

        Para el material glass llegará aproximadamente:
        0.150858

        Para los materiales opacos:
        1.0
    */
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

    // =====================================================
    // 2. VINCULAR LAS TEXTURAS
    // =====================================================

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
        string name =
            textures[i].type;

        if (
            name ==
            "texture_diffuse"
        )
        {
            number =
                std::to_string(
                    diffuseNr++
                );
        }
        else if (
            name ==
            "texture_specular"
        )
        {
            number =
                std::to_string(
                    specularNr++
                );
        }
        else if (
            name ==
            "texture_normal"
        )
        {
            number =
                std::to_string(
                    normalNr++
                );
        }
        else if (
            name ==
            "texture_height"
        )
        {
            number =
                std::to_string(
                    heightNr++
                );
        }

        glUniform1i(
            glGetUniformLocation(
                shader.ID,
                (
                    name +
                    number
                ).c_str()
            ),
            i
        );

        glBindTexture(
            GL_TEXTURE_2D,
            textures[i].id
        );
    }

    // =====================================================
    // 3. DIBUJAR LA MALLA
    // =====================================================

    glBindVertexArray(VAO);

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(
            indices.size()
        ),
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);

    // =====================================================
    // 4. RESTAURAR UNIDAD DE TEXTURA
    // =====================================================

    glActiveTexture(
        GL_TEXTURE0
    );
}




private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }
};
#endif