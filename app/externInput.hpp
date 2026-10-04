#pragma once
#include <resources/scene.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace loader
{
    struct loaderOutput
    {
        std::vector<engine::Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    class assimpLoader
    {
        public:
        static loaderOutput loadModel(const std::string& path);
        private:
        static void processMesh(aiMesh *mesh, aiNode* node, const aiScene *scene, loaderOutput& data);
        static void processNode(aiNode* node, const aiScene* scene, loaderOutput& data);
        inline static Assimp::Importer importer;
    };
}