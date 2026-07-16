#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader)
{
    // =====================================================
    // PRIMERA PASADA: MATERIALES OPACOS
    // =====================================================

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

    // =====================================================
    // SEGUNDA PASADA: MATERIALES TRANSPARENTES
    // =====================================================

    /*
        Los objetos transparentes se dibujan después
        de los objetos opacos.

        El depth test sigue activo, pero desactivamos
        temporalmente la escritura en el depth buffer.
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
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

	MeshMaterial meshMaterial;

/*
    Valores predeterminados.

    Es importante inicializar opacity porque antes podía
    quedar con basura de memoria si Assimp no encontraba
    AI_MATKEY_OPACITY.
*/
meshMaterial.diffuse =
    glm::vec3(0.5f);

meshMaterial.specular =
    glm::vec3(0.5f);

meshMaterial.emissive =
    glm::vec3(0.0f);

meshMaterial.shininess =
    32.0f;

meshMaterial.opacity =
    1.0f;

meshMaterial.name =
    "";

meshMaterial.metallic =
    false;

meshMaterial.transparent =
    false;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        

// =========================================================
// PROCESAR EL MATERIAL DEL ARCHIVO MTL
// =========================================================

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

aiColor3D emissiveColor(
    0.0f,
    0.0f,
    0.0f
);

float shininess =
    32.0f;

float opacity =
    1.0f;

aiString materialName;

// =========================================================
// NOMBRE DEL MATERIAL
// =========================================================

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

// =========================================================
// COLOR DIFUSO
// =========================================================

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

// =========================================================
// COLOR ESPECULAR
// =========================================================

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

// =========================================================
// COLOR EMISIVO KE
// =========================================================

if (
    material->Get(
        AI_MATKEY_COLOR_EMISSIVE,
        emissiveColor
    ) == AI_SUCCESS
)
{
    meshMaterial.emissive =
        glm::vec3(
            emissiveColor.r,
            emissiveColor.g,
            emissiveColor.b
        );
}

// =========================================================
// BRILLO NS
// =========================================================

if (
    material->Get(
        AI_MATKEY_SHININESS,
        shininess
    ) == AI_SUCCESS
)
{
    meshMaterial.shininess =
        shininess;
}

// =========================================================
// OPACIDAD D
// =========================================================

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

// =========================================================
// IDENTIFICACIÓN DE MATERIALES ESPECIALES
// =========================================================

/*
    En tu archivo Body.mtl los materiales tienen nombres:

    car_paint
    black_paint
    glass
    chrome
    darkchrome
    redglass
    etc.
*/

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

/*
    El material glass tiene d = 0.150858.

    Utilizamos el nombre y la opacidad para detectar
    materiales transparentes.

    redglass tiene d = 1.0, por lo tanto seguirá opaco.
*/
meshMaterial.transparent =
    (
        meshMaterial.name ==
        "glass"
    ) ||
    (
        meshMaterial.opacity <
        0.999f
    );

// Información temporal para comprobar la importación.
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


    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
	return Mesh(vertices, indices, textures, meshMaterial);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};

inline unsigned int TextureFromFile(
    const char* path,
    const string& directory,
    bool gamma
)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width = 0;
    int height = 0;
    int nrComponents = 0;

    unsigned char* data = stbi_load(
        filename.c_str(),
        &width,
        &height,
        &nrComponents,
        0
    );

    if (data)
    {
        GLenum dataFormat = GL_RGBA;
        GLenum internalFormat = GL_RGBA;

        if (nrComponents == 1)
        {
            dataFormat = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrComponents == 2)
        {
            // Imagen de dos canales: normalmente gris + alfa.
            dataFormat = GL_RG;
            internalFormat = GL_RG;
        }
        else if (nrComponents == 3)
        {
            dataFormat = GL_RGB;
            internalFormat = gamma
                ? GL_SRGB
                : GL_RGB;
        }
        else if (nrComponents == 4)
        {
            dataFormat = GL_RGBA;
            internalFormat = gamma
                ? GL_SRGB_ALPHA
                : GL_RGBA;
        }
        else
        {
            std::cout
                << "ERROR: Cantidad de canales no soportada: "
                << nrComponents
                << " en "
                << filename
                << std::endl;

            stbi_image_free(data);
            glDeleteTextures(1, &textureID);

            return 0;
        }

        glBindTexture(
            GL_TEXTURE_2D,
            textureID
        );

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

        glPixelStorei(
            GL_UNPACK_ALIGNMENT,
            4
        );

        glBindTexture(
            GL_TEXTURE_2D,
            0
        );

        stbi_image_free(data);
    }
    else
    {
        std::cout
            << "ERROR: No se pudo cargar la textura completa: "
            << filename
            << std::endl;

        glDeleteTextures(
            1,
            &textureID
        );

        return 0;
    }

    return textureID;
}

#endif