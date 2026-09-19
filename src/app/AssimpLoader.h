#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <core/Model.h>


struct LoadSceneInfo
{
    uint32_t prevTextCount;
    vk::Device device;
    VmaAllocator alloc;
    vk::CommandPool cPool;
    vk::Queue queue;
};


class AssimpLoader
{
    public:
        ModelConstructData LoadModel(const std::string& path, const LoadSceneInfo& info, bool loadingFromEngScene = false);
    private:
        void LoadTextureType(aiTextureType textureType, TextureVk& out
        , const aiScene* scene,const aiMaterial* material, const LoadSceneInfo& info, ModelConstructData& data
        , Material& engineMat, uint32_t& updateIdx, std::string path);
        void processNode(aiNode* node, const aiScene* scene, ModelConstructData& data);
        void processMesh(aiMesh *mesh, aiNode* node, const aiScene *scene, ModelConstructData& data);
        void loadMaterials(const aiScene* scene, ModelConstructData& data, const LoadSceneInfo& info, std::string path, bool loadFromEngScene);
        glm::mat4 AiToGlm(const aiMatrix4x4& m);
        glm::mat4 getGlobalTransform(const aiNode* node);

        void addNodeToVec(const aiNode* node, std::vector<NodeData>& nodeData);
        uint32_t findNodeId(const std::string& name, const std::vector<NodeData>& nodeData);

        std::vector<NodeData> createNodeDataVec(const aiScene *scene);
        Assimp::Importer importer;
        std::vector<TextureVk> texturesLoaded;
};