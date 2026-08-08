#include "vkRaytracer.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <filesystem>
#include <core/Utils.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stbi_write.h>
#include <core/Utils.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


struct MaterialShaderData
{
    alignas(16) glm::vec3 albedo;
    alignas(16) glm::vec3 emmColor;
    float roughness;

    uint32_t albedoMap = -1;
    uint32_t roughnessMap = -1;
};

struct CameraShaderData
{
    alignas(16) glm::vec3 pos;
    int frameIdx;
    alignas(16) glm::mat4 invProj;
    alignas(16) glm::mat4 invView;
    MaterialShaderData mat[512];
};

void vkRaytracer::Init(RaytracerInitInfo info)
{
    dynamicDispatchLoader = vk::detail::DispatchLoaderDynamic(info.instance, vkGetInstanceProcAddr, info.device);
    mComputeQueue = info.computeQueue;

    //Getting properties
    vk::PhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.pNext = &mAsProperties;
    info.physicalDevice.getProperties2(&deviceProperties);
    vk::PhysicalDeviceProperties2 rayTracingProperties{};
    rayTracingProperties.pNext = &mRtProperties;
    info.physicalDevice.getProperties2(&rayTracingProperties);


    vk::CommandPoolCreateInfo commandPoolCi;
    commandPoolCi.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .setQueueFamilyIndex(info.queueFamily);
    mCommandPool = info.device.createCommandPool(commandPoolCi);

    //Loading models.
    mVkScene = vkUtils::LoadScene(info.device, info.alloc, mCommandPool, mComputeQueue, info.rs->scene);
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        bottomAccStructures.push_back(CreateBLAS(info.device, info.alloc, mComputeQueue, mVkScene.vkModels[i]));
    }
    topAccStructure = creatTLAS(info.device, info.alloc, mComputeQueue);


    vk::SamplerCreateInfo samplerCi;
    samplerCi.setMagFilter(vk::Filter::eLinear)
    .setMinFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);

    texturSampler = info.device.createSampler(samplerCi);
    //Creating descriptor layout.
    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos(mVkScene.vkModels.size());
    std::vector<vk::DescriptorBufferInfo> indexBufferInfos(mVkScene.vkModels.size());
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        vk::DescriptorBufferInfo vertexBufferDescInfo;
        vertexBufferDescInfo.setBuffer(mVkScene.vkModels[i].buffer)
        .setOffset(0)
        .setRange(mVkScene.vkModels[i].vBufSize);
        vertexBufferInfos[i] = vertexBufferDescInfo;

        vk::DescriptorBufferInfo indexBufferDescInfo;
        indexBufferDescInfo.setBuffer(mVkScene.vkModels[i].buffer)
        .setOffset(mVkScene.vkModels[i].vBufSize)
        .setRange(mVkScene.vkModels[i].iBufSize);
        indexBufferInfos[i] = indexBufferDescInfo;
    }

    VkImageCreateData skyboxCreateData;
    skyboxCreateData.allocator = info.alloc;
    skyboxCreateData.device = info.device;
    skyboxCreateData.commandPool = mCommandPool;
    skyboxCreateData.format = vk::Format::eR32G32B32A32Sfloat;
    skyboxCreateData.queue = mComputeQueue;
    skyboxCreateData.sizePerByte = 4;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    float* skyboxData = stbi_loadf("sky.hdr", &width, &height, &channels, 4);

    skybox = vkUtils::LoadTexture(width, height, channels, (unsigned char*)skyboxData, skyboxCreateData);

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.resize(8);


    stbi_image_free(skyboxData);

    bindings[0].setBinding(0)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    bindings[1].setBinding(1)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    bindings[2].setBinding(2)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[3].setBinding(3)
    .setDescriptorCount(500)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[4].setBinding(4)
    .setDescriptorCount(500)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[5].setBinding(5)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    bindings[6].setBinding(6)
    .setDescriptorCount(500)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[7].setBinding(7)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setStageFlags(vk::ShaderStageFlagBits::eMissKHR);

    std::vector<vk::DescriptorBindingFlags> bindingFlags = {
    {}, // Binding 0
        {},
        {},
    vk::DescriptorBindingFlagBits::ePartiallyBound, // Binding 3
    vk::DescriptorBindingFlagBits::ePartiallyBound,  // Binding 4
    {},
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    {}
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
    flagsCreateInfo.setBindingFlags(bindingFlags);

    vk::DescriptorSetLayoutCreateInfo layoutCi;
    
    layoutCi.setBindingCount(static_cast<uint32_t>(bindings.size()))
    .setPNext(&flagsCreateInfo)
    .setPBindings(bindings.data());

    setLayout = info.device.createDescriptorSetLayout(layoutCi);

    std::vector<vk::DescriptorPoolSize> poolSizes = 
    {
        {
            vk::DescriptorType::eAccelerationStructureKHR,
            1
        },
        {
            vk::DescriptorType::eStorageImage,
            2
        },
        {
            vk::DescriptorType::eCombinedImageSampler,
            1000
        },
        {
            vk::DescriptorType::eUniformBuffer,
            1
        },
        {
            vk::DescriptorType::eStorageBuffer,
            1000
        }
    };

    //Creating summed image.
    vk::ImageCreateInfo sumPixelCi;
    sumPixelCi.setImageType(vk::ImageType::e2D)
    .setFormat(vk::Format::eR8G8B8A8Unorm)
    .extent.setWidth(RTXimgWidth).setHeight(RTXimgHeight).setDepth(1);
    sumPixelCi.setMipLevels(1)
    .setArrayLayers(1)
    .setSamples(vk::SampleCountFlagBits::e1)
    .setTiling(vk::ImageTiling::eOptimal)
    .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
    .setInitialLayout(vk::ImageLayout::eUndefined);

    VmaAllocation sumPixelAlloc{};
    VmaAllocationCreateInfo sumPixelAllocInfo{};
    sumPixelAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    sumPixelAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(info.alloc, reinterpret_cast<VkImageCreateInfo*>(&sumPixelCi), &sumPixelAllocInfo,
    reinterpret_cast<VkImage*>(&sumImage), &sumPixelAlloc, nullptr);

    vk::ImageViewCreateInfo sumPixelViewCi;
    sumPixelViewCi.setImage(sumImage)
    .setFormat(vk::Format::eR8G8B8A8Unorm)
    .setViewType(vk::ImageViewType::e2D)
    .components.r = vk::ComponentSwizzle::eR;
    sumPixelViewCi.components.g = vk::ComponentSwizzle::eG;
    sumPixelViewCi.components.b = vk::ComponentSwizzle::eB;
    sumPixelViewCi.components.a = vk::ComponentSwizzle::eA;
    sumPixelViewCi.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setBaseArrayLayer(0)
    .setBaseMipLevel(0)
    .setLayerCount(1)
    .setLevelCount(1);

    sumImageView = info.device.createImageView(sumPixelViewCi);


    //Creating render target.
    vk::ImageCreateInfo rendTargCi;
    rendTargCi.setImageType(vk::ImageType::e2D)
    .setFormat(vk::Format::eR32G32B32A32Sfloat)
    .extent.setWidth(RTXimgWidth).setHeight(RTXimgHeight).setDepth(1);
    rendTargCi.setMipLevels(1)
    .setArrayLayers(1)
    .setSamples(vk::SampleCountFlagBits::e1)
    .setTiling(vk::ImageTiling::eOptimal)
    .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
    .setInitialLayout(vk::ImageLayout::eUndefined);

    VmaAllocation imgAlloc{};
    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(info.alloc, reinterpret_cast<VkImageCreateInfo*>(&rendTargCi), &imgAllocInfo, reinterpret_cast<VkImage*>(&renderTargetImage),
    &imgAlloc, nullptr);

    vk::ImageViewCreateInfo rendTargImageViewCi;
    rendTargImageViewCi.setImage(renderTargetImage)
    .setViewType(vk::ImageViewType::e2D)
    .setFormat(vk::Format::eR32G32B32A32Sfloat)
    .components.r = vk::ComponentSwizzle::eR;
    rendTargImageViewCi.components.g = vk::ComponentSwizzle::eG;
    rendTargImageViewCi.components.b = vk::ComponentSwizzle::eB;
    rendTargImageViewCi.components.a = vk::ComponentSwizzle::eA;
    rendTargImageViewCi.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setBaseArrayLayer(0)
    .setBaseMipLevel(0)
    .setLayerCount(1)
    .setLevelCount(1);

    renderTargetImageView = info.device.createImageView(rendTargImageViewCi);


    //Creating the uniform buffers with the scene data.
    vk::Buffer shaderData;
    vk::BufferCreateInfo sdCi;
    sdCi.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
    .setSize(static_cast<uint32_t>(sizeof(CameraShaderData)));

    VmaAllocation shaderDataAlloc{};
    VmaAllocationCreateInfo sdAllocCi{};
    sdAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    sdAllocCi.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(info.alloc, reinterpret_cast<VkBufferCreateInfo*>(&sdCi), &sdAllocCi, 
    reinterpret_cast<VkBuffer*>(&shaderData), &shaderDataAlloc, &shaderDataAllocInfo);

    CameraShaderData csd;
    csd.invProj = info.rs->scene.camera.GetInvProjection();
    csd.invView = info.rs->scene.camera.GetInvView();
    csd.pos = info.rs->scene.camera.GetPos();
    //! Gotta update here!
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        csd.mat[i].albedo = glm::vec4(info.rs->scene.materials[mVkScene.vkModels[i].model->mMeshes[0].matIndex].albedo, 1.0f);
        csd.mat[i].emmColor = glm::vec4(info.rs->scene.materials[mVkScene.vkModels[i].model->mMeshes[0].matIndex].emmColor, 1.0f);
        csd.mat[i].roughness = info.rs->scene.materials[mVkScene.vkModels[i].model->mMeshes[0].matIndex].roughness;
    }

    std::memcpy(shaderDataAllocInfo.pMappedData, &csd, sizeof(CameraShaderData));



    vk::DescriptorPoolCreateInfo descPoolCi;
    descPoolCi.setMaxSets(1)
    .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
    .setPPoolSizes(poolSizes.data());

    descPool = info.device.createDescriptorPool(descPoolCi);

    //Finalizing the descriptor set.
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(descPool)
    .setDescriptorSetCount(1)
    .setPSetLayouts(&setLayout);
    descSet = info.device.allocateDescriptorSets(allocInfo).front();

    vk::DescriptorImageInfo renderTargetImageInfo;
    renderTargetImageInfo.setImageView(renderTargetImageView)
    .setImageLayout(vk::ImageLayout::eGeneral);

    vk::DescriptorImageInfo sumImageInfo;
    sumImageInfo.setImageView(sumImageView)
    .setImageLayout(vk::ImageLayout::eGeneral);

    vk::WriteDescriptorSetAccelerationStructureKHR accStructInfo;
    accStructInfo.setAccelerationStructureCount(1)
    .setPAccelerationStructures(&topAccStructure.accel);

    vk::DescriptorBufferInfo sdInfo;
    sdInfo.setBuffer(shaderData)
    .setOffset(0)
    .setRange(sizeof(CameraShaderData));

    vk::DescriptorImageInfo skyboxInfo;
    skyboxInfo.setImageView(skybox.view)
    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);


    std::vector<vk::WriteDescriptorSet> descWrites;
    descWrites.resize(7);
    descWrites[0].setDstSet(descSet)
    .setDstBinding(0)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setPImageInfo(&renderTargetImageInfo);
    descWrites[1].setDstSet(descSet)
    .setPNext(&accStructInfo)
    .setDstBinding(1)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
    descWrites[2].setDstSet(descSet)
    .setDstBinding(2)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setPBufferInfo(&sdInfo);
    descWrites[3].setDstSet(descSet)
    .setDstBinding(3)
    .setDescriptorCount(indexBufferInfos.size())
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(indexBufferInfos.data());
    descWrites[4].setDstSet(descSet)
    .setDstBinding(4)
    .setDescriptorCount(vertexBufferInfos.size())
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(vertexBufferInfos.data());
    descWrites[5].setDstSet(descSet)
    .setDstBinding(5)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setPImageInfo(&sumImageInfo);
    descWrites[6].setDstSet(descSet)
    .setDstBinding(7)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setPImageInfo(&skyboxInfo);

    info.device.updateDescriptorSets(static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);

    //Setting up the pipeline
    vk::PipelineLayoutCreateInfo pipLayoutInfo;
    pipLayoutInfo.setSetLayoutCount(1)
    .setPSetLayouts(&setLayout)
    .setPushConstantRangeCount(0)
    .setPPushConstantRanges(nullptr);
    fmt::println("{}", std::filesystem::current_path().string());
    pipLayout = info.device.createPipelineLayout(pipLayoutInfo);

    vk::ShaderModule rayGenModule = createShaderModule(info.device, "../Shaders/RT/shader.rgen.spv");
    vk::ShaderModule rayMissModule = createShaderModule(info.device, "../Shaders/RT/shader.rmiss.spv");
    vk::ShaderModule rayHitModule = createShaderModule(info.device, "../Shaders/RT/shader.rchit.spv");

    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    stages.resize(3);
    stages[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR)
    .setModule(rayGenModule)
    .setPName("main");
    stages[1].setStage(vk::ShaderStageFlagBits::eMissKHR)
    .setModule(rayMissModule)
    .setPName("main");
    stages[2].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
    .setModule(rayHitModule)
    .setPName("main");

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;
    groups.resize(3);
    groups[0].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
    .setGeneralShader(0)
    .setClosestHitShader(vk::ShaderUnusedKHR)
    .setAnyHitShader(vk::ShaderUnusedKHR)
    .setIntersectionShader(vk::ShaderUnusedKHR);
    groups[1].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
    .setGeneralShader(1)
    .setIntersectionShader(vk::ShaderUnusedKHR)
    .setClosestHitShader(vk::ShaderUnusedKHR)
    .setAnyHitShader(vk::ShaderUnusedKHR);
    groups[2].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
    .setGeneralShader(vk::ShaderUnusedKHR)
    .setAnyHitShader(vk::ShaderUnusedKHR)
    .setClosestHitShader(2)
    .setIntersectionShader(vk::ShaderUnusedKHR);

    vk::PipelineLibraryCreateInfoKHR libCi;
    libCi.setLibraryCount(0);
    
    vk::RayTracingPipelineCreateInfoKHR rtPipCi;
    rtPipCi.setStageCount(static_cast<uint32_t>(stages.size()))
    .setPStages(stages.data())
    .setGroupCount(static_cast<uint32_t>(groups.size()))
    .setPGroups(groups.data())
    .setMaxPipelineRayRecursionDepth(mRtProperties.maxRayRecursionDepth)
    .setPLibraryInfo(&libCi)
    .setPLibraryInterface(nullptr)
    .setPLibraryInfo(nullptr)
    .setLayout(pipLayout)
    .setBasePipelineHandle(VK_NULL_HANDLE)
    .setBasePipelineIndex(0);

    pip = info.device.createRayTracingPipelineKHR(nullptr, nullptr, rtPipCi, nullptr, dynamicDispatchLoader).value;

    info.device.destroyShaderModule(rayGenModule);
    info.device.destroyShaderModule(rayMissModule);
    info.device.destroyShaderModule(rayHitModule);

    uint32_t baseAlign = mRtProperties.shaderGroupBaseAlignment;
    uint32_t handleSize = mRtProperties.shaderGroupHandleSize;

    const uint32_t shaderGroupCount = 3;
    vk::DeviceSize sbtBufferSize = baseAlign * 3;

    vk::Buffer shaderBindingTableBuffer;

    vk::BufferCreateInfo sbtBuffCi;
    sbtBuffCi.setUsage(vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
    .setSize(sbtBufferSize);

    VmaAllocation sbtAlloc{};
    VmaAllocationInfo sbtAllocInfo{};

    VmaAllocationCreateInfo sbtAllocCi{};
    sbtAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    sbtAllocCi.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBufferWithAlignment(info.alloc, reinterpret_cast<VkBufferCreateInfo*>(&sbtBuffCi), &sbtAllocCi, mRtProperties.shaderGroupBaseAlignment, 
    reinterpret_cast<VkBuffer*>(&shaderBindingTableBuffer), &sbtAlloc, &sbtAllocInfo);

    std::vector<uint8_t> handles = info.device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pip, 0 , shaderGroupCount,
    shaderGroupCount * handleSize, dynamicDispatchLoader);

    vk::BufferDeviceAddressInfo sbtBufferAddressInfo{};
    sbtBufferAddressInfo.setBuffer(shaderBindingTableBuffer);
    vk::DeviceAddress sbtAddress = info.device.getBufferAddress(sbtBufferAddressInfo);

    vk::StridedDeviceAddressRegionKHR addressRegion;
    addressRegion.setStride(baseAlign)
    .setSize(handleSize);

    sbtRayGenAddressRegion = addressRegion;
    sbtRayGenAddressRegion.setSize(baseAlign)
    .setDeviceAddress(sbtAddress);

    sbtMissAddressRegion = addressRegion;
    sbtMissAddressRegion.setDeviceAddress(sbtAddress + baseAlign);

    sbtCHitAddresRegion = addressRegion;
    sbtCHitAddresRegion.setDeviceAddress(sbtAddress + baseAlign * 2);
    uint8_t* sbtBufferData = static_cast<uint8_t*>(sbtAllocInfo.pMappedData);

    std::memcpy(sbtBufferData, handles.data(), handleSize);
    std::memcpy(sbtBufferData + baseAlign, handles.data() + handleSize, handleSize);
    std::memcpy(sbtBufferData + baseAlign * 2, handles.data() + handleSize * 2, handleSize);

    mLastModelSize = info.rs->scene.models.size();
    fence = info.device.createFence({});
}



void vkRaytracer::Run(RaytracerRenderInfo info)
{
    bool shouldUpdate = false;
    if(info.rs->scene.models.size() != mLastModelSize)
    {
        UpdateModels(info.device, info.alloc, info.rs->scene);
        mLastModelSize = info.rs->scene.models.size();
        shouldUpdate = true;
    }
    if(info.rs->scene.camera.moved)
    {
        frameIdxForRender = 0;
    }
    vk::AccelerationStructureGeometryKHR geometry{};
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
        const glm::mat4& matrix = mVkScene.vkModels[i].model->model;

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
        .accelerationStructureReference = info.device.getAccelerationStructureAddressKHR(deviceAddInfo, dynamicDispatchLoader);
    }

    vk::BufferDeviceAddressInfo scratchBuffAddressInfo{};
    scratchBuffAddressInfo.setBuffer(topAccStructure.ScratchBuffer.buffer);

    vk::DeviceAddress scratchBufferAdd = info.device.getBufferAddress(scratchBuffAddressInfo);

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
    geometry.geometry.instances.data.deviceAddress = info.device.getBufferAddress(instanceBuffAddressInfo);

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(accelerationStructureInstances.size())
    .setPrimitiveOffset(0)
    .setFirstVertex(0)
    .setTransformOffset(0);

    shouldUpdate = true; ///!GOTTA REMOVE THIS AFTER DEBUGGING
    if(frameIdx == 0)
    {
        CreateRayTracingCB(info.device, buildInfo, buildRangeInfo, true);
    }
    else if(shouldUpdate)
    {
        CreateRayTracingCB(info.device, buildInfo, buildRangeInfo, false);
    }

    CameraShaderData csd;
    csd.invProj = info.rs->scene.camera.GetInvProjection();
    csd.invView = info.rs->scene.camera.GetInvView();
    csd.pos = info.rs->scene.camera.GetPos();
    csd.frameIdx = frameIdxForRender;
    //! Gotta update here!    
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        csd.mat[i].albedo = glm::vec4(info.rs->scene.materials[mVkScene.vkModels[i].matIdx].albedo, 1.0f);
        csd.mat[i].emmColor = glm::vec4(info.rs->scene.materials[mVkScene.vkModels[i].matIdx].emmColor, 1.0f);
        csd.mat[i].roughness = info.rs->scene.materials[mVkScene.vkModels[i].matIdx].roughness;

        csd.mat[i].albedoMap = info.rs->scene.materials[mVkScene.vkModels[i].matIdx].albedoTexture;
    }
    
    std::memcpy(shaderDataAllocInfo.pMappedData, &csd, sizeof(CameraShaderData));
    info.device.resetFences(fence);

    vk::SubmitInfo subInfo;
    subInfo.setCommandBufferCount(1)
    .setPCommandBuffers(&cb);

    mComputeQueue.submit(1, &subInfo, fence);

    
    info.device.waitForFences(1, &fence, true, UINT64_MAX);

    info.device.resetFences(fence);

    frameIdx++;
    frameIdxForRender++;
    info.rs->frameIdx = frameIdxForRender;
}



void vkRaytracer::UpdateModels(const vk::Device& device, const VmaAllocator& alloc, Scene& scene)
{


    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(mVkScene.vkModels[i].buffer), mVkScene.vkModels[i].bufferAllocation);
    }

    mVkScene.vkModels.clear();

    // for(int i = 0; i < mVkScene.vkTextures.size(); i++)
    // {
    //     vmaDestroyImage(alloc, static_cast<VkImage>(mVkScene.vkTextures[i].image), mVkScene.vkTextures[i].alloc);
    //     device.destroyImageView(mVkScene.vkTextures[i].view);
    // }
    // mVkScene.vkTextures.clear();
    mVkScene = vkUtils::LoadScene(device, alloc, mCommandPool, mComputeQueue, scene);


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

    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        bottomAccStructures.push_back(CreateBLAS(device, alloc, mComputeQueue, mVkScene.vkModels[i]));
    }

    topAccStructure = creatTLAS(device, alloc, mComputeQueue);


    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos(mVkScene.vkModels.size());
    std::vector<vk::DescriptorBufferInfo> indexBufferInfos(mVkScene.vkModels.size());
    std::vector<vk::DescriptorImageInfo> textureInfos(mVkScene.vkTextures.size());
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        vk::DescriptorBufferInfo vertexBufferDescInfo;
        vertexBufferDescInfo.setBuffer(mVkScene.vkModels[i].buffer)
        .setOffset(0)
        .setRange(mVkScene.vkModels[i].vBufSize);
        vertexBufferInfos[i] = vertexBufferDescInfo;

        vk::DescriptorBufferInfo indexBufferDescInfo;
        indexBufferDescInfo.setBuffer(mVkScene.vkModels[i].buffer)
        .setOffset(mVkScene.vkModels[i].vBufSize)
        .setRange(mVkScene.vkModels[i].iBufSize);
        indexBufferInfos[i] = indexBufferDescInfo;
    }

    for(int i = 0; i < mVkScene.vkTextures.size(); i++)
    {
        vk::DescriptorImageInfo textureImageInfo;
        textureImageInfo.setSampler(texturSampler)
        .setImageView(mVkScene.vkTextures[i].view)
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        textureInfos[i] = textureImageInfo;
    }


    std::vector<vk::WriteDescriptorSet> descWrite;
    descWrite.resize(4);
    vk::WriteDescriptorSetAccelerationStructureKHR accStructInfo;
    accStructInfo.setAccelerationStructureCount(1)
    .setPAccelerationStructures(&topAccStructure.accel);
    descWrite[0].setDstSet(descSet)
    .setPNext(&accStructInfo)
    .setDstBinding(1)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
    descWrite[1].setDstSet(descSet)
    .setDstBinding(3)
    .setDescriptorCount(indexBufferInfos.size())
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(indexBufferInfos.data());
    descWrite[2].setDstSet(descSet)
    .setDstBinding(4)
    .setDescriptorCount(vertexBufferInfos.size())
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(vertexBufferInfos.data());
    descWrite[3].setDstSet(descSet)
    .setDstBinding(6)
    .setDescriptorCount(textureInfos.size())
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setPImageInfo(textureInfos.data());

    device.updateDescriptorSets(static_cast<uint32_t>(descWrite.size()), descWrite.data(), 0, nullptr);
}


void vkRaytracer::CreateRayTracingCB(const vk::Device& device, vk::AccelerationStructureBuildGeometryInfoKHR buildInfo,vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo , bool init)
{

    if(!init)
    {
        device.freeCommandBuffers(mCommandPool, cb);
    }

    vk::CommandBufferAllocateInfo cbAllocInfo;
    cbAllocInfo.setCommandPool(mCommandPool)
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandBufferCount(1);
    
    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&buildRangeInfo};
    cb = device.allocateCommandBuffers(cbAllocInfo).front();
    vk::CommandBufferBeginInfo beginInfo{};
    cb.begin(beginInfo);

    cb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);


    vk::MemoryBarrier2 barrierAcc{};
    barrierAcc.setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
        .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR);

    vk::DependencyInfo dependencyInfo{};
    dependencyInfo.setMemoryBarrierCount(1)
                .setPMemoryBarriers(&barrierAcc);

    cb.pipelineBarrier2(dependencyInfo);

    std::array<vk::ImageMemoryBarrier2, 2> barriers;

    barriers[0].setSrcAccessMask(vk::AccessFlagBits2::eNoneKHR)
    .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
    .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eGeneral)
    .setImage(sumImage)
    .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setBaseMipLevel(0)
    .setLevelCount(1)
    .setBaseArrayLayer(0)
    .setLayerCount(1);

    barriers[1].setSrcAccessMask(vk::AccessFlagBits2::eNoneKHR)
    .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
    .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eGeneral)
    .setImage(renderTargetImage)
    .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setBaseMipLevel(0)
    .setLevelCount(1)
    .setBaseArrayLayer(0)
    .setLayerCount(1);


    vk::DependencyInfo depInfo;
    depInfo.setPImageMemoryBarriers(barriers.data())
    .setImageMemoryBarrierCount(barriers.size());
    cb.pipelineBarrier2(depInfo);

    cb.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,pip);

    std::vector<vk::DescriptorSet> descSetsToBind = {descSet};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipLayout, 0, descSetsToBind, nullptr);
    cb.traceRaysKHR(sbtRayGenAddressRegion, sbtMissAddressRegion, sbtCHitAddresRegion, {}
    , RTXimgWidth, RTXimgHeight, 1, dynamicDispatchLoader);


    vk::ImageMemoryBarrier2 barrierToGUI;
    barrierToGUI.setImage(sumImage)
    .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
    .setBaseMipLevel(0)
    .setLevelCount(1)
    .setBaseArrayLayer(0)
    .setLayerCount(1);
    barrierToGUI.setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
    .setDstAccessMask(vk::AccessFlagBits2::eNone)
    .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
    .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
    .setOldLayout(vk::ImageLayout::eGeneral)
    .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    
    vk::DependencyInfo depInfoGUI;
    depInfoGUI.setPImageMemoryBarriers(&barrierToGUI)
    .setImageMemoryBarrierCount(1);

    cb.pipelineBarrier2(depInfoGUI);
    cb.end();
}

vk::ShaderModule vkRaytracer::createShaderModule(const vk::Device& device, const std::string& path)
{
    std::vector<char> shaderCode = Utils::ReadFileBinary(path);
    vk::ShaderModuleCreateInfo ci;
    ci.setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()))
    .setCodeSize(shaderCode.size());
    vk::ShaderModule module = device.createShaderModule(ci);
    return module;
}


AccelerationStruct vkRaytracer::CreateBLAS(const vk::Device& device, const VmaAllocator alloc, const vk::Queue& queue,
const vkUtils::vkModel& vkModel)
{

    AccelerationStruct accStructure;

    vk::AccelerationStructureGeometryKHR geometry;
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
    .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
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

    vkUtils::ExecuteSingleTimeCb(device, mCommandPool, mComputeQueue, [&](const vk::CommandBuffer& singleTimeCb){
        singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);
    });
    return accStructure;
}

//Got to be called after creating the bottomStructures.
AccelerationStruct vkRaytracer::creatTLAS(const vk::Device& device,const VmaAllocator alloc, const vk::Queue& queue)
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
        const glm::mat4& matrix = mVkScene.vkModels[i].model->model;
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
    vkUtils::ExecuteSingleTimeCb(device, mCommandPool, mComputeQueue, [&](const vk::CommandBuffer& singleTimeCb){
        singleTimeCb.buildAccelerationStructuresKHR(1, &buildInfo, pBuildRangeInfos, dynamicDispatchLoader);
    });
    return accStruct;
}

void vkRaytracer::ExportToPng(VmaAllocator alloc, vk::Device device)
{
    uint32_t width = RTXimgWidth;
    uint32_t height = RTXimgHeight;
    uint32_t bytesPerPixel = 4; // Assuming 8-bit channels (RGBA/BGRA)
    vk::BufferCreateInfo dstBufferCi;
    dstBufferCi.setSize(width * height * bytesPerPixel);
    dstBufferCi.setUsage(vk::BufferUsageFlagBits::eTransferDst);

    VmaAllocation dstBufferAlloc;
    VmaAllocationInfo dstBufferAllocInfo;
    VmaAllocationCreateInfo dstBufferAllocCi{};
    dstBufferAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    dstBufferAllocCi.usage = VMA_MEMORY_USAGE_AUTO;
    vk::Buffer dstBuffer;
    vmaCreateBuffer(alloc, reinterpret_cast<VkBufferCreateInfo*>(&dstBufferCi), &dstBufferAllocCi, reinterpret_cast<VkBuffer*>(&dstBuffer),
    &dstBufferAlloc, &dstBufferAllocInfo);


    vkUtils::ExecuteSingleTimeCb(device, mCommandPool, mComputeQueue, [&](const vk::CommandBuffer& singleTimeCb)
    {
        vk::ImageMemoryBarrier2 bImgToDst;
        bImgToDst.setImage(sumImage)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
        .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

        vk::DependencyInfo depInfo;
        depInfo.setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&bImgToDst);

        singleTimeCb.pipelineBarrier2(depInfo);

        vk::BufferImageCopy imgCopy;
        imgCopy.setBufferOffset(0)
        .setBufferImageHeight(0)
        .setBufferRowLength(0)
        .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(0);
        imgCopy.imageExtent.setDepth(1)
        .setHeight(RTXimgHeight)
        .setWidth(RTXimgWidth);

        singleTimeCb.copyImageToBuffer(sumImage, vk::ImageLayout::eTransferSrcOptimal, dstBuffer, imgCopy);

    });

    stbi_flip_vertically_on_write(true);

    stbi_write_png("vkRender.png", width, height, bytesPerPixel, dstBufferAllocInfo.pMappedData, width * bytesPerPixel);
}




void vkRaytracer::destroy(vk::Device device, VmaAllocator alloc)
{
    device.destroyImage(renderTargetImage);
    device.destroyImageView(renderTargetImageView);
    device.destroyImageView(sumImageView);
    device.destroyImage(sumImage);
    device.destroyPipelineLayout(pipLayout);
    device.destroyPipeline(pip);
    device.destroyDescriptorSetLayout(setLayout);
    device.freeDescriptorSets(descPool, 1, &descSet);
    device.destroyDescriptorPool(descPool);
    device.freeCommandBuffers(mCommandPool, 1, &cb);
    device.destroyCommandPool(mCommandPool);
    for(int i = 0; i < mVkScene.vkModels.size(); i++)
    {
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(mVkScene.vkModels[i].buffer), mVkScene.vkModels[i].bufferAllocation);
    }
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

}

