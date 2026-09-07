#include "VkEngineApiBinding.h"


void vkUtils::LoadTextures(Scene& scene, vkScene& vkscene)
{
    vkscene.vkTextures.clear();
    for(int i = 0; i < scene.textures.size(); i++)
    {
        vkscene.vkTextures.push_back(scene.textures[i]);
    }
}
vkUtils::vkScene vkUtils::LoadScene(VkContext context, vk::CommandPool cPool, Scene& scene)
{
    vkScene vkscene;


    evaluateBoneTransforms(scene, vkscene.bones);

    LoadTextures(scene, vkscene);
    uint32_t prevMeshSize = 0;
    uint32_t lastBoneInfCount = 0;
    for(int i = 0; i < scene.models.size(); i++)
    {
        VkModel vkmodel;
        vkmodel.nodeData = &scene.models[i].nodeData;
        vkmodel.modelMat = &scene.models[i].model;

        loadMeshes(context, i, prevMeshSize, scene.models[i].meshes, vkscene.vkMeshes
        , lastBoneInfCount, vkscene.boneInfluenceVec, vkscene.bones);

        vkscene.vkModels.push_back(vkmodel);
        prevMeshSize = scene.models[i].meshes.size();
    }

    if(vkscene.bones.empty())
        vkscene.bones.push_back({glm::mat4(1.0f)});
    if(vkscene.boneInfluenceVec.empty())
        vkscene.boneInfluenceVec.push_back({0, 0.0f});

    vk::BufferCreateInfo boneInfluenceBufferCi;
    boneInfluenceBufferCi.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
    .setSize(vkscene.boneInfluenceVec.size() * sizeof(BoneInfluece));

    vkscene.boneInfluenceBuffer.size = vkscene.boneInfluenceVec.size() * sizeof(BoneInfluece);

    vk::BufferCreateInfo boneDataBufferCi;
    boneDataBufferCi.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
    .setSize(vkscene.bones.size() * sizeof(vkBone));

    vkscene.boneTransforms.size = vkscene.bones.size() * sizeof(vkBone);

    VmaAllocationCreateInfo allocCi{};
    allocCi.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocCi.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(context.alloc, reinterpret_cast<VkBufferCreateInfo*>(&boneInfluenceBufferCi), &allocCi
    , reinterpret_cast<VkBuffer*>(&vkscene.boneInfluenceBuffer.buffer), &vkscene.boneInfluenceBuffer.allocation
    ,&vkscene.boneInfluenceBuffer.allocInfo);

    vmaCreateBuffer(context.alloc, reinterpret_cast<VkBufferCreateInfo*>(&boneDataBufferCi), &allocCi
    , reinterpret_cast<VkBuffer*>(&vkscene.boneTransforms.buffer), &vkscene.boneTransforms.allocation
    ,&vkscene.boneTransforms.allocInfo);

    std::memcpy(vkscene.boneInfluenceBuffer.allocInfo.pMappedData, vkscene.boneInfluenceVec.data(), vkscene.boneInfluenceBuffer.size);
    
    UpdateVertices(scene, vkscene);

    return vkscene;
}

void vkUtils::loadMeshes(VkContext context, uint32_t modelIdx
,uint32_t prevMeshSize, std::vector<Mesh>& meshes, std::vector<VkMesh>& vkMeshes
, uint32_t& lastBoneInfCount, std::vector<BoneInfluece>& boneInfluenceVec, std::vector<vkBone>& bones)
{
    for(int i = 0; i < meshes.size(); i++)
    {
        VkMesh vkMesh;

        vkMesh.matIdx = &meshes[i].matIndex;
        vkMesh.modelIdx = modelIdx;
        vkMesh.meshIdx = (modelIdx - 1) * prevMeshSize + i;
        vkMesh.nodeIdx = meshes[i].nodeId;
        vkMesh.localMeshIdx = i;

        for (auto el : meshes[i].boneInfluencesMap)
        {
            meshes[i].vertices[el.first].boneOffset = lastBoneInfCount;
            meshes[i].vertices[el.first].boneCount = el.second.size();
            for(int b = 0; b < el.second.size(); b++)
            {
                boneInfluenceVec.push_back({el.second[b].boneId, el.second[b].weight});
            }
            lastBoneInfCount += el.second.size();
        }
        
        vkMesh.vertCount = meshes[i].vertices.size();
        vkMesh.indexCount = meshes[i].indices.size();
        vkMesh.vBufSize = sizeof(Vertex) * meshes[i].vertices.size();
        vkMesh.iBufSize = sizeof(int) * meshes[i].indices.size();
        vk::BufferCreateInfo bufferCi;
        bufferCi.size = vkMesh.vBufSize + vkMesh.iBufSize;
        bufferCi.usage = vk::BufferUsageFlagBits::eVertexBuffer 
        | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
        | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eStorageBuffer;
        VmaAllocationCreateInfo vBufferAllocCi{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateBuffer(context.alloc, reinterpret_cast<VkBufferCreateInfo*>(&bufferCi), &vBufferAllocCi,
        reinterpret_cast<VkBuffer*>(&vkMesh.buffer), &vkMesh.bufferAllocation, &vkMesh.bufferAllocInfo);
        

        vk::BufferCreateInfo originalVertBufferCi;
        originalVertBufferCi.setSize(vkMesh.vBufSize)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

        vmaCreateBuffer(context.alloc, reinterpret_cast<VkBufferCreateInfo*>(&originalVertBufferCi), &vBufferAllocCi,
        reinterpret_cast<VkBuffer*>(&vkMesh.OriginalVertBuffer.buffer), &vkMesh.OriginalVertBuffer.allocation, &vkMesh.OriginalVertBuffer.allocInfo);

        vkMesh.OriginalVertBuffer.size = vkMesh.vBufSize;

        memcpy(vkMesh.bufferAllocInfo.pMappedData, meshes[i].vertices.data(), vkMesh.vBufSize);
        memcpy(vkMesh.OriginalVertBuffer.allocInfo.pMappedData, meshes[i].vertices.data(), vkMesh.OriginalVertBuffer.size);
        memcpy(((char*)vkMesh.bufferAllocInfo.pMappedData) + vkMesh.vBufSize, meshes[i].indices.data(), vkMesh.iBufSize);



        vkMeshes.push_back(vkMesh);
    }
}


glm::mat4 vkUtils::NodeHierarchyTransform(uint32_t nodeId, const std::vector<NodeData>& nodeData)
{
    glm::mat4 transform =  nodeData[nodeId].transform;
    uint32_t next = nodeData[nodeId].parentId;

    while(next != -1)
    {
        transform = nodeData[next].transform * transform;
        next = nodeData[next].parentId;
    }
    return transform;
}

void vkUtils::evaluateBoneTransforms(Scene& scene, std::vector<vkBone>& bones)
{
    for(int i = 0; i < scene.models.size(); i++)
    {
        for(int m = 0; m < scene.models[i].meshes.size(); m++)
        {
            Mesh& mesh = scene.models[i].meshes[m];

            for(int b = 0; b < mesh.bones.size(); b++)
            {
                vkBone bone;
                glm::mat4 mat = NodeHierarchyTransform(mesh.bones[b].nodeId, scene.models[i].nodeData);
                bone.transform =  mat * mesh.bones[b].offset;
                bones.push_back(bone);
            }
        }
    }
}

void vkUtils::UpdateVertices(Scene& scene, vkScene& vkscene)
{
    uint32_t offset = 0;
    for(int i = 0; i < scene.models.size(); i++)
    {
        for(int m = 0; m < scene.models[i].meshes.size(); m++)
        {
            Mesh& mesh = scene.models[i].meshes[m];
            std::vector<Vertex> newVertices;
            newVertices.resize(mesh.vertices.size());
            for(int v = 0; v < mesh.vertices.size(); v++)
            {
                Vertex& vertex = mesh.vertices[v];
                newVertices[v] = vertex;
                glm::vec3 newPos(0.0f);
                if(vertex.boneCount != 0)
                {
                    for(int b = vertex.boneOffset; b < vertex.boneOffset + vertex.boneCount; b++)
                    {
                        glm::mat4 transform = vkscene.bones[vkscene.boneInfluenceVec[b].boneId].transform;
                        float weight = vkscene.boneInfluenceVec[b].weight;
                        newPos += glm::vec3(weight * transform * glm::vec4(vertex.position, 1.0f));
                    }
                    newVertices[v].position = newPos;
                }
            }

            memcpy(vkscene.vkMeshes[offset + m].bufferAllocInfo.pMappedData, newVertices.data(), vkscene.vkMeshes[offset + m].vBufSize);
        }
        offset += scene.models[i].meshes.size();
    }
}