#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <core/Model.h>



class AssimpLoader
{
    public:
        ModelConstructData LoadModel(const std::string& path);
    private:
        void processNode(aiNode* node, const aiScene* scene, ModelConstructData& data);
        void processMesh(aiMesh *mesh, const aiScene *scene, ModelConstructData& data);
        unsigned int LoadTexture(const std::string& path);
        unsigned int LoadTextureFromData(const void* data, int width, int height);
        Assimp::Importer importer;
        std::vector<Texture> texturesLoaded;
};