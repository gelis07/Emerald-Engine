#include "ASManager.h"




void ASManager::Init(VkContext context, vk::detail::DispatchLoaderDynamic loader)
{
    device = context.device;
    queue = context.queue;
    alloc = context.alloc;
    commandPool = context.commandPool;
    dynamicDispatchLoader = loader;

    vk::PhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.pNext = &mAsProperties;
    context.physicalDevice.getProperties2(&deviceProperties);
}


BuildASOnGPUInfo ASManager::SetUpASForScene(const vkUtils::vkScene& scene)
{
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances)
    .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    geometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
    geometry.geometry.instances.arrayOfPointers = false;
    std::vector<vk::AccelerationStructureInstanceKHR> accelerationStructureInstances{};
    accelerationStructureInstances.resize(bottomAccStructures.size());
    for(int i = 0; i < bottomAccStructures.size(); i++)
    {
        vk::AccelerationStructureDeviceAddressInfoKHR deviceAddInfo{};
        deviceAddInfo.setAccelerationStructure(bottomAccStructures[i].accel);

        VkTransformMatrixKHR vkMatrix;
        const glm::mat4& matrix = (*scene.vkModels[scene.vkMeshes[i].modelIdx].modelMat) * (*scene.vkModels[scene.vkMeshes[i].modelIdx].nodeData)[scene.vkMeshes[i].nodeIdx].transform;
        // GLM is matrix[column][row]
        vkMatrix.matrix[0][0] = matrix[0][0]; // Row 0
        vkMatrix.matrix[0][1] = matrix[1][0];
        vkMatrix.matrix[0][2] = matrix[2][0];
        vkMatrix.matrix[0][3] = matrix[3][0]; // X Translation

        vkMatrix.matrix[1][0] = matrix[0][1]; // Row 1
        vkMatrix.matrix[1][1] = matrix[1][1];
        vkMatrix.matrix[1][2] = matrix[2][1];
        vkMatrix.matrix[1][3] = matrix[3][1]; // Y Translation

        vkMatrix.matrix[2][0] = matrix[0][2]; // Row 2
        vkMatrix.matrix[2][1] = matrix[1][2];
        vkMatrix.matrix[2][2] = matrix[2][2];
        vkMatrix.matrix[2][3] = matrix[3][2]; // Z Translation

        accelerationStructureInstances[i].transform = vkMatrix;
        accelerationStructureInstances[i].setInstanceCustomIndex(0)
        .setMask(0xFF)
        .setInstanceShaderBindingTableRecordOffset(0)
        .accelerationStructureReference = device.getAccelerationStructureAddressKHR(deviceAddInfo, dynamicDispatchLoader);
    }

    vk::BufferDeviceAddressInfo scratchBuffAddressInfo{};
    scratchBuffAddressInfo.setBuffer(topAccStructure.ScratchBuffer.buffer);

    vk::DeviceAddress scratchBufferAdd = device.getBufferAddress(scratchBuffAddressInfo);

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
    .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
    .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | 
    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
    .setSrcAccelerationStructure(topAccStructure.accel)
    .setDstAccelerationStructure(topAccStructure.accel)
    .setGeometryCount(1)
    .setScratchData(scratchBufferAdd)
    .setPGeometries(&geometry);
    
    std::memcpy(topAccStructure.InstBuffer.allocInfo.pMappedData, accelerationStructureInstances.data(), 
    sizeof(vk::AccelerationStructureInstanceKHR) * accelerationStructureInstances.size());

    vk::BufferDeviceAddressInfo instanceBuffAddressInfo{};
    instanceBuffAddressInfo.setBuffer(topAccStructure.InstBuffer.buffer);
    geometry.geometry.instances.data.deviceAddress = device.getBufferAddress(instanceBuffAddressInfo);

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(accelerationStructureInstances.size())
    .setPrimitiveOffset(0)
    .setFirstVertex(0)
    .setTransformOffset(0);
    

    return {buildInfo, buildRangeInfo};
}
void ASManager::UpdateBLASes(const std::vector<vkUtils::VkMesh>& vkMeshes)
{
    for(int i = 0; i < bottomAccStructures.size(); i++)
    {
        geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        
        vk::BufferDeviceAddressInfo buffAddInfo;
        buffAddInfo.setBuffer(vkMeshes[i].buffer);
        vk::DeviceAddress buffAddress = device.getBufferAddress(buffAddInfo);

        geometry.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        geometry.geometry.triangles.setVertexData(buffAddress)
        .setIndexData(buffAddress + vkMeshes[i].vBufSize)
        .setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setMaxVertex(vkMeshes[i].vertCount)
        .setVertexStride(sizeof(Vertex))
        .setIndexType(vk::IndexType::eUint32);

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace
        | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
        .setSrcAccelerationStructure(bottomAccStructures[i].accel)
        .setDstAccelerationStructure(bottomAccStructures[i].accel)
        .setGeometryCount(1)
        .setPGeometries(&geometry)
        .setScratchData({});

        buildInfo.dstAccelerationStructure = bottomAccStructures[i].accel;
        vk::BufferDeviceAddressInfo scratchDeviceAddressInfo;
        scratchDeviceAddressInfo.setBuffer(bottomAccStructures[i].ScratchBuffer.buffer);
        buildInfo.scratchData.deviceAddress = device.getBufferAddress(scratchDeviceAddressInfo);

        uint32_t primCount = vkMeshes[i].indexCount / 3;
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
        buildRangeInfo.setPrimitiveCount(primCount)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};

        vkUtils::ExecuteSingleTimeCb(device, commandPool, queue, [&](const vk::CommandBuffer& singleTimeCb){
            singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);

            vk::MemoryBarrier2 barrier;
            barrier.setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
            .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
            .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR);
        });
    }
}
AccelerationStruct ASManager::CreateBLAS(const vkUtils::VkMesh& vkModel)
{
    AccelerationStruct accStructure;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
    .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    
    vk::BufferDeviceAddressInfo buffAddInfo;
    buffAddInfo.setBuffer(vkModel.buffer);
    vk::DeviceAddress buffAddress = device.getBufferAddress(buffAddInfo);

    geometry.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
    geometry.geometry.triangles.setVertexData(buffAddress)
    .setIndexData(buffAddress + vkModel.vBufSize)
    .setVertexFormat(vk::Format::eR32G32B32Sfloat)
    .setMaxVertex(vkModel.vertCount)
    .setVertexStride(sizeof(Vertex))
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

    uint32_t primCount = vkModel.indexCount / 3;
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
    buildInfo, primCount, dynamicDispatchLoader);

    vk::BufferCreateInfo structScratchBufferCi;
    structScratchBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
    .setSize(buildSizesInfo.accelerationStructureSize);

    VmaAllocationCreateInfo structBufferAllocInfo{};
    structBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    structBufferAllocInfo.flags = 0;

    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi), &structBufferAllocInfo, mAsProperties.minAccelerationStructureScratchOffsetAlignment
    , reinterpret_cast<VkBuffer*>(&accStructure.StructureBuffer.buffer), &accStructure.StructureBuffer.allocation, nullptr);
    
    structScratchBufferCi.setSize(buildSizesInfo.buildScratchSize);
    structScratchBufferCi.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi)
    , &structBufferAllocInfo, mAsProperties.minAccelerationStructureScratchOffsetAlignment, reinterpret_cast<VkBuffer*>(&accStructure.ScratchBuffer.buffer),
    &accStructure.ScratchBuffer.allocation, nullptr);

    vk::AccelerationStructureCreateInfoKHR accCi;
    accCi.setBuffer(accStructure.StructureBuffer.buffer)
    .setOffset(0)
    .setSize(buildSizesInfo.accelerationStructureSize)
    .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

    accStructure.accel = device.createAccelerationStructureKHR(accCi, nullptr, dynamicDispatchLoader);

    buildInfo.dstAccelerationStructure = accStructure.accel;
    vk::BufferDeviceAddressInfo scratchDeviceAddressInfo;
    scratchDeviceAddressInfo.setBuffer(accStructure.ScratchBuffer.buffer);
    buildInfo.scratchData.deviceAddress = device.getBufferAddress(scratchDeviceAddressInfo);

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
    buildRangeInfo.setPrimitiveCount(primCount)
    .setPrimitiveOffset(0)
    .setFirstVertex(0)
    .setTransformOffset(0);

    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};

    vkUtils::ExecuteSingleTimeCb(device, commandPool, queue, [&](const vk::CommandBuffer& singleTimeCb){
        singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);
    });
    return accStructure;
}



//Got to be called after creating the bottomStructures.
AccelerationStruct ASManager::creatTLAS(const vkUtils::vkScene& scene)
{
    AccelerationStruct accStruct;

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

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, {static_cast<uint32_t>(bottomAccStructures.size())}, dynamicDispatchLoader);
    
    vk::BufferCreateInfo structScratchBufferCi;
    structScratchBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
    .setSize(buildSizesInfo.accelerationStructureSize);
    VmaAllocationCreateInfo structBufferAllocInfo{};
    structBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    structBufferAllocInfo.flags = 0;

    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi), &structBufferAllocInfo, mAsProperties.minAccelerationStructureScratchOffsetAlignment
    , reinterpret_cast<VkBuffer*>(&accStruct.StructureBuffer)
    , &accStruct.StructureBuffer.allocation, nullptr);
    
    structScratchBufferCi.setSize(buildSizesInfo.buildScratchSize);
    structScratchBufferCi.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&structScratchBufferCi), &structBufferAllocInfo, 
    mAsProperties.minAccelerationStructureScratchOffsetAlignment
    , reinterpret_cast<VkBuffer*>(&accStruct.ScratchBuffer.buffer),
    &accStruct.ScratchBuffer.allocation, nullptr);

    vk::AccelerationStructureCreateInfoKHR accCi{};
    accCi.setBuffer(accStruct.StructureBuffer.buffer)
    .setOffset(0)
    .setSize(buildSizesInfo.accelerationStructureSize)
    .setType(vk::AccelerationStructureTypeKHR::eTopLevel);

    accStruct.accel = device.createAccelerationStructureKHR(accCi, nullptr, dynamicDispatchLoader);

    std::vector<vk::AccelerationStructureInstanceKHR> accelerationStructureInstances{};
    accelerationStructureInstances.resize(bottomAccStructures.size());
    for(int i = 0; i < bottomAccStructures.size(); i++)
    {
        vk::AccelerationStructureDeviceAddressInfoKHR deviceAddInfo{};
        deviceAddInfo.setAccelerationStructure(bottomAccStructures[i].accel);


        VkTransformMatrixKHR vkMatrix;
        const glm::mat4& matrix = *scene.vkModels[scene.vkMeshes[i].modelIdx].modelMat * (*scene.vkModels[scene.vkMeshes[i].modelIdx].nodeData)[scene.vkMeshes[i].nodeIdx].transform;
        // GLM is matrix[column][row]
        vkMatrix.matrix[0][0] = matrix[0][0]; // Row 0
        vkMatrix.matrix[0][1] = matrix[1][0];
        vkMatrix.matrix[0][2] = matrix[2][0];
        vkMatrix.matrix[0][3] = matrix[3][0]; // X Translation

        vkMatrix.matrix[1][0] = matrix[0][1]; // Row 1
        vkMatrix.matrix[1][1] = matrix[1][1];
        vkMatrix.matrix[1][2] = matrix[2][1];
        vkMatrix.matrix[1][3] = matrix[3][1]; // Y Translation

        vkMatrix.matrix[2][0] = matrix[0][2]; // Row 2
        vkMatrix.matrix[2][1] = matrix[1][2];
        vkMatrix.matrix[2][2] = matrix[2][2];
        vkMatrix.matrix[2][3] = matrix[3][2]; // Z Translation

        accelerationStructureInstances[i].transform = vkMatrix;
        accelerationStructureInstances[i].setInstanceCustomIndex(0)
        .setMask(0xFF)
        .setInstanceShaderBindingTableRecordOffset(0)
        .accelerationStructureReference = device.getAccelerationStructureAddressKHR(deviceAddInfo, dynamicDispatchLoader);
    }

    vk::BufferCreateInfo instanceBufferCi{};
    instanceBufferCi.setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
    .setSize(sizeof(vk::AccelerationStructureInstanceKHR) * accelerationStructureInstances.size());
    
    VmaAllocationCreateInfo instanceBufferAllocInfo{};
    instanceBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
                | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    instanceBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&instanceBufferCi), &instanceBufferAllocInfo, mAsProperties.minAccelerationStructureScratchOffsetAlignment,
    reinterpret_cast<VkBuffer*>(&accStruct.InstBuffer.buffer), &accStruct.InstBuffer.allocation, &accStruct.InstBuffer.allocInfo);

    std::memcpy(accStruct.InstBuffer.allocInfo.pMappedData, accelerationStructureInstances.data(), 
    sizeof(vk::AccelerationStructureInstanceKHR) * accelerationStructureInstances.size());

    vk::BufferDeviceAddressInfo scratchBuffAddressInfo{};
    scratchBuffAddressInfo.setBuffer(accStruct.ScratchBuffer.buffer);

    buildInfo.setDstAccelerationStructure(accStruct.accel)
    .scratchData.setDeviceAddress(device.getBufferAddress(scratchBuffAddressInfo));
    vk::BufferDeviceAddressInfo instanceBuffAddressInfo{};
    instanceBuffAddressInfo.setBuffer(accStruct.InstBuffer.buffer);
    geometry.geometry.instances.data.deviceAddress = device.getBufferAddress(instanceBuffAddressInfo);

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(accelerationStructureInstances.size())
    .setPrimitiveOffset(0)
    .setFirstVertex(0)
    .setTransformOffset(0);
    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};
    vkUtils::ExecuteSingleTimeCb(device, commandPool, queue, [&](const vk::CommandBuffer& singleTimeCb){
        singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);
    });
    return accStruct;
}
void ASManager::DeleteSceneAS()
{
    for(int i = 0; i < bottomAccStructures.size(); i++)
    {
        device.destroyAccelerationStructureKHR(bottomAccStructures[i].accel, nullptr, dynamicDispatchLoader);
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(bottomAccStructures[i].StructureBuffer.buffer), bottomAccStructures[i].StructureBuffer.allocation);
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(bottomAccStructures[i].ScratchBuffer.buffer), bottomAccStructures[i].ScratchBuffer.allocation);
    }
    device.destroyAccelerationStructureKHR(topAccStructure.accel, nullptr, dynamicDispatchLoader);
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(topAccStructure.StructureBuffer.buffer), topAccStructure.StructureBuffer.allocation);
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(topAccStructure.ScratchBuffer.buffer), topAccStructure.ScratchBuffer.allocation);
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(topAccStructure.InstBuffer.buffer), topAccStructure.InstBuffer.allocation);

    bottomAccStructures.clear();
}
void ASManager::CreateSceneAS(const vkUtils::vkScene& scene)
{
    for(int i = 0; i < scene.vkMeshes.size(); i++)
    {
        bottomAccStructures.push_back(CreateBLAS(scene.vkMeshes[i]));
    }

    topAccStructure = creatTLAS(scene);
}
