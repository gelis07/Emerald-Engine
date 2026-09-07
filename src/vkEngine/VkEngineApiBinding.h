#pragma once
#include "core/Scene.h"
#include "vkBackend.h"
#include "stb_image.h"


namespace vkUtils
{
    struct vkBone
    {
        glm::mat4 transform;
    };

    struct VkModel
    {
        const std::vector<NodeData>* nodeData;
        glm::mat4* modelMat; 
    };

    struct VkMesh
    {
        vk::Buffer buffer;
        vk::DeviceSize vBufSize, iBufSize, indexCount, vertCount;
        VmaAllocationInfo bufferAllocInfo{};
        VmaAllocation bufferAllocation{};

        VkUtilBuffer OriginalVertBuffer;
        
        uint32_t meshIdx; // In certain structures a mesh is indipendent of the model.
        uint32_t localMeshIdx;
        uint32_t nodeIdx;
        uint32_t modelIdx;
        uint32_t* matIdx;
    };
    struct finalNodeData
    {
        glm::mat4 transform;
        glm::mat3 normalMatrix;
    };

    struct vkScene
    {
        std::vector<VkModel> vkModels;
        std::vector<VkMesh> vkMeshes;
        std::vector<TextureVk> vkTextures;
        std::vector<BoneInfluece> boneInfluenceVec;
        std::vector<vkBone> bones;

        VkUtilBuffer boneTransforms;
        VkUtilBuffer boneInfluenceBuffer;
    };

    [[nodiscard]]vkScene LoadScene(VkContext context, vk::CommandPool cPool, Scene& scene);

    void LoadTextures(Scene& scene, vkScene& vkscene);
    glm::mat4 NodeHierarchyTransform(uint32_t nodeId, const std::vector<NodeData>& nodeData);
    void evaluateBoneTransforms(Scene& scene, std::vector<vkBone>& bones);
    void UpdateVertices(Scene& scene, vkScene& vkscene);


    void loadMeshes(VkContext context, uint32_t modelIdx
    ,uint32_t prevMeshSize, std::vector<Mesh>& meshes, std::vector<VkMesh>& vkMeshes, uint32_t& lastBoneInfCount
    ,std::vector<BoneInfluece>& boneInfluenceVec, std::vector<vkBone>& bones);
}