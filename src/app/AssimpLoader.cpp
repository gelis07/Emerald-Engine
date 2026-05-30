#include "AssimpLoader.h"
#include "fmt/base.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


ModelConstructData AssimpLoader::LoadModel(const std::string& path)
{
    texturesLoaded.clear();
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    ModelConstructData data;
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        fmt::println("Assimp error: {}", importer.GetErrorString());
        return data;
    }
    data.path = path;
    processNode(scene->mRootNode, scene, data);
    return data;
}

void AssimpLoader::processNode(aiNode* node, const aiScene* scene, ModelConstructData& data)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        processMesh(scene->mMeshes[node->mMeshes[i]], scene, data);
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, data);
    }
}

void AssimpLoader::processMesh(aiMesh *mesh, const aiScene *scene, ModelConstructData& data)
{
    Mesh newMesh;
    for(int mat = 0; mat < scene->mNumMaterials; mat++)
    {
        aiMaterial *material = scene->mMaterials[mat];
        for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
        {
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, i, &str);
            const aiTexture* embTexture = scene->GetEmbeddedTexture(str.C_Str());
            if(embTexture != nullptr)
            {
                int width, height, channels;
                unsigned char* texData = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(embTexture->pcData),
                    embTexture->mWidth,
                    &width,
                    &height,
                    &channels,
                    4 // force RGBA
                );
                newMesh.texture.id = LoadTextureFromData(texData, width, height);
                newMesh.texture.path = str.C_Str();
                continue;
            }
            bool skip = false;
            for (int j = 0; j < texturesLoaded.size(); j++) 
            {
                if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    newMesh.texture = texturesLoaded[j];
                    skip = true;
                    break;
                }
            }
            if(!skip)
            {
                Texture texture;
                texture.path = std::string(str.C_Str()); 
                texture.id = LoadTexture(texture.path);
                newMesh.texture = texture;
                texturesLoaded.push_back(texture);
            }
        }
    }  
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
        }
        else
            vertex.texCoords = glm::vec2(-1.0f, -1.0f);
        
        newMesh.vertices.push_back(vertex);
    }
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
        {
            newMesh.indices.push_back(face.mIndices[j]);
        }
    }  
    data.meshes.push_back(newMesh);
}

unsigned int AssimpLoader::LoadTexture(const std::string& path)
{
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0); 

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,0, GL_RGB, GL_UNSIGNED_BYTE, data);

    return texture;
}
unsigned int AssimpLoader::LoadTextureFromData(const void* data, int width, int height)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return texture;
}