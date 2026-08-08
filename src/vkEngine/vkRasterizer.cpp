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

    vk::SemaphoreCreateInfo semaphoreCi;
    vk::FenceCreateInfo fenceCi;
    fenceCi.flags = vk::FenceCreateFlagBits::eSignaled;

    for(int i = 0; i < maxFramesInFlight; i++)
    {
        mFences[i] = info.device.createFence(fenceCi);
        mImageAcquiredSemaphores[i] = info.device.createSemaphore(semaphoreCi);
    }
    mRenderCompleteSemaphores.resize(info.swapchainImgCount);
    for(int i = 0; i < info.swapchainImgCount; i++)
    {
        mRenderCompleteSemaphores[i] = info.device.createSemaphore(semaphoreCi);
    }


    vk::CommandPoolCreateInfo commandPoolCi;
    commandPoolCi.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCi.queueFamilyIndex = info.queueFamily;
    mCommandPool = info.device.createCommandPool(commandPoolCi);

        vkUtils::vkScene vkscene = vkUtils::LoadScene(info.device, info.allocator, mCommandPool, info.queue,info.rs->scene);
        mVkModels = vkscene.vkModels;
    lastModelCount = info.rs->scene.models.size();

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

    CreateGraphicsPipeline(info, vertShaderModule, fragShaderModule);
}


#define check(a) \
if(a != vk::Result::eSuccess) \
    fmt::println("check error in {}", __LINE__)

void vkRasterizer::Render(RasterizerRenderInfo info)
{
    check(info.device.waitForFences(1, &mFences[info.frameIdx], vk::True, UINT64_MAX));
    check(info.device.resetFences(1, &mFences[info.frameIdx]));

    uint32_t imageIdx = info.device.acquireNextImageKHR(info.swapchain, UINT64_MAX, mImageAcquiredSemaphores[info.frameIdx]).value;

    // if(info.rs->scene.models.size() != lastModelCount)
    // {
    //     vkUtils::vkScene vkscene = vkUtils::LoadScene(info.device, info.alloc, mCommandPool, info.queue,info.rs->scene);
    //     mVkModels = vkscene.vkModels;
    //     lastModelCount = info.rs->scene.models.size();
    // }

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
    for(int i = 0; i < mVkModels.size(); i++)
    {
        ShaderData shaderData;
        shaderData.mvp = info.rs->scene.camera.GetProjection() * info.rs->scene.camera.GetView() * info.rs->scene.models[i].model;

        vk::DeviceSize vOffset{0};
        cb.bindVertexBuffers(0, 1, &mVkModels[i].buffer, &vOffset);
        cb.bindIndexBuffer(mVkModels[i].buffer, mVkModels[i].vBufSize, vk::IndexType::eUint32);

        cb.pushConstants(pipLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), glm::value_ptr(shaderData.mvp));

        cb.drawIndexed(mVkModels[i].indexCount, 1, 0, 0, 0);
    }
    cb.endRendering();

    std::array<vk::ImageMemoryBarrier2, 2> midBarriers;
    midBarriers[0].setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setSrcAccessMask(vk::AccessFlagBits2::eNone)
    .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
    .setImage(info.swapchainImgs[imageIdx]);
    midBarriers[0].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    midBarriers[0].subresourceRange.levelCount = 1;
    midBarriers[0].subresourceRange.layerCount = 1;

    midBarriers[1].setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
    .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
    .setDstAccessMask(vk::AccessFlagBits2::eShaderSampledRead)
    .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
    .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
    .setImage(info.drawImgs[info.frameIdx]);
    midBarriers[1].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    midBarriers[1].subresourceRange.levelCount = 1;
    midBarriers[1].subresourceRange.layerCount = 1;


    vk::DependencyInfo midBarriersDepInfo{};
    midBarriersDepInfo.setImageMemoryBarrierCount(2)
    .setPImageMemoryBarriers(midBarriers.data());


    cb.pipelineBarrier2(midBarriersDepInfo);

    colorAttachmentInfo.setLoadOp(vk::AttachmentLoadOp::eLoad);
    colorAttachmentInfo.setImageView(info.swapchainImgViews[imageIdx]);
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.renderArea.extent.width = static_cast<uint32_t>(mWindowDim.x);
    renderingInfo.renderArea.extent.height = static_cast<uint32_t>(mWindowDim.y);
    cb.beginRendering(renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cb));
    cb.endRendering();

    vk::ImageMemoryBarrier2 barrierPresent{};
    barrierPresent.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrierPresent.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrierPresent.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrierPresent.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrierPresent.oldLayout = vk::ImageLayout::eAttachmentOptimal;
    barrierPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrierPresent.image = info.swapchainImgs[imageIdx];
    barrierPresent.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrierPresent.subresourceRange.levelCount = 1;
    barrierPresent.subresourceRange.layerCount = 1;

    vk::DependencyInfo barrierPresentDepInfo{};
    barrierPresentDepInfo.setImageMemoryBarrierCount(1)
    .setPImageMemoryBarriers(&barrierPresent);
    cb.pipelineBarrier2(barrierPresentDepInfo);

    cb.end();

    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo;
    submitInfo.setWaitSemaphoreCount(1)
    .setPWaitSemaphores(&mImageAcquiredSemaphores[info.frameIdx])
    .setPWaitDstStageMask(&waitStages)
    .setCommandBufferCount(1)
    .setPCommandBuffers(&cb)
    .setSignalSemaphoreCount(1)
    .setPSignalSemaphores(&mRenderCompleteSemaphores[imageIdx]);
    info.queue.submit(submitInfo, mFences[info.frameIdx]);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphoreCount(1)
    .setPWaitSemaphores(&mRenderCompleteSemaphores[imageIdx])
    .setSwapchainCount(1)
    .setPSwapchains(&info.swapchain)
    .setPImageIndices(&imageIdx);

    check(info.queue.presentKHR(presentInfo));
}

void vkRasterizer::UpdateSwapchain(const VmaAllocator& allocator,int width, int height,int imageCount, const vk::Device& device)
{
    for (int i = 0; i < imageCount; i++)
    {
        device.destroySemaphore(mRenderCompleteSemaphores[i]);
    }
    mRenderCompleteSemaphores.resize(imageCount);
    for(int i = 0; i < imageCount; i++)
    {
        mRenderCompleteSemaphores[i] = device.createSemaphore({});
    }

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
    depthImageCI.extent.width = static_cast<uint32_t>(info.wWidth);
    depthImageCI.extent.height = static_cast<uint32_t>(info.wHeight);
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
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushConstantRange.size = sizeof(glm::mat4);

    vk::PipelineLayoutCreateInfo pipelineLayoutCi;
    pipelineLayoutCi.setLayoutCount = 0;
    pipelineLayoutCi.pSetLayouts = nullptr;
    pipelineLayoutCi.pushConstantRangeCount = 1;
    pipelineLayoutCi.pPushConstantRanges = &pushConstantRange;

    pipLayout = info.device.createPipelineLayout(pipelineLayoutCi);

    vk::VertexInputBindingDescription vertexBinding;
    vertexBinding.setBinding(0)
    .setStride(sizeof(Vertex))
    .setInputRate(vk::VertexInputRate::eVertex);

    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    vertexAttributes.resize(3);
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
    for(int i = 0; i < mFences.size(); i++)
    {
        device.destroyFence(mFences[i]);
    }
    for(int i = 0; i < mImageAcquiredSemaphores.size(); i++)
    {
        device.destroySemaphore(mImageAcquiredSemaphores[i]);
    }
    for(int i = 0; i < mRenderCompleteSemaphores.size(); i++)
    {
        device.destroySemaphore(mRenderCompleteSemaphores[i]);
    }
    for(int i = 0; i < mVkModels.size(); i++)
    {
        vmaDestroyBuffer(alloc, static_cast<VkBuffer>(mVkModels[i].buffer), mVkModels[i].bufferAllocation);
    }
    device.destroyCommandPool(mCommandPool);
}