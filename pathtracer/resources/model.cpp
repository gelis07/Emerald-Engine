#include "model.hpp"

namespace engine
{
    void Model::InitBuffer(core::context context,const std::vector<Vertex> vertices, const std::vector<uint32_t> indices)
    {
        vCount = vertices.size();
        iCount = indices.size();
        vSize = sizeof(Vertex) * vCount;
        iSize = sizeof(uint32_t) * iCount;
        vk::BufferCreateInfo bufferCi;
        bufferCi.size = vSize + iSize;
        bufferCi.usage = vk::BufferUsageFlagBits::eVertexBuffer 
        | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
        | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eStorageBuffer;
        VmaAllocationCreateInfo vBufferAllocCi{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateBuffer(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&bufferCi),
        &vBufferAllocCi,
        reinterpret_cast<VkBuffer*>(&vertBuffer.buffer),
        &vertBuffer.allocation,
        &vertBuffer.allocInfo);

        std::memcpy(vertBuffer.allocInfo.pMappedData, vertices.data(), vSize); 
        std::memcpy((char*)vertBuffer.allocInfo.pMappedData + vSize, indices.data(), iSize); 
    }
}