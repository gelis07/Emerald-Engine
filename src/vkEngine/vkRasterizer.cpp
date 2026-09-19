#include "vkRasterizer.h"
#include "glm/gtc/type_ptr.hpp"
#include <core/Utils.h>
#include <fmt/base.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>


void vkRasterizer::Init(RasterizerInitInfo info)
{
    std::vector<vk::Format> depthFormatList {vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
    for(vk::Format& format : depthFormatList)
    {
        vk::FormatProperties2 formatProperties;
        formatProperties = info.physicalDevice.getFormatProperties2(format);
        if(formatProperties.formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            depthFormat = format;
            break;
        }
    }
    mWindowDim.x = info.wWidth;
    mWindowDim.y = info.wHeight;
    
    CreateDepthImg(info);

    

    for(int i = 0; i < maxFramesInFlight; i++)
    {
        vk::BufferCreateInfo uBufferCi;
        uBufferCi.size = sizeof(ShaderData);
        uBufferCi.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        VmaAllocationCreateInfo uBufferAllocCi
        {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vmaCreateBuffer(info.allocator, reinterpret_cast<VkBufferCreateInfo*>(&uBufferCi), &uBufferAllocCi, reinterpret_cast<VkBuffer*>(&mShaderBuffers[i].buffer),
        &mShaderBuffers[i].allocation, &mShaderBuffers[i].allocInfo);

        vk::BufferDeviceAddressInfo uBufferAdInfo;
        uBufferAdInfo.buffer = mShaderBuffers[i].buffer;
        mShaderBuffers[i].adress = info.device.getBufferAddress(uBufferAdInfo);
    }

    vk::CommandPoolCreateInfo commandPoolCi;
    commandPoolCi.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCi.queueFamilyIndex = info.queueFamily;
    mCommandPool = info.device.createCommandPool(commandPoolCi);

    context.alloc = info.allocator;
    context.commandPool = mCommandPool;
    context.device = info.device;
    context.physicalDevice = info.physicalDevice;
    context.queue = info.queue;
    context.queueFamily = info.queueFamily;

    vk::CommandBufferAllocateInfo cbAllocInfo;
    cbAllocInfo.commandPool = mCommandPool;
    cbAllocInfo.commandBufferCount = maxFramesInFlight;

    std::vector<vk::CommandBuffer> cbs = info.device.allocateCommandBuffers(cbAllocInfo);
    std::copy(cbs.begin(), cbs.end(), mCommandBuffers.begin());

    std::vector<char> vertShaderCode = Utils::ReadFileBinary("../Shaders/rasterizerVert.spv");
    std::vector<char> fragShaderCode = Utils::ReadFileBinary("../Shaders/rasterizerFrag.spv");

    vk::ShaderModuleCreateInfo vertShaderModuleCi;
    vertShaderModuleCi.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
    vertShaderModuleCi.codeSize = vertShaderCode.size();
    vk::ShaderModule vertShaderModule = info.device.createShaderModule(vertShaderModuleCi);

    vk::ShaderModuleCreateInfo fragShaderModuleCi;
    fragShaderModuleCi.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
    fragShaderModuleCi.codeSize = fragShaderCode.size();
    vk::ShaderModule fragShaderModule = info.device.createShaderModule(fragShaderModuleCi);

    vk::SamplerCreateInfo samplerCi;
    samplerCi.setMagFilter(vk::Filter::eLinear)
    .setMinFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge);

    texturSampler = info.device.createSampler(samplerCi);

    InitDescPool(context);
    WriteDynamicDescriptors(context);
    mLastModelSize = info.rs->scene.models.size();
    CreateGraphicsPipeline(info, vertShaderModule, fragShaderModule);


}


#define check(a) \
if(a != vk::Result::eSuccess) \
    fmt::println("check error in {}", __LINE__)

void vkRasterizer::Render(RasterizerRenderInfo info)
{

    if(info.rs->scene.models.size() != mLastModelSize)
    {
        WriteDynamicDescriptors(context);
        mLastModelSize = info.rs->scene.models.size();
    }

    // if(mVkScene->boneTransforms.size != 0)
    // {
    //     std::vector<vkUtils::vkBone> bones;
    //     vkUtils::evaluateBoneTransforms(info.rs->scene, bones);
    //     if(bones.empty())
    //         bones.push_back({glm::mat4(1.0f)});
        
    //     mVkScene->bones = bones;
    //     std::memcpy(mVkScene->boneTransforms.allocInfo.pMappedData, bones.data(), mVkScene->boneTransforms.size);
    //     // vkUtils::UpdateVertices(info.rs->scene, *mVkScene);
    // }

    info.device.waitForFences(1, info.fence, vk::True, UINT64_MAX);
    info.device.resetFences(1, info.fence);
    auto cb = mCommandBuffers[info.frameIdx];
    cb.reset();

    vk::CommandBufferBeginInfo cbBi{};
    cbBi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cb.begin(cbBi);

    std::array<vk::ImageMemoryBarrier2, 2> outputBarriers;
    outputBarriers[0].setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setSrcAccessMask(vk::AccessFlagBits2::eNone)
    .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
    .setImage(info.drawImgs[info.frameIdx]);
    outputBarriers[0].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    outputBarriers[0].subresourceRange.levelCount = 1;
    outputBarriers[0].subresourceRange.layerCount = 1;

    outputBarriers[1].setSrcStageMask(vk::PipelineStageFlagBits2::eLateFragmentTests)
    .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
    .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
    .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
    .setImage(mDepthImage);
    outputBarriers[1].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    outputBarriers[1].subresourceRange.levelCount = 1;
    outputBarriers[1].subresourceRange.layerCount = 1;

    vk::DependencyInfo barrierDependencyInfo;
    barrierDependencyInfo.imageMemoryBarrierCount = 2;
    barrierDependencyInfo.setPImageMemoryBarriers(outputBarriers.data());

    cb.pipelineBarrier2(barrierDependencyInfo);

    vk::RenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.setImageView(info.drawImgViews[info.frameIdx])
    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eStore)
    .clearValue.setColor({0.0f, 0.0f, 0.2f, 1.0f});

    vk::RenderingAttachmentInfo depthAttachmentInfo{};
    depthAttachmentInfo.setImageView(mDepthImageView)
    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eDontCare)
    .clearValue.setDepthStencil({1.0f, 0});

    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea.extent.width = static_cast<uint32_t>(info.rs->ImgWidth);
    renderingInfo.renderArea.extent.height = static_cast<uint32_t>(info.rs->ImgHeight);
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;

    cb.beginRendering(renderingInfo);

    vk::Viewport vp{};
    vp.setWidth(static_cast<uint32_t>(info.rs->ImgWidth))
    .setHeight(static_cast<uint32_t>(info.rs->ImgHeight))
    .setMinDepth(0.0f)
    .setMaxDepth(1.0f);
    vk::Rect2D scissor;
    scissor.extent.width = static_cast<uint32_t>(info.rs->ImgWidth);
    scissor.extent.height = static_cast<uint32_t>(info.rs->ImgHeight);

    cb.setViewport(0,vp);
    cb.setScissor(0, scissor);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, mPip);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,pipLayout,0, 1, &descSet, 0, nullptr);
    for(int i = 0; i < mVkScene->vkMeshes.size(); i++)
    {
        ShaderData shaderData;
        shaderData.mvp = info.rs->scene.camera.GetProjection() * info.rs->scene.camera.GetView() 
        * *mVkScene->vkModels[mVkScene->vkMeshes[i].modelIdx].modelMat * ((*mVkScene->vkModels[mVkScene->vkMeshes[i].modelIdx].nodeData)[mVkScene->vkMeshes[i].nodeIdx].transform);

        vk::DeviceSize vOffset{0};
        cb.bindVertexBuffers(0, 1, &mVkScene->vkMeshes[i].buffer, &vOffset);
        cb.bindIndexBuffer(mVkScene->vkMeshes[i].buffer, mVkScene->vkMeshes[i].vBufSize, vk::IndexType::eUint32);
        pushConstantsStruct constants;
        constants.mat = shaderData.mvp;
        constants.albedo = info.rs->scene.materials[*mVkScene->vkMeshes[i].matIdx].albedo;
        constants.textId = info.rs->scene.materials[*mVkScene->vkMeshes[i].matIdx].albedoTexture;

        cb.pushConstants(pipLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(constants), &constants);

        cb.drawIndexed(mVkScene->vkMeshes[i].indexCount, 1, 0, 0, 0);
    }
    cb.endRendering();
    
    vk::ImageMemoryBarrier2 midBarrier;
    midBarrier.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
    .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
    .setDstAccessMask(vk::AccessFlagBits2::eShaderSampledRead)
    .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
    .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
    .setImage(info.drawImgs[info.frameIdx]);
    midBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    midBarrier.subresourceRange.levelCount = 1;
    midBarrier.subresourceRange.layerCount = 1;
    vk::DependencyInfo midBarriersDepInfo{};
    midBarriersDepInfo.setImageMemoryBarrierCount(1)
    .setPImageMemoryBarriers(&midBarrier);

    cb.pipelineBarrier2(midBarriersDepInfo);

    cb.end();
}

void vkRasterizer::UpdateSwapchain(const VmaAllocator& allocator,int width, int height,int imageCount, const vk::Device& device)
{
    device.destroyImage(mDepthImage);
    device.destroyImageView(mDepthImageView);

    vk::ImageCreateInfo depthImageCI;
    depthImageCI.extent.depth = 1;
    depthImageCI.imageType = vk::ImageType::e2D;
    depthImageCI.format = depthFormat;
    depthImageCI.extent.width = static_cast<uint32_t>(width);
    depthImageCI.extent.height = static_cast<uint32_t>(height);
    depthImageCI.mipLevels = 1;
    depthImageCI.arrayLayers = 1;
    depthImageCI.samples = vk::SampleCountFlagBits::e1;
    depthImageCI.tiling = vk::ImageTiling::eOptimal;
    depthImageCI.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depthImageCI.initialLayout = vk::ImageLayout::eUndefined;
    VmaAllocation alloc;
    VmaAllocationCreateInfo allocCi
    {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO  
    };
    vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo*>(&depthImageCI), &allocCi, reinterpret_cast<VkImage*>(&mDepthImage)
    ,&alloc, nullptr);

    vk::ImageViewCreateInfo depthViewCi;
    depthViewCi.setImage(mDepthImage)
    .setViewType(vk::ImageViewType::e2D)
    .setComponents({vk::ComponentSwizzle::eIdentity,
    vk::ComponentSwizzle::eIdentity,
    vk::ComponentSwizzle::eIdentity,
    vk::ComponentSwizzle::eIdentity})
    .setFormat(depthFormat)
    .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
    .setLayerCount(1)
    .setLevelCount(1);

    mDepthImageView = device.createImageView(depthViewCi);
    mWindowDim.x = width;
    mWindowDim.y = height;
}


void vkRasterizer::CreateDepthImg(const RasterizerInitInfo& info)
{
    vk::ImageCreateInfo depthImageCI;
    depthImageCI.extent.depth = 1;
    depthImageCI.imageType = vk::ImageType::e2D;
    depthImageCI.format = depthFormat;
    depthImageCI.extent.width = static_cast<uint32_t>(info.rs->ImgWidth);
    depthImageCI.extent.height = static_cast<uint32_t>(info.rs->ImgHeight);
    depthImageCI.mipLevels = 1;
    depthImageCI.arrayLayers = 1;
    depthImageCI.samples = vk::SampleCountFlagBits::e1;
    depthImageCI.tiling = vk::ImageTiling::eOptimal;
    depthImageCI.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depthImageCI.initialLayout = vk::ImageLayout::eUndefined;

    VmaAllocationCreateInfo allocCI{};
    allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCI.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(
        info.allocator, 
        reinterpret_cast<const VkImageCreateInfo*>(&depthImageCI),
        &allocCI,
        reinterpret_cast<VkImage*>(&mDepthImage),
        &depthImageAllocation,
        nullptr
    );

    vk::ImageViewCreateInfo depthViewCI;
    depthViewCI.image = mDepthImage;
    depthViewCI.viewType = vk::ImageViewType::e2D;
    depthViewCI.format = depthFormat;
    depthViewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depthViewCI.subresourceRange.levelCount = 1;
    depthViewCI.subresourceRange.layerCount = 1;

    mDepthImageView = info.device.createImageView(depthViewCI);
}

void vkRasterizer::CreateGraphicsPipeline(const RasterizerInitInfo& info, vk::ShaderModule vertModule, vk::ShaderModule fragModule)
{
    vk::PushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    pushConstantRange.size = sizeof(pushConstantsStruct);

    vk::PipelineLayoutCreateInfo pipelineLayoutCi;
    pipelineLayoutCi.setLayoutCount = 1;
    pipelineLayoutCi.pSetLayouts = &setLayout;
    pipelineLayoutCi.pushConstantRangeCount = 1;
    pipelineLayoutCi.pPushConstantRanges = &pushConstantRange;

    pipLayout = info.device.createPipelineLayout(pipelineLayoutCi);

    vk::VertexInputBindingDescription vertexBinding;
    vertexBinding.setBinding(0)
    .setStride(sizeof(Vertex))
    .setInputRate(vk::VertexInputRate::eVertex);

    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    vertexAttributes.resize(7);
    vertexAttributes[0].setLocation(0)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat);
    vertexAttributes[1].setLocation(1)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32Sfloat)
    .setOffset(offsetof(Vertex, texCoords));
    vertexAttributes[2].setLocation(2)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat)
    .setOffset(offsetof(Vertex, normals));
    vertexAttributes[3].setLocation(3)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat)
    .setOffset(offsetof(Vertex, tangent));
    vertexAttributes[4].setLocation(4)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat)
    .setOffset(offsetof(Vertex, bitangent));
    vertexAttributes[5].setLocation(5)
    .setBinding(0)
    .setFormat(vk::Format::eR32Uint)
    .setOffset(offsetof(Vertex, boneOffset));
    vertexAttributes[6].setLocation(6)
    .setBinding(0)
    .setFormat(vk::Format::eR32Uint)
    .setOffset(offsetof(Vertex, boneCount));

    vk::PipelineVertexInputStateCreateInfo vertexInputState;
    vertexInputState.setVertexBindingDescriptionCount(1)
    .setVertexBindingDescriptions(vertexBinding)
    .setVertexAttributeDescriptionCount(static_cast<uint32_t>(vertexAttributes.size()))
    .setPVertexAttributeDescriptions(vertexAttributes.data());

    vk::PipelineInputAssemblyStateCreateInfo inputAssemplyState;
    inputAssemplyState.setTopology(vk::PrimitiveTopology::eTriangleList);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.resize(2);
    shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex)
    .setModule(vertModule)
    .setPName("main");
    shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment)
    .setModule(fragModule)
    .setPName("main");

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.setViewportCount(1)
    .setScissorCount(1);

    std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport,vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.setDynamicStateCount(2)
    .setDynamicStates(dynamicStates);

    vk::PipelineDepthStencilStateCreateInfo depthStenchilState;
    depthStenchilState.setDepthTestEnable(vk::True)
    .setDepthWriteEnable(vk::True)
    .setDepthCompareOp(vk::CompareOp::eLessOrEqual);

    vk::PipelineRenderingCreateInfo renderCi;
    renderCi.setColorAttachmentCount(1)
    .setColorAttachmentFormats(info.imageFormat)
    .setDepthAttachmentFormat(depthFormat);

    vk::PipelineColorBlendAttachmentState blendAttatchment;
    blendAttatchment.colorWriteMask = vk::ColorComponentFlagBits::eR
                                    | vk::ColorComponentFlagBits::eG
                                    | vk::ColorComponentFlagBits::eB
                                    | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo blendState;
    blendState.setAttachmentCount(1)
    .setAttachments(blendAttatchment);
    vk::PipelineRasterizationStateCreateInfo rastState;
    rastState.lineWidth = 1.0f;
    vk::PipelineMultisampleStateCreateInfo multisampleState;
    multisampleState.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::GraphicsPipelineCreateInfo pipCi;
    pipCi.pNext = &renderCi;
    pipCi.setStageCount(2)
    .setStages(shaderStages)
    .setPVertexInputState(&vertexInputState)
    .setPInputAssemblyState(&inputAssemplyState)
    .setPViewportState(&viewportState)
    .setPRasterizationState(&rastState)
    .setPColorBlendState(&blendState)
    .setPMultisampleState(&multisampleState)
    .setPDepthStencilState(&depthStenchilState)
    .setPDynamicState(&dynamicState)
    .setLayout(pipLayout);

    mPip = info.device.createGraphicsPipelines(nullptr, {pipCi}).value[0];
}


void vkRasterizer::destroy(vk::Device device, VmaAllocator alloc)
{
    vmaDestroyImage(alloc, static_cast<VkImage>(mDepthImage), depthImageAllocation);
    device.destroyImageView(mDepthImageView);
    device.destroyPipelineLayout(pipLayout);
    device.destroyPipeline(mPip);
    for (int i = 0; i < mShaderBuffers.size(); i++)
    {
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(mShaderBuffers[i].buffer), mShaderBuffers[i].allocation);
    }
    device.freeCommandBuffers(mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());

    device.destroyCommandPool(mCommandPool);
}


void vkRasterizer::InitDescPool(VkContext context)
{
    
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.resize(1);
    bindings[0].setBinding(0)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setDescriptorCount(100)
    .setStageFlags(vk::ShaderStageFlagBits::eFragment);
    std::vector<vk::DescriptorBindingFlags> bindingFlags = 
    {
        vk::DescriptorBindingFlagBits::ePartiallyBound
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
    flagsCreateInfo.setBindingFlags(bindingFlags);

    vk::DescriptorSetLayoutCreateInfo layoutCi;
    layoutCi.setBindingCount(bindings.size())
    .setPNext(&flagsCreateInfo)
    .setPBindings(bindings.data());

    setLayout = context.device.createDescriptorSetLayout(layoutCi);
        std::vector<vk::DescriptorPoolSize> poolSizes = 
    {
        {
            vk::DescriptorType::eCombinedImageSampler,
            100
        }
    };


    vk::DescriptorPoolCreateInfo descPoolCi;
    descPoolCi.setMaxSets(1)
    .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
    .setPPoolSizes(poolSizes.data())
    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    descPool = context.device.createDescriptorPool(descPoolCi);
    
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(descPool)
    .setDescriptorSetCount(1)
    .setPSetLayouts(&setLayout);

    descSet = context.device.allocateDescriptorSets(allocInfo).front();
}

void vkRasterizer::WriteDynamicDescriptors(VkContext context)
{
    std::vector<vk::DescriptorImageInfo> textureInfos(mVkScene->vkTextures.size());
    for(int i = 0; i < mVkScene->vkTextures.size(); i++)
    {
        vk::DescriptorImageInfo textureImageInfo;
        textureImageInfo.setSampler(texturSampler)
        .setImageView(mVkScene->vkTextures[i].view)
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        textureInfos[i] = textureImageInfo;
    }

    std::vector<vk::WriteDescriptorSet> descWrites;
    if(!textureInfos.empty())
    {
        vk::WriteDescriptorSet textureDescWrite;
        textureDescWrite.setDstSet(descSet)
        .setDstBinding(0)
        .setDescriptorCount(textureInfos.size())
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setPImageInfo(textureInfos.data());
        descWrites.push_back(textureDescWrite);
    }

    context.device.updateDescriptorSets(static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);
}
