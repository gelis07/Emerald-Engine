#pragma once
#include "core/Scene.h"
#include "vkBackend.h"
#include "stb_image.h"


namespace vkUtils
{
    struct vkModel
    {
        vk::Buffer buffer;
        vk::DeviceSize vBufSize, iBufSize, indexCount, vertCount;
        VmaAllocationInfo bufferAllocInfo{};
        VmaAllocation bufferAllocation{};
        uint32_t matIdx;

        Model* model;
    };


    struct vkScene
    {
        std::vector<vkModel> vkModels;
        std::vector<TextureVk> vkTextures;
    };


    [[nodiscard]]inline static vkScene LoadScene(const vk::Device& device, const VmaAllocator& alloc, vk::CommandPool cPool, 
       vk::Queue queue ,Scene& scene)
    {
        vkScene vkscene;

        std::vector<vkModel> vkModels;
        std::vector<TextureVk> vkTextures;
        const std::vector<Model>& models = scene.models;

        for(int i = 0; i < scene.textures.size(); i++)
        {
            vkTextures.push_back(scene.textures[i]);
        }

        for(int i = 0; i < models.size(); i++)
        {
            const Model& model = models[i];
            for(const Mesh& mesh : model.mMeshes)
            {
                vkModel vkModel;

                std::vector<Vertex> vertices;
                std::vector<int> indices;
                uint32_t vertCount = mesh.vertices.size();
                uint32_t indexCount = mesh.indices.size();

                vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
                indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());

                
                vkModel.vertCount = vertCount;
                vkModel.indexCount = indexCount;
                vkModel.vBufSize = sizeof(Vertex) * vertCount;
                vkModel.iBufSize = sizeof(int) * indexCount;
                vkModel.model = &scene.models[i];
                
                vk::BufferCreateInfo bufferCi;
                bufferCi.size = vkModel.vBufSize + vkModel.iBufSize;
                bufferCi.usage = vk::BufferUsageFlagBits::eVertexBuffer 
                | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eStorageBuffer;
                
                VmaAllocationCreateInfo vBufferAllocCi{
                    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    .usage = VMA_MEMORY_USAGE_AUTO
                };
                vmaCreateBuffer(alloc, reinterpret_cast<VkBufferCreateInfo*>(&bufferCi), &vBufferAllocCi,
                reinterpret_cast<VkBuffer*>(&vkModel.buffer), &vkModel.bufferAllocation, &vkModel.bufferAllocInfo);
                
                memcpy(vkModel.bufferAllocInfo.pMappedData, vertices.data(), vkModel.vBufSize);
                memcpy(((char*)vkModel.bufferAllocInfo.pMappedData) + vkModel.vBufSize, indices.data(), vkModel.iBufSize);
                
                vkModel.matIdx = mesh.matIndex;
                
                vkModels.push_back(vkModel);
            }
        }
        vkscene.vkTextures = vkTextures;
        vkscene.vkModels = vkModels;
        return vkscene;
    }
}