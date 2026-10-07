#include "tlas.hpp"

namespace engine
{
    void TLAS::Init(core::context context,
    vk::CommandPool commandPool, const std::vector<core::BottomAccStructTlasRef> bottomAccel)
    {
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProperties;
        vk::PhysicalDeviceProperties2 properties;
        properties.pNext = &asProperties;

        context.device.getPhysicalDevice().getProperties2(&properties);

        vk::AccelerationStructureGeometryKHR geometry{};
        geometry.setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        geometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
        geometry.geometry.instances.arrayOfPointers = false;

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | 
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setSrcAccelerationStructure(nullptr)
        .setDstAccelerationStructure(nullptr)
        .setGeometryCount(1)
        .setPGeometries(&geometry)
        .setScratchData({});

        vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
         context.device.getDevice().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, 
            {static_cast<uint32_t>(bottomAccel.size())}, 
            context.device.getLoader());
        
        vk::BufferCreateInfo structScratchBufferCi;
        structScratchBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR 
            | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setSize(buildSizesInfo.accelerationStructureSize);
        VmaAllocationCreateInfo structBufferAllocInfo{};
        structBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        structBufferAllocInfo.flags = 0;

        vmaCreateBufferWithAlignment(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi), 
        &structBufferAllocInfo,
        asProperties.minAccelerationStructureScratchOffsetAlignment
        , reinterpret_cast<VkBuffer*>(&tlas.StructureBuffer)
        , &tlas.StructureBuffer.allocation, nullptr);
        
        structScratchBufferCi.setSize(buildSizesInfo.buildScratchSize);
        structScratchBufferCi.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        vmaCreateBufferWithAlignment(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi),
        &structBufferAllocInfo, 
        asProperties.minAccelerationStructureScratchOffsetAlignment
        , reinterpret_cast<VkBuffer*>(&tlas.ScratchBuffer.buffer),
        &tlas.ScratchBuffer.allocation, nullptr);

        vk::AccelerationStructureCreateInfoKHR accCi{};
        accCi.setBuffer(tlas.StructureBuffer.buffer)
        .setOffset(0)
        .setSize(buildSizesInfo.accelerationStructureSize)
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel);

        tlas.accel = context.device.getDevice().createAccelerationStructureKHR(accCi, nullptr, context.device.getLoader());

        std::vector<vk::AccelerationStructureInstanceKHR> accelerationStructureInstances{};
        accelerationStructureInstances.resize(bottomAccel.size());
        for(int i = 0; i < bottomAccel.size(); i++)
        {
            vk::AccelerationStructureDeviceAddressInfoKHR deviceAddInfo{};
            deviceAddInfo.setAccelerationStructure(bottomAccel[i].accel);

            accelerationStructureInstances[i].transform = core::GlmToVk(bottomAccel[i].transform);
            accelerationStructureInstances[i].setInstanceCustomIndex(i)
            .setMask(0xFF)
            .setInstanceShaderBindingTableRecordOffset(0)
            .accelerationStructureReference = context.device.getDevice().getAccelerationStructureAddressKHR(deviceAddInfo,
                context.device.getLoader());
        }

        vk::BufferCreateInfo instanceBufferCi{};
        instanceBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setSize(sizeof(vk::AccelerationStructureInstanceKHR) * accelerationStructureInstances.size());
        
        VmaAllocationCreateInfo instanceBufferAllocInfo{};
        instanceBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
                    | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        instanceBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateBufferWithAlignment(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&instanceBufferCi), 
        &instanceBufferAllocInfo, 
        asProperties.minAccelerationStructureScratchOffsetAlignment,
        reinterpret_cast<VkBuffer*>(&tlas.InstBuffer.buffer), 
        &tlas.InstBuffer.allocation, 
        &tlas.InstBuffer.allocInfo);

        std::memcpy(tlas.InstBuffer.allocInfo.pMappedData, accelerationStructureInstances.data(), 
        sizeof(vk::AccelerationStructureInstanceKHR) * accelerationStructureInstances.size());

        vk::BufferDeviceAddressInfo scratchBuffAddressInfo{};
        scratchBuffAddressInfo.setBuffer(tlas.ScratchBuffer.buffer);

        buildInfo.setDstAccelerationStructure(tlas.accel)
        .scratchData.setDeviceAddress(context.device.getDevice().getBufferAddress(scratchBuffAddressInfo));
        vk::BufferDeviceAddressInfo instanceBuffAddressInfo{};
        instanceBuffAddressInfo.setBuffer(tlas.InstBuffer.buffer);
        geometry.geometry.instances.data.deviceAddress = context.device.getDevice().getBufferAddress(instanceBuffAddressInfo);

        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(accelerationStructureInstances.size())
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);
        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};
        core::ExecuteSingleTimeCb(context.device.getDevice(), commandPool, context.device.getQueue(), [&](const vk::CommandBuffer& singleTimeCb){
            singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, context.device.getLoader());
        });
    }
}