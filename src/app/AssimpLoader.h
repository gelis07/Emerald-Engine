#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <core/Model.h>


struct LoadSceneInfo
{
    vk::Device device;
    VmaAllocator alloc;
    vk::CommandPool cPool;
    vk::Queue queue;
};


class AssimpLoader
{
    public:
        ModelConstructData LoadModel(const std::string& path, const LoadSceneInfo& info);
    private:
        void processNode(aiNode* node, const aiScene* scene, ModelConstructData& data);
        void processMesh(aiMesh *mesh, const aiScene *scene, ModelConstructData& data);
        void loadMaterials(const aiScene* scene, ModelConstructData& data, const LoadSceneInfo& info);

        Assimp::Importer importer;
        std::vector<TextureVk> texturesLoaded;
};