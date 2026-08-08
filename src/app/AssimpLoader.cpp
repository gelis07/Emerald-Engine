#include "AssimpLoader.h"
#include "fmt/base.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stbi_write.h>
#include <core/Utils.h>
#include <vkEngine/vkBackend.h>




ModelConstructData AssimpLoader::LoadModel(const std::string& path, const LoadSceneInfo& info)
{
    texturesLoaded.clear();
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    ModelConstructData data;
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        fmt::println("Assimp error: {}", importer.GetErrorString());
        return data;
    }

    loadMaterials(scene, data, info);

    data.path = path;
    processNode(scene->mRootNode, scene, data);
    return data;
}


void AssimpLoader::loadMaterials(const aiScene* scene, ModelConstructData& data, const LoadSceneInfo& info)
{
    for(int mat = 0; mat < scene->mNumMaterials; mat++)
    {
        Material engineMat;
        TextureVk newTexture;
        aiMaterial *material = scene->mMaterials[mat];

        engineMat.albedoTexture = -1;

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
                VkImageCreateData imgCreateData;
                imgCreateData.allocator = info.alloc;
                imgCreateData.commandPool = info.cPool;
                imgCreateData.device = info.device;
                imgCreateData.queue = info.queue;

                newTexture = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                data.textureData.push_back(newTexture);
                engineMat.albedoTexture = data.textureData.size() - 1;

                stbi_image_free(texData);
                continue;
            }
            bool skip = false;
            for (int j = 0; j < texturesLoaded.size(); j++) 
            {
                if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    newTexture = texturesLoaded[j];
                    skip = true;
                    break;
                }
            }
            if(!skip)
            {
                int width, height, channels;
                unsigned char* texData = stbi_load(str.C_Str(), &width, &height, &channels, 4);
                VkImageCreateData imgCreateData;
                imgCreateData.allocator = info.alloc;
                imgCreateData.commandPool = info.cPool;
                imgCreateData.device = info.device;
                imgCreateData.queue = info.queue;
                newTexture = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                texturesLoaded.push_back(newTexture);
                stbi_image_free(texData);
            }
            
            data.textureData.push_back(newTexture);
            engineMat.albedoTexture = data.textureData.size() - 1;

        }
        aiColor3D albedo;
        aiColor3D emmColor;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emmColor);
        engineMat.albedo = glm::vec3(albedo.r, albedo.g, albedo.b);
        engineMat.emmColor = glm::vec3(emmColor.r, emmColor.g, emmColor.b);
        
        
        data.materials.push_back(engineMat);
    }
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
    
    newMesh.matIndex = mesh->mMaterialIndex;
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec4 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
        }
        else
            vertex.texCoords = glm::vec2(-1.0f, -1.0f);
        

        vertex.normals.x = mesh->mNormals[i].x;
        vertex.normals.y = mesh->mNormals[i].y;
        vertex.normals.z = mesh->mNormals[i].z;
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
