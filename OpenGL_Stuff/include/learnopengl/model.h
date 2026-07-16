#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/stb_image.h>
#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// =========================================================
// DECLARACIÓN DE LA FUNCIÓN DE CARGA DE TEXTURAS
// =========================================================

inline unsigned int TextureFromFile(
    const char* path,
    const string& directory,
    bool gamma = false
);

// =========================================================
// CLASE MODEL
// =========================================================

class Model
{
public:

    // Texturas ya cargadas.
    vector<Texture> textures_loaded;

    // Mallas que componen el modelo.
    vector<Mesh> meshes;

    // Directorio del modelo.
    string directory;

    // Corrección gamma.
    bool gammaCorrection = false;

    // =====================================================
    // CONSTRUCTOR
    // =====================================================

    explicit Model(
        string const& path,
        bool gamma = false
    )
        : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // =====================================================
    // DIBUJAR EL MODELO
    // =====================================================

    void Draw(Shader& shader)
    {
        // -------------------------------------------------
        // Primera pasada: materiales opacos
        // -------------------------------------------------

        for (
            unsigned int i = 0;
            i < meshes.size();
            i++
        )
        {
            if (
                meshes[i].material.transparent
            )
            {
                continue;
            }

            meshes[i].Draw(shader);
        }

        // -------------------------------------------------
        // Segunda pasada: materiales transparentes
        // -------------------------------------------------

        /*
            Se mantiene activo el depth test, pero se
            desactiva temporalmente la escritura en el
            depth buffer.

            Esto evita que las superficies transparentes
            bloqueen incorrectamente otros fragmentos.
        */

        glDepthMask(GL_FALSE);

        for (
            unsigned int i = 0;
            i < meshes.size();
            i++
        )
        {
            if (
                !meshes[i].material.transparent
            )
            {
                continue;
            }

            meshes[i].Draw(shader);
        }

        glDepthMask(GL_TRUE);
    }

private:

    // =====================================================
    // CARGAR EL MODELO
    // =====================================================

    void loadModel(
        string const& path
    )
    {
        Assimp::Importer importer;

        const aiScene* scene =
            importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace
            );

        if (
            !scene ||
            (
                scene->mFlags &
                AI_SCENE_FLAGS_INCOMPLETE
            ) ||
            !scene->mRootNode
        )
        {
            cout
                << "ERROR::ASSIMP:: "
                << importer.GetErrorString()
                << endl;

            return;
        }

        const size_t separatorPosition =
            path.find_last_of(
                "/\\"
            );

        if (
            separatorPosition ==
            string::npos
        )
        {
            directory = ".";
        }
        else
        {
            directory =
                path.substr(
                    0,
                    separatorPosition
                );
        }

        processNode(
            scene->mRootNode,
            scene
        );
    }

    // =====================================================
    // PROCESAR LOS NODOS DEL MODELO
    // =====================================================

    void processNode(
        aiNode* node,
        const aiScene* scene
    )
    {
        for (
            unsigned int i = 0;
            i < node->mNumMeshes;
            i++
        )
        {
            aiMesh* mesh =
                scene->mMeshes[
                    node->mMeshes[i]
                ];

            meshes.push_back(
                processMesh(
                    mesh,
                    scene
                )
            );
        }

        for (
            unsigned int i = 0;
            i < node->mNumChildren;
            i++
        )
        {
            processNode(
                node->mChildren[i],
                scene
            );
        }
    }

    // =====================================================
    // PROCESAR UNA MALLA
    // =====================================================

    Mesh processMesh(
        aiMesh* mesh,
        const aiScene* scene
    )
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        MeshMaterial meshMaterial;

        // -------------------------------------------------
        // Procesar los vértices
        // -------------------------------------------------

        vertices.reserve(
            mesh->mNumVertices
        );

        for (
            unsigned int i = 0;
            i < mesh->mNumVertices;
            i++
        )
        {
            Vertex vertex;

            // Posición.
            vertex.Position =
                glm::vec3(
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                );

            // Normal.
            if (
                mesh->HasNormals()
            )
            {
                vertex.Normal =
                    glm::vec3(
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z
                    );
            }

            // Coordenadas de textura.
            if (
                mesh->mTextureCoords[0]
            )
            {
                vertex.TexCoords =
                    glm::vec2(
                        mesh->mTextureCoords[0][i].x,
                        mesh->mTextureCoords[0][i].y
                    );
            }

            // Tangentes y bitangentes.
            if (
                mesh->HasTangentsAndBitangents()
            )
            {
                vertex.Tangent =
                    glm::vec3(
                        mesh->mTangents[i].x,
                        mesh->mTangents[i].y,
                        mesh->mTangents[i].z
                    );

                vertex.Bitangent =
                    glm::vec3(
                        mesh->mBitangents[i].x,
                        mesh->mBitangents[i].y,
                        mesh->mBitangents[i].z
                    );
            }

            vertices.push_back(
                vertex
            );
        }

        // -------------------------------------------------
        // Procesar los índices
        // -------------------------------------------------

        for (
            unsigned int i = 0;
            i < mesh->mNumFaces;
            i++
        )
        {
            const aiFace& face =
                mesh->mFaces[i];

            for (
                unsigned int j = 0;
                j < face.mNumIndices;
                j++
            )
            {
                indices.push_back(
                    face.mIndices[j]
                );
            }
        }

        // -------------------------------------------------
        // Procesar el material
        // -------------------------------------------------

        aiMaterial* material =
            scene->mMaterials[
                mesh->mMaterialIndex
            ];

        aiColor3D diffuseColor(
            0.5f,
            0.5f,
            0.5f
        );

        aiColor3D specularColor(
            0.5f,
            0.5f,
            0.5f
        );

        float shininess = 32.0f;
        float opacity = 1.0f;

        aiString materialName;

        // Nombre del material.
        if (
            material->Get(
                AI_MATKEY_NAME,
                materialName
            ) == AI_SUCCESS
        )
        {
            meshMaterial.name =
                materialName.C_Str();
        }

        // Color difuso.
        if (
            material->Get(
                AI_MATKEY_COLOR_DIFFUSE,
                diffuseColor
            ) == AI_SUCCESS
        )
        {
            meshMaterial.diffuse =
                glm::vec3(
                    diffuseColor.r,
                    diffuseColor.g,
                    diffuseColor.b
                );
        }

        // Color especular.
        if (
            material->Get(
                AI_MATKEY_COLOR_SPECULAR,
                specularColor
            ) == AI_SUCCESS
        )
        {
            meshMaterial.specular =
                glm::vec3(
                    specularColor.r,
                    specularColor.g,
                    specularColor.b
                );
        }

        // Brillo.
        if (
            material->Get(
                AI_MATKEY_SHININESS,
                shininess
            ) == AI_SUCCESS
        )
        {
            meshMaterial.shininess =
                std::max(
                    shininess,
                    1.0f
                );
        }

        // Opacidad.
        if (
            material->Get(
                AI_MATKEY_OPACITY,
                opacity
            ) == AI_SUCCESS
        )
        {
            meshMaterial.opacity =
                glm::clamp(
                    opacity,
                    0.0f,
                    1.0f
                );
        }

        // -------------------------------------------------
        // Identificar materiales metálicos
        // -------------------------------------------------

        meshMaterial.metallic =
            (
                meshMaterial.name ==
                "car_paint"
            ) ||
            (
                meshMaterial.name ==
                "black_paint"
            ) ||
            (
                meshMaterial.name ==
                "chrome"
            ) ||
            (
                meshMaterial.name ==
                "darkchrome"
            ) ||
            (
                meshMaterial.name ==
                "aluminum"
            );

        // -------------------------------------------------
        // Identificar materiales transparentes
        // -------------------------------------------------

        meshMaterial.transparent =
            (
                meshMaterial.name ==
                "glass"
            ) ||
            (
                meshMaterial.opacity <
                0.999f
            );

        // -------------------------------------------------
        // Información para comprobar materiales
        // -------------------------------------------------

        std::cout
            << "Material importado: "
            << meshMaterial.name
            << " | Opacidad: "
            << meshMaterial.opacity
            << " | Brillo: "
            << meshMaterial.shininess
            << " | Metalico: "
            << (
                meshMaterial.metallic
                    ? "SI"
                    : "NO"
            )
            << " | Transparente: "
            << (
                meshMaterial.transparent
                    ? "SI"
                    : "NO"
            )
            << std::endl;

        // -------------------------------------------------
        // Cargar mapas difusos
        // -------------------------------------------------

        vector<Texture> diffuseMaps =
            loadMaterialTextures(
                material,
                aiTextureType_DIFFUSE,
                "texture_diffuse"
            );

        textures.insert(
            textures.end(),
            diffuseMaps.begin(),
            diffuseMaps.end()
        );

        // -------------------------------------------------
        // Cargar mapas especulares
        // -------------------------------------------------

        vector<Texture> specularMaps =
            loadMaterialTextures(
                material,
                aiTextureType_SPECULAR,
                "texture_specular"
            );

        textures.insert(
            textures.end(),
            specularMaps.begin(),
            specularMaps.end()
        );

        // -------------------------------------------------
        // Cargar mapas normales
        // -------------------------------------------------

        vector<Texture> normalMaps =
            loadMaterialTextures(
                material,
                aiTextureType_HEIGHT,
                "texture_normal"
            );

        textures.insert(
            textures.end(),
            normalMaps.begin(),
            normalMaps.end()
        );

        // -------------------------------------------------
        // Cargar mapas de altura
        // -------------------------------------------------

        vector<Texture> heightMaps =
            loadMaterialTextures(
                material,
                aiTextureType_AMBIENT,
                "texture_height"
            );

        textures.insert(
            textures.end(),
            heightMaps.begin(),
            heightMaps.end()
        );

        return Mesh(
            vertices,
            indices,
            textures,
            meshMaterial
        );
    }

    // =====================================================
    // CARGAR LAS TEXTURAS DEL MATERIAL
    // =====================================================

    vector<Texture> loadMaterialTextures(
        aiMaterial* material,
        aiTextureType type,
        string typeName
    )
    {
        vector<Texture> textures;

        for (
            unsigned int i = 0;
            i < material->GetTextureCount(type);
            i++
        )
        {
            aiString texturePath;

            material->GetTexture(
                type,
                i,
                &texturePath
            );

            bool skip = false;

            for (
                unsigned int j = 0;
                j < textures_loaded.size();
                j++
            )
            {
                if (
                    std::strcmp(
                        textures_loaded[j].path.c_str(),
                        texturePath.C_Str()
                    ) == 0
                )
                {
                    textures.push_back(
                        textures_loaded[j]
                    );

                    skip = true;
                    break;
                }
            }

            if (
                !skip
            )
            {
                Texture texture;

                texture.id =
                    TextureFromFile(
                        texturePath.C_Str(),
                        directory,
                        gammaCorrection
                    );

                texture.type =
                    typeName;

                texture.path =
                    texturePath.C_Str();

                /*
                    Solamente se registra la textura si
                    fue cargada correctamente.
                */
                if (
                    texture.id != 0
                )
                {
                    textures.push_back(
                        texture
                    );

                    textures_loaded.push_back(
                        texture
                    );
                }
            }
        }

        return textures;
    }
};

// =========================================================
// CARGAR UNA TEXTURA DESDE UN ARCHIVO
// =========================================================

inline unsigned int TextureFromFile(
    const char* path,
    const string& directory,
    bool gamma
)
{
    string filename =
        directory +
        "/" +
        string(path);

    // Convertir barras de Windows a barras estándar.
    std::replace(
        filename.begin(),
        filename.end(),
        '\\',
        '/'
    );

    unsigned int textureID = 0;

    glGenTextures(
        1,
        &textureID
    );

    int width = 0;
    int height = 0;
    int nrComponents = 0;

    unsigned char* data =
        stbi_load(
            filename.c_str(),
            &width,
            &height,
            &nrComponents,
            0
        );

    if (
        !data
    )
    {
        std::cout
            << "ERROR: No se pudo cargar la textura: "
            << filename
            << std::endl;

        glDeleteTextures(
            1,
            &textureID
        );

        return 0;
    }

    GLenum dataFormat = GL_RGBA;
    GLenum internalFormat = GL_RGBA;

    switch (
        nrComponents
    )
    {
        case 1:
        {
            dataFormat = GL_RED;
            internalFormat = GL_RED;
            break;
        }

        case 2:
        {
            dataFormat = GL_RG;
            internalFormat = GL_RG;
            break;
        }

        case 3:
        {
            dataFormat = GL_RGB;

            internalFormat =
                gamma
                    ? GL_SRGB
                    : GL_RGB;

            break;
        }

        case 4:
        {
            dataFormat = GL_RGBA;

            internalFormat =
                gamma
                    ? GL_SRGB_ALPHA
                    : GL_RGBA;

            break;
        }

        default:
        {
            std::cout
                << "ERROR: Cantidad de canales no soportada: "
                << nrComponents
                << " en "
                << filename
                << std::endl;

            stbi_image_free(
                data
            );

            glDeleteTextures(
                1,
                &textureID
            );

            return 0;
        }
    }

    glBindTexture(
        GL_TEXTURE_2D,
        textureID
    );

    /*
        Evita problemas cuando el ancho de la imagen no
        es múltiplo de cuatro bytes.
    */
    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(
        GL_TEXTURE_2D
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    // Restaurar la alineación predeterminada.
    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        4
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    stbi_image_free(
        data
    );

    return textureID;
}

#endif