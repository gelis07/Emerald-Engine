#include "externInput.hpp"
#include <utils.hpp>

namespace loader
{
    void assimpLoader::processMesh(aiMesh *mesh, aiNode* node, const aiScene *scene, loaderOutput& data)
    {
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            engine::Vertex vertex;
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
            
            data.vertices.push_back(vertex);
        }
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
            {
                data.indices.push_back(face.mIndices[j]);
            }
        }
    }
    void assimpLoader::processNode(aiNode* node, const aiScene* scene, loaderOutput& data)
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

    loaderOutput assimpLoader::loadModel(const std::string& path)
    {
        loaderOutput output;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            CORE_PRINT("Can't load scene with path {} \n Assimp error: {}", path, importer.GetErrorString());
        }

        processNode(scene->mRootNode, scene, output);
        return output;
    }
}