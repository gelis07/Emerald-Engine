#include "blas.hpp"
namespace engine
{
    void BLAS::Init(core::context context, vk::CommandPool commandPool
    , vk::Buffer buffer, uint32_t vSize, uint32_t iCount, uint32_t vCount)
    {
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProperties;
        vk::PhysicalDeviceProperties2 properties;
        properties.pNext = &asProperties;

        context.device.getPhysicalDevice().getProperties2(&properties);

        vk::AccelerationStructureGeometryKHR geometry{};
        geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        vk::BufferDeviceAddressInfo buffAddInfo;
        buffAddInfo.setBuffer(buffer);
        vk::DeviceAddress buffAddress = context.device.getDevice().getBufferAddress(buffAddInfo);
        
        geometry.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        geometry.geometry.triangles.setVertexData(buffAddress)
        .setIndexData(buffAddress + vSize * vCount)
        .setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setMaxVertex(vCount)
        .setVertexStride(vSize)
        .setIndexType(vk::IndexType::eUint32);

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace
        | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setSrcAccelerationStructure(nullptr)
        .setDstAccelerationStructure(nullptr)
        .setGeometryCount(1)
        .setPGeometries(&geometry)
        .setScratchData({});

        uint32_t primCount = iCount / 3;
        vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = context.device.getDevice().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfo, primCount, context.device.getLoader());

        vk::BufferCreateInfo structScratchBufferCi;
        structScratchBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setSize(buildSizesInfo.accelerationStructureSize);

        VmaAllocationCreateInfo structBufferAllocInfo{};
        structBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        structBufferAllocInfo.flags = 0;

        vmaCreateBufferWithAlignment(context.alloc.getAlloc()
        , reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi)
        , &structBufferAllocInfo
        , asProperties.minAccelerationStructureScratchOffsetAlignment
        , reinterpret_cast<VkBuffer*>(&blas.StructureBuffer.buffer)
        , &blas.StructureBuffer.allocation
        , nullptr);
        
        structScratchBufferCi.setSize(buildSizesInfo.buildScratchSize);
        structScratchBufferCi.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        vmaCreateBufferWithAlignment(context.alloc.getAlloc(), reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi)
        , &structBufferAllocInfo, asProperties.minAccelerationStructureScratchOffsetAlignment, reinterpret_cast<VkBuffer*>(&blas.ScratchBuffer.buffer),
        &blas.ScratchBuffer.allocation, nullptr);

        vk::AccelerationStructureCreateInfoKHR accCi;
        accCi.setBuffer(blas.StructureBuffer.buffer)
        .setOffset(0)
        .setSize(buildSizesInfo.accelerationStructureSize)
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

        blas.accel = context.device.getDevice().createAccelerationStructureKHR(accCi, nullptr, context.device.getLoader());

        buildInfo.dstAccelerationStructure = blas.accel;
        vk::BufferDeviceAddressInfo scratchDeviceAddressInfo;
        scratchDeviceAddressInfo.setBuffer(blas.ScratchBuffer.buffer);
        buildInfo.scratchData.deviceAddress = context.device.getDevice().getBufferAddress(scratchDeviceAddressInfo);

        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
        buildRangeInfo.setPrimitiveCount(primCount)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};

        core::ExecuteSingleTimeCb(context.device.getDevice(), commandPool, context.device.getQueue(), [&](const vk::CommandBuffer& singleTimeCb){
            singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, context.device.getLoader());
        });
    }
}