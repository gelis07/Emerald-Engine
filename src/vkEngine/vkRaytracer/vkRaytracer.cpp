#include "vkRaytracer.h"
#include "fmt/base.h"
#include "fmt/format.h"
#include <core/Utils.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stbi_write.h>
#include <core/Utils.h>
#include <chrono>
#include <fpng.h>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void vkRaytracer::Init(RaytracerInitInfo info)
{
    dynamicDispatchLoader = vk::detail::DispatchLoaderDynamic(info.instance, vkGetInstanceProcAddr, info.device);
    mComputeQueue = info.computeQueue;

    
    vk::CommandPoolCreateInfo commandPoolCi;
    commandPoolCi.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCi.setQueueFamilyIndex(info.queueFamily);
    mCommandPool = info.device.createCommandPool(commandPoolCi);
    
    vk::SamplerCreateInfo samplerCi;
    samplerCi.setMagFilter(vk::Filter::eLinear)
    .setMinFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);

    texturSampler = info.device.createSampler(samplerCi);
    VkImageCreateData skyboxCreateData;
    skyboxCreateData.allocator = info.alloc;
    skyboxCreateData.device = info.device;
    skyboxCreateData.commandPool = mCommandPool;
    skyboxCreateData.format = vk::Format::eR32G32B32A32Sfloat;
    skyboxCreateData.queue = mComputeQueue;
    skyboxCreateData.sizePerByte = 4;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    float* skyboxData = stbi_loadf("sky.hdr", &width, &height, &channels, 4);



    skybox = vkUtils::LoadTexture(width, height, channels, (unsigned char*)skyboxData, skyboxCreateData);

    mContext.alloc = info.alloc;
    mContext.commandPool = mCommandPool;
    mContext.device = info.device;
    mContext.physicalDevice = info.physicalDevice;
    mContext.queue = info.computeQueue;
    mContext.queueFamily = info.queueFamily;


    lightSampler.setUpSkyboxLightSampler(mContext, skyboxData, width * height, width, height);
    stbi_image_free(skyboxData);
    hasSkybox = false;

    asManager.Init(mContext, dynamicDispatchLoader);
    asManager.CreateSceneAS(*mVkScene);


    lightSampler.setUpSceneLightPbs(mContext, &info.rs->scene, *mVkScene);

    createSceneBuffers(info.alloc, info.rs->scene);


    setUpDescriptors();
    
    rtPip.Init(mContext, dynamicDispatchLoader, &setLayout);
    
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
    
    

    writeStaticDescriptors(*mVkScene,info.rs->scene);
    writeDynamicDescriptors(*mVkScene,info.rs->scene);

    mLastModelSize = info.rs->scene.models.size();
    fence = info.device.createFence({});
}



void vkRaytracer::Run(RaytracerRenderInfo info)
{
    info.device.waitForFences(1,&fence, true, UINT64_MAX);

    info.device.waitIdle();
    if(prevAnimState != info.rs->playAnimation)
    {
        resetFrameIdx();
        anim->animationFrame = 0;
        prevAnimState = info.rs->playAnimation;
    }
    if(info.rs->playAnimation)
    {
        playAnim(info);
    }

    if(info.rs->scene.models.size() != mLastModelSize || info.rs->ReloadScene)
    {
        reloadScene(info);
        info.rs->ReloadScene = false;
    }
    if(info.rs->scene.camera.moved)
    {
        frameIdxForRender = 0;
    }

    asManager.UpdateBLASes(mVkScene->vkMeshes);

    BuildASOnGPUInfo ASGPUBuildInfo = asManager.SetUpASForScene(*mVkScene);
    fillSceneBuffers(info.rs->scene);

    CreateRayTracingCB(info.device, ASGPUBuildInfo, frameIdx == 0);
    hasSkybox = false;


    CameraShaderData csd;
    csd.invProj = info.rs->scene.camera.GetInvProjection();
    csd.invView = info.rs->scene.camera.GetInvView();
    csd.pos = info.rs->scene.camera.GetPos();
    csd.frameIdx = frameIdxForRender;
    csd.lightCount = lightSampler.lightCount;
    csd.totalSkyboxPower = lightSampler.skyboxPower;
    csd.skybox = info.rs->EnvLight;
    if(csd.skybox)
        csd.skyboxProb = lightSampler.skyboxPower / lightSampler.mTotalPower;
    else
        csd.skyboxProb = 0.0f;
    csd.intervalLength = lightSampler.intervalLength;

    std::memcpy(shaderDataAllocInfo.pMappedData, &csd, sizeof(CameraShaderData));
    info.device.resetFences(fence);

    vk::SubmitInfo subInfo;
    subInfo.setCommandBufferCount(1)
    .setPCommandBuffers(&cb);

    mComputeQueue.submit(1, &subInfo, fence);

    

    frameIdx++;
    frameIdxForRender++;
    info.rs->frameIdx = frameIdxForRender;
}



void vkRaytracer::UpdateModels(const vk::Device& device, const VmaAllocator& alloc, Scene& scene)
{

    deleteSceneBuffers(alloc);
    asManager.DeleteSceneAS();
    asManager.CreateSceneAS(*mVkScene);

    vkUtils::LoadTextures(scene, *mVkScene);


    lightSampler.deleteScene(mContext);

    createSceneBuffers(alloc, scene);
    lightSampler.setUpSceneLightPbs(mContext, &scene, *mVkScene);
    writeDynamicDescriptors(*mVkScene, scene);

}


void vkRaytracer::CreateRayTracingCB(const vk::Device& device, BuildASOnGPUInfo ASBuildInfo , bool init)
{

    if(!init)
    {
        device.freeCommandBuffers(mCommandPool, cb);
    }

    vk::CommandBufferAllocateInfo cbAllocInfo;
    cbAllocInfo.setCommandPool(mCommandPool)
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandBufferCount(1);
    
    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = {&ASBuildInfo.buildRangeInfo};
    cb = device.allocateCommandBuffers(cbAllocInfo).front();
    vk::CommandBufferBeginInfo beginInfo{};
    cb.begin(beginInfo);

    cb.buildAccelerationStructuresKHR(1, &ASBuildInfo.buildInfo, pBuildRangeInfos, dynamicDispatchLoader);


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

    cb.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPip.pip);

    std::vector<vk::DescriptorSet> descSetsToBind = {descSet};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPip.pipLayout, 0, descSetsToBind, nullptr);
    cb.traceRaysKHR(rtPip.sbtRayGenAddressRegion, rtPip.sbtMissAddressRegion, rtPip.sbtCHitAddresRegion, {}
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





void vkRaytracer::ExportToPng(VmaAllocator alloc, vk::Device device, const std::string& filename)
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

    std::vector<uint8_t> flipped(width * height * 4);
    
    for (uint32_t y = 0; y < height; y++)
    {
        memcpy(
            flipped.data() + y * width * 4,
            reinterpret_cast<const uint8_t*>(dstBufferAllocInfo.pMappedData) + (height - 1 - y) * width * 4,
            width * 4
        );
    }
    bool ok = fpng::fpng_encode_image_to_file(
        filename.c_str(),
        flipped.data(),
        width,
        height,
        bytesPerPixel
    );
}




void vkRaytracer::destroy(vk::Device device, VmaAllocator alloc)
{
    device.destroyImage(renderTargetImage);
    device.destroyImageView(renderTargetImageView);
    device.destroyImageView(sumImageView);
    device.destroyImage(sumImage);
    device.destroyDescriptorSetLayout(setLayout);
    device.freeDescriptorSets(descPool, 1, &descSet);
    device.destroyDescriptorPool(descPool);
    device.freeCommandBuffers(mCommandPool, 1, &cb);
    device.destroyCommandPool(mCommandPool);
    rtPip.destroy();
    asManager.DeleteSceneAS();
}

void vkRaytracer::setUpDescriptors()
{
    bindings.resize(16);
    bindings[0].setBinding(0)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
    bindings[1].setBinding(1)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[2].setBinding(2)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);
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
    .setStageFlags(vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[8].setBinding(8)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[9].setBinding(9)
    .setDescriptorCount(20)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[10].setBinding(10)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[11].setBinding(11)
    .setDescriptorCount(20)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[12].setBinding(12)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[13].setBinding(13)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[14].setBinding(14)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
    bindings[15].setBinding(15)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    std::vector<vk::DescriptorBindingFlags> bindingFlags = {
    {},
    {},
    {},
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    {},
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    {},
    {},
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    {},
    vk::DescriptorBindingFlagBits::ePartiallyBound,
    {},
    {},
    {},
    {}
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
    flagsCreateInfo.setBindingFlags(bindingFlags);

    vk::DescriptorSetLayoutCreateInfo layoutCi;
    
    layoutCi.setBindingCount(static_cast<uint32_t>(bindings.size()))
    .setPNext(&flagsCreateInfo)
    .setPBindings(bindings.data());

    setLayout = mContext.device.createDescriptorSetLayout(layoutCi);

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
            2
        },
        {
            vk::DescriptorType::eStorageBuffer,
            1500
        }
    };


    vk::DescriptorPoolCreateInfo descPoolCi;
    descPoolCi.setMaxSets(1)
    .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
    .setPPoolSizes(poolSizes.data())
    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    descPool = mContext.device.createDescriptorPool(descPoolCi);
    
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(descPool)
    .setDescriptorSetCount(1)
    .setPSetLayouts(&setLayout);

    descSet = mContext.device.allocateDescriptorSets(allocInfo).front();
}


void vkRaytracer::writeStaticDescriptors(const vkUtils::vkScene& vkScene, const Scene& scene)
{
    //Creating the uniform buffers with the scene data.
    vk::Buffer shaderData;
    vk::BufferCreateInfo sdCi;
    sdCi.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
    .setSize(static_cast<uint32_t>(sizeof(CameraShaderData)));

    VmaAllocation shaderDataAlloc{};
    VmaAllocationCreateInfo sdAllocCi{};
    sdAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    sdAllocCi.usage = VMA_MEMORY_USAGE_AUTO;
    
    vmaCreateBuffer(mContext.alloc, reinterpret_cast<VkBufferCreateInfo*>(&sdCi), &sdAllocCi, 
    reinterpret_cast<VkBuffer*>(&shaderData), &shaderDataAlloc, &shaderDataAllocInfo);

    vk::DescriptorBufferInfo sdInfo;
    sdInfo.setBuffer(shaderData)
    .setOffset(0)
    .setRange(sizeof(CameraShaderData));

    vk::DescriptorImageInfo renderTargetImageInfo;
    renderTargetImageInfo.setImageView(renderTargetImageView)
    .setImageLayout(vk::ImageLayout::eGeneral);

    vk::DescriptorImageInfo sumImageInfo;
    sumImageInfo.setImageView(sumImageView)
    .setImageLayout(vk::ImageLayout::eGeneral);

    CameraShaderData csd;
    csd.invProj = scene.camera.GetInvProjection();
    csd.invView = scene.camera.GetInvView();
    csd.pos = scene.camera.GetPos();
    csd.lightCount = lightSampler.lightCount;
    csd.skybox = hasSkybox;
    
    std::memcpy(shaderDataAllocInfo.pMappedData, &csd, sizeof(CameraShaderData));

    vk::DescriptorImageInfo skyboxInfo;
    skyboxInfo.setImageView(skybox.view)
    .setSampler(texturSampler)
    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::DescriptorBufferInfo skyboxWalkersAlias;
    skyboxWalkersAlias.setBuffer(lightSampler.SkyboxWalkersAlias.buffer)
    .setOffset(0)
    .setRange(lightSampler.SkyboxWalkersAlias.size);

    std::vector<vk::WriteDescriptorSet> descWrites;
    descWrites.resize(5);
    descWrites[0].setDstSet(descSet)
    .setDstBinding(0)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setPImageInfo(&renderTargetImageInfo);
    descWrites[1].setDstSet(descSet)
    .setDstBinding(2)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setPBufferInfo(&sdInfo);
    descWrites[2].setDstSet(descSet)
    .setDstBinding(5)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageImage)
    .setPImageInfo(&sumImageInfo);
    descWrites[3].setDstSet(descSet)
    .setDstBinding(7)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setPImageInfo(&skyboxInfo);
    descWrites[4].setDstSet(descSet)
    .setDstBinding(15)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&skyboxWalkersAlias);

    mContext.device.updateDescriptorSets(static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);
}
void vkRaytracer::writeDynamicDescriptors(const vkUtils::vkScene& vkScene, const Scene& scene)
{
    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos(mVkScene->vkMeshes.size());
    std::vector<vk::DescriptorBufferInfo> indexBufferInfos(mVkScene->vkMeshes.size());
    std::vector<vk::DescriptorImageInfo> textureInfos(mVkScene->vkTextures.size());
    for(int i = 0; i < mVkScene->vkMeshes.size(); i++)
    {
        vk::DescriptorBufferInfo vertexBufferDescInfo;
        vertexBufferDescInfo.setBuffer(mVkScene->vkMeshes[i].buffer)
        .setOffset(0)
        .setRange(mVkScene->vkMeshes[i].vBufSize);
        vertexBufferInfos[i] = vertexBufferDescInfo;

        vk::DescriptorBufferInfo indexBufferDescInfo;
        indexBufferDescInfo.setBuffer(mVkScene->vkMeshes[i].buffer)
        .setOffset(mVkScene->vkMeshes[i].vBufSize)
        .setRange(mVkScene->vkMeshes[i].iBufSize);
        indexBufferInfos[i] = indexBufferDescInfo;
    }

    for(int i = 0; i < mVkScene->vkTextures.size(); i++)
    {
        vk::DescriptorImageInfo textureImageInfo;
        textureImageInfo.setSampler(texturSampler)
        .setImageView(mVkScene->vkTextures[i].view)
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        textureInfos[i] = textureImageInfo;
    }

    vk::DescriptorBufferInfo walkerAliasLightsInfo;
    walkerAliasLightsInfo.setBuffer(lightSampler.WalkersAliasLights.buffer)
    .setOffset(0)
    .setRange(lightSampler.WalkersAliasLights.size);
    vk::DescriptorBufferInfo LightProbsInfo;
    LightProbsInfo.setBuffer(lightSampler.LightProbs.buffer)
    .setOffset(0)
    .setRange(lightSampler.LightProbs.size);

    std::vector<vk::DescriptorBufferInfo> walkersAliasMeshTrianglesInfo;
    std::vector<vk::DescriptorBufferInfo> TriangleProbsInfo;
    for(int i = 0; i < lightSampler.WalkersAliasMeshTriangles.size(); i++)
    {
        vk::DescriptorBufferInfo walkersAliasMeshTriangleInfo;
        walkersAliasMeshTriangleInfo.setBuffer(lightSampler.WalkersAliasMeshTriangles[i].buffer)
        .setOffset(0)
        .setRange(lightSampler.WalkersAliasMeshTriangles[i].size);
        walkersAliasMeshTrianglesInfo.push_back(walkersAliasMeshTriangleInfo);

        vk::DescriptorBufferInfo TriangleProbInfo;
        TriangleProbInfo.setBuffer(lightSampler.TriangleProbs[i].buffer)
        .setOffset(0)
        .setRange(lightSampler.TriangleProbs[i].size);
        TriangleProbsInfo.push_back(TriangleProbInfo);
    }

    vk::DescriptorBufferInfo modelBufferInfo;
    modelBufferInfo.setBuffer(ModelBuffer.buffer)
    .setOffset(0)
    .setRange(ModelBuffer.size);

    vk::DescriptorBufferInfo meshBufferInfo;
    meshBufferInfo.setBuffer(MeshBuffer.buffer)
    .setOffset(0)
    .setRange(MeshBuffer.size);

    vk::DescriptorBufferInfo materialBufferInfo;
    materialBufferInfo.setBuffer(MaterialBuffer.buffer)
    .setOffset(0)
    .setRange(MaterialBuffer.size);



    std::vector<vk::WriteDescriptorSet> descWrite;
    descWrite.resize(6);
    vk::WriteDescriptorSetAccelerationStructureKHR accStructInfo;
    accStructInfo.setAccelerationStructureCount(1)
    .setPAccelerationStructures(&asManager.topAccStructure.accel);
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
    .setDstBinding(12)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&modelBufferInfo);
    descWrite[4].setDstSet(descSet)
    .setDstBinding(13)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&meshBufferInfo);
    descWrite[5].setDstSet(descSet)
    .setDstBinding(14)
    .setDescriptorCount(1)
    .setDstArrayElement(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&materialBufferInfo);
    if(!lightSampler.WalkersAliasMeshTriangles.empty())
    {
        vk::WriteDescriptorSet walkersAliasLightWrite;
        walkersAliasLightWrite.setDstSet(descSet)
        .setDstBinding(8)
        .setDescriptorCount(1)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(&walkerAliasLightsInfo);
        descWrite.push_back(walkersAliasLightWrite);

        vk::WriteDescriptorSet walkersAliasTriangleWrite;
        walkersAliasTriangleWrite.setDstSet(descSet)
        .setDstBinding(9)
        .setDescriptorCount(walkersAliasMeshTrianglesInfo.size())
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(walkersAliasMeshTrianglesInfo.data());
        descWrite.push_back(walkersAliasTriangleWrite);

        vk::WriteDescriptorSet LightProbsWrite;
        LightProbsWrite.setDstSet(descSet)
        .setDstBinding(10)
        .setDescriptorCount(1)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(&LightProbsInfo);
        descWrite.push_back(LightProbsWrite);

        vk::WriteDescriptorSet triangleProbsWrite;
        triangleProbsWrite.setDstSet(descSet)
        .setDstBinding(11)
        .setDescriptorCount(TriangleProbsInfo.size())
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(TriangleProbsInfo.data());
        descWrite.push_back(triangleProbsWrite);
    }
    if(!textureInfos.empty())
    {
        vk::WriteDescriptorSet textureDescWrite;
        textureDescWrite.setDstSet(descSet)
        .setDstBinding(6)
        .setDescriptorCount(textureInfos.size())
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setPImageInfo(textureInfos.data());
        descWrite.push_back(textureDescWrite);
    }
    mContext.device.updateDescriptorSets(static_cast<uint32_t>(descWrite.size()), descWrite.data(), 0, nullptr);
}

void vkRaytracer::reloadScene(RaytracerRenderInfo info)
{
    UpdateModels(info.device, info.alloc, info.rs->scene);
    mLastModelSize = info.rs->scene.models.size();
    frameIdxForRender = 0;
}

//Really simple function, but makes more sense to call this function than calling frameIdxForRender=0.
void vkRaytracer::resetFrameIdx()
{
    frameIdxForRender = 0;
}

void vkRaytracer::playAnim(RaytracerRenderInfo info)
{

    if(anim->animationFrame / anim->timeStep >= 5.0f)
    {
        info.rs->playAnimation = false;
        return;
    }

    if(frameIdxForRender > anim->renderFramesPerAnimFrame)
    {
        resetFrameIdx();
        anim->animationFrame++;
        exportAnimFrame(info);
    }

    anim->run(frameIdxForRender, info.rs->scene);
}

void vkRaytracer::exportAnimFrame(RaytracerRenderInfo info)
{
    stbi_write_png_compression_level = 0;
    std::string filename = "animation/frame" + fmt::format("{:03d}", anim->animationFrame) + ".png";
    ExportToPng(info.alloc, info.device, filename);
}


void vkRaytracer::createSceneBuffers(VmaAllocator alloc,const Scene& scene)
{
    vk::BufferCreateInfo modelBufferCi;
    modelBufferCi.setSize(scene.models.size() * sizeof(ModelDataGPU))
    .setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

    uint32_t meshSize = 0;
    for(int i = 0; i < scene.models.size(); i++)
        meshSize += scene.models[i].meshes.size();

    vk::BufferCreateInfo meshBufferCi;
    meshBufferCi.setSize(meshSize * sizeof(MeshDataGPU))
    .setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

    vk::BufferCreateInfo materialBufferCi;
    materialBufferCi.setSize(scene.materials.size() * sizeof(MaterialShaderData))
    .setUsage(vk::BufferUsageFlagBits::eStorageBuffer);


    VmaAllocationCreateInfo allocCi{};
    allocCi.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocCi.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(alloc, reinterpret_cast<VkBufferCreateInfo*>(&modelBufferCi), &allocCi
    ,reinterpret_cast<VkBuffer*>(&ModelBuffer.buffer), &ModelBuffer.allocation, &ModelBuffer.allocInfo );

    vmaCreateBuffer(alloc, reinterpret_cast<VkBufferCreateInfo*>(&meshBufferCi), &allocCi
    ,reinterpret_cast<VkBuffer*>(&MeshBuffer.buffer), &MeshBuffer.allocation, &MeshBuffer.allocInfo );

    vmaCreateBuffer(alloc, reinterpret_cast<VkBufferCreateInfo*>(&materialBufferCi), &allocCi
    ,reinterpret_cast<VkBuffer*>(&MaterialBuffer.buffer), &MaterialBuffer.allocation, &MaterialBuffer.allocInfo);

    ModelBuffer.size = scene.models.size() * sizeof(ModelDataGPU);
    MeshBuffer.size = meshSize * sizeof(MeshDataGPU);
    MaterialBuffer.size = scene.materials.size() * sizeof(MaterialShaderData);
}
void vkRaytracer::fillSceneBuffers(const Scene& scene)
{
    std::vector<ModelDataGPU> modelData;
    std::vector<MeshDataGPU> meshData;
    std::vector<MaterialShaderData> materialData;

    uint32_t meshGlobalIdx = 0;
    for(int i = 0; i < scene.models.size(); i++)
    {
        const Model& model = scene.models[i];
        ModelDataGPU modelGPU;
        modelGPU.modelMatrix = model.model;
        modelData.push_back(modelGPU);
        for(int m = 0; m < model.meshes.size(); m++)
        {
            MeshDataGPU meshGPU;
            meshGPU.globalIdx = meshGlobalIdx;
            meshGPU.matIdx = model.meshes[m].matIndex;
            meshGPU.modelIdx = i;
            meshGPU.localTransform = model.nodeData[model.meshes[m].nodeId].transform;
            meshData.push_back(meshGPU);
            meshGlobalIdx++;
        }
    }

    for(int i = 0; i < scene.materials.size(); i++)
    {
        MaterialShaderData materialGPU;
        materialGPU.albedo = scene.materials[i].albedo;
        materialGPU.roughness = scene.materials[i].roughness;
        materialGPU.albedoMap = scene.materials[i].albedoTexture;
        materialGPU.roughnessMap = scene.materials[i].roughnessTexture;
        materialGPU.metallicnesMap = scene.materials[i].metallicnesTexture;
        materialGPU.normalMap = scene.materials[i].normalTexture;
        materialGPU.emmColor = scene.materials[i].emmColor;
        materialGPU.metalness = scene.materials[i].metalness;
        materialGPU.idr = scene.materials[i].idr;
        
        materialGPU.transmittance = scene.materials[i].transmittance;
        materialData.push_back(materialGPU);
    }


    std::memcpy(MeshBuffer.allocInfo.pMappedData, meshData.data(), meshData.size() * sizeof(MeshDataGPU));
    std::memcpy(ModelBuffer.allocInfo.pMappedData, modelData.data(), modelData.size() * sizeof(ModelDataGPU));
    std::memcpy(MaterialBuffer.allocInfo.pMappedData, materialData.data(), materialData.size() * sizeof(MaterialShaderData));

}
void vkRaytracer::deleteSceneBuffers(VmaAllocator alloc)
{
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(ModelBuffer.buffer), ModelBuffer.allocation);
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(MeshBuffer.buffer), MeshBuffer.allocation);
    vmaDestroyBuffer(alloc, static_cast<VkBuffer>(MaterialBuffer.buffer), MaterialBuffer.allocation);
}