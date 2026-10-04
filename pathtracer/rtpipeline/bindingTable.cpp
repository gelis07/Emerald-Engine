#include "bindingTable.hpp"


namespace engine
{
    void bindingTable::Init(core::context context
        , vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties, vk::Pipeline pip)
    {
        uint32_t baseAlign = rtProperties.shaderGroupBaseAlignment;
        uint32_t handleSize = rtProperties.shaderGroupHandleSize;

        const uint32_t shaderGroupCount = 3;
        vk::DeviceSize sbtBufferSize = baseAlign * shaderGroupCount;

        vk::BufferCreateInfo sbtBuffCi;
        sbtBuffCi.setUsage(vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setSize(sbtBufferSize);

        VmaAllocationCreateInfo sbtAllocCi{};
        sbtAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        sbtAllocCi.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        vmaCreateBufferWithAlignment(context.alloc.getAlloc(), reinterpret_cast<VkBufferCreateInfo*>(&sbtBuffCi), &sbtAllocCi
        , rtProperties.shaderGroupBaseAlignment, 
        reinterpret_cast<VkBuffer*>(&buffer.buffer), &buffer.allocation, &buffer.allocInfo);

        std::vector<uint8_t> handles = context.device.getDevice().getRayTracingShaderGroupHandlesKHR<uint8_t>(pip, 0 , shaderGroupCount,
        shaderGroupCount * handleSize, context.device.getLoader());

        vk::BufferDeviceAddressInfo sbtBufferAddressInfo{};
        sbtBufferAddressInfo.setBuffer(buffer.buffer);
        vk::DeviceAddress sbtAddress = context.device.getDevice().getBufferAddress(sbtBufferAddressInfo);

        vk::StridedDeviceAddressRegionKHR addressRegion;
        addressRegion.setStride(baseAlign)
        .setSize(baseAlign);

        mSbtRayGenAddressRegion = addressRegion;
        mSbtRayGenAddressRegion.setSize(baseAlign)
        .setDeviceAddress(sbtAddress);

        mSbtMissAddressRegion = addressRegion;
        mSbtMissAddressRegion.setDeviceAddress(sbtAddress + baseAlign);

        mSbtCHitAddresRegion = addressRegion;
        mSbtCHitAddresRegion.setDeviceAddress(sbtAddress + baseAlign * 2);
        uint8_t* sbtBufferData = static_cast<uint8_t*>(buffer.allocInfo.pMappedData);

        std::memcpy(sbtBufferData, handles.data(), handleSize);
        std::memcpy(sbtBufferData + baseAlign, handles.data() + handleSize, handleSize);
        std::memcpy(sbtBufferData + baseAlign * 2, handles.data() + handleSize * 2, handleSize);
    }
}