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
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

    ModelConstructData data;
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        fmt::println("Assimp error: {}", importer.GetErrorString());
        return data;
    }



    loadMaterials(scene, data, info);

    data.nodeData = createNodeDataVec(scene);

    data.path = path;
    processNode(scene->mRootNode, scene, data);
    return data;
}


void AssimpLoader::loadMaterials(const aiScene* scene, ModelConstructData& data, const LoadSceneInfo& info)
{
    for(int mat = 0; mat < scene->mNumMaterials; mat++)
    {
        Material engineMat;
        TextureVk newTextureAlbedo;
        TextureVk newTextureRoughness;
        TextureVk newTextureMetalness;
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

                newTextureAlbedo = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                data.textureData.push_back(newTextureAlbedo);
                engineMat.albedoTexture = info.prevTextCount + data.textureData.size() - 1;

                stbi_image_free(texData);
                continue;
            }
            bool skip = false;
            for (int j = 0; j < texturesLoaded.size(); j++) 
            {
                if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    newTextureAlbedo = texturesLoaded[j];
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
                newTextureAlbedo = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                texturesLoaded.push_back(newTextureAlbedo);
                stbi_image_free(texData);
            }
            
            data.textureData.push_back(newTextureAlbedo);
            engineMat.albedoTexture = info.prevTextCount + data.textureData.size() - 1;

        }
        for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS); i++)
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
                imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
                newTextureRoughness = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                data.textureData.push_back(newTextureRoughness);
                engineMat.albedoTexture = info.prevTextCount + data.textureData.size() - 1;

                stbi_image_free(texData);
                continue;
            }
            bool skip = false;
            for (int j = 0; j < texturesLoaded.size(); j++) 
            {
                if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    newTextureRoughness = texturesLoaded[j];
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
                imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
                newTextureRoughness = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                texturesLoaded.push_back(newTextureRoughness);
                stbi_image_free(texData);
            }
            
            data.textureData.push_back(newTextureRoughness);
            engineMat.roughnessTexture = info.prevTextCount + data.textureData.size() - 1;
        }
        for (int i = 0; i < material->GetTextureCount(aiTextureType_METALNESS); i++)
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
                imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
                newTextureMetalness = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                data.textureData.push_back(newTextureMetalness);
                engineMat.albedoTexture = info.prevTextCount + data.textureData.size() - 1;

                stbi_image_free(texData);
                continue;
            }
            bool skip = false;
            for (int j = 0; j < texturesLoaded.size(); j++) 
            {
                if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    newTextureMetalness = texturesLoaded[j];
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
                imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
                newTextureMetalness = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                texturesLoaded.push_back(newTextureMetalness);
                stbi_image_free(texData);
            }
            
            data.textureData.push_back(newTextureMetalness);
            engineMat.metallicnesTexture = info.prevTextCount + data.textureData.size() - 1;
        }
        aiColor3D albedo;
        aiColor3D emmColor;
        float metalness;
        float roughness;
        float brightness;
        material->Get(AI_MATKEY_EMISSIVE_INTENSITY, brightness);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emmColor);
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
        engineMat.albedo = glm::vec3(albedo.r, albedo.g, albedo.b);
        engineMat.metalness = metalness;
        engineMat.roughness = roughness;
        // engineMat.emmColor = glm::vec3(emmColor.r, emmColor.g, emmColor.b) * brightness;
        
        data.materials.push_back(engineMat);
    }
}



void AssimpLoader::processNode(aiNode* node, const aiScene* scene, ModelConstructData& data)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        processMesh(scene->mMeshes[node->mMeshes[i]], node, scene, data);
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, data);
    }
}
glm::mat4 AssimpLoader::AiToGlm(const aiMatrix4x4& m)
{
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
void AssimpLoader::processMesh(aiMesh *mesh, aiNode* node, const aiScene *scene, ModelConstructData& data)
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

        vertex.tangent.x = mesh->mTangents[i].x;
        vertex.tangent.y = mesh->mTangents[i].y;
        vertex.tangent.z = mesh->mTangents[i].z;

        vertex.bitangent.x = mesh->mBitangents[i].x;
        vertex.bitangent.y = mesh->mBitangents[i].y;
        vertex.bitangent.z = mesh->mBitangents[i].z;
        
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
    newMesh.nodeId = findNodeId(node->mName.C_Str(), data.nodeData);
    for (uint32_t i = 0; i < mesh->mNumBones; i++)
    {
        Bone bone;
        aiBone* aiBone = mesh->mBones[i];
        bone.offset = AiToGlm(mesh->mBones[i]->mOffsetMatrix);

        const aiNode* boneNode = scene->mRootNode->findBoneNode(mesh->mBones[i]);
        bone.nodeId = findNodeId(boneNode->mName.C_Str(), data.nodeData);
        bone.name = boneNode->mName.C_Str();
        newMesh.bones.push_back(bone);
        for (uint32_t j = 0; j < aiBone->mNumWeights; j++)
        {
            uint32_t vertexId = aiBone->mWeights[j].mVertexId;
            float weight = aiBone->mWeights[j].mWeight;

            newMesh.boneInfluencesMap[vertexId].push_back({i, weight});
        }
    }

    data.meshes.push_back(newMesh);
}
glm::mat4 AssimpLoader::getGlobalTransform(const aiNode* node)
{
    glm::mat4 transform = AiToGlm(node->mTransformation);

    while (node->mParent)
    {
        node = node->mParent;

        transform = AiToGlm(node->mTransformation) * transform;
    }

    return transform;
}

void AssimpLoader::addNodeToVec(const aiNode* node, std::vector<NodeData>& nodeData)
{
    glm::mat4 mat = AiToGlm(node->mTransformation);
    std::string parentName;
    if(node->mParent)
        parentName = node->mParent->mName.C_Str();
    else
        parentName = ROOT_NODE;

    nodeData.push_back({mat, UINT32_MAX, parentName, node->mName.C_Str()});
    for(int i = 0; i < node->mNumChildren; i++)
    {
        addNodeToVec(node->mChildren[i], nodeData);
    }
}


uint32_t AssimpLoader::findNodeId(const std::string& name, const std::vector<NodeData>& nodeData)
{
    if(name == ROOT_NODE)
    {
        return -1;
    }

    for(int i = 0; i < nodeData.size(); i++)
    {
        if(name == nodeData[i].name)
        {
            return i;
        }
    }
    return -1;
}

std::vector<NodeData> AssimpLoader::createNodeDataVec(const aiScene *scene)
{
    std::vector<NodeData> nodeData;

    const aiNode* node = scene->mRootNode;

    addNodeToVec(node, nodeData);
    
    for(int i = 0; i < nodeData.size(); i++)
    {
        nodeData[i].parentId = findNodeId(nodeData[i].parentName, nodeData);
    }

    return nodeData;
}
