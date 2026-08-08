#define VMA_IMPLEMENTATION
#include "vkBackend.h"



TextureVk vkUtils::LoadTexture(int width, int height, int channels, unsigned char* data, const VkImageCreateData& vkData)
{
    TextureVk texture;


    vk::ImageCreateInfo imageCi;
    imageCi.imageType = vk::ImageType::e2D;
    imageCi.format = vkData.format;
    imageCi.extent.width = width;
    imageCi.extent.height = height;
    imageCi.extent.depth = 1;
    imageCi.mipLevels = 1;
    imageCi.arrayLayers = 1;
    imageCi.samples = vk::SampleCountFlagBits::e1;
    imageCi.tiling = vk::ImageTiling::eOptimal;
    imageCi.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageCi.initialLayout = vk::ImageLayout::eUndefined;
    VmaAllocationCreateInfo allocCi{};
    allocCi.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateImage(vkData.allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCi), 
    &allocCi, reinterpret_cast<VkImage*>(&texture.image), &texture.alloc, nullptr);


    vk::ImageViewCreateInfo viewCi;
    viewCi.image = texture.image;
    viewCi.viewType = vk::ImageViewType::e2D;
    viewCi.format = imageCi.format;
    viewCi.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewCi.subresourceRange.levelCount = 1;
    viewCi.subresourceRange.layerCount = 1;

    texture.view = vkData.device.createImageView(viewCi);

    vk::Buffer imageSrcBuffer{};
    VmaAllocation imgSrcAllocation{};
    vk::BufferCreateInfo imgSrcbufferCi;
    imgSrcbufferCi.size = width * height * 4 * vkData.sizePerByte;
    imgSrcbufferCi.usage = vk::BufferUsageFlagBits::eTransferSrc;
    VmaAllocationCreateInfo imgSrcAllocCi{};
    imgSrcAllocCi.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    imgSrcAllocCi.usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(vkData.allocator, reinterpret_cast<const VkBufferCreateInfo*>(&imgSrcbufferCi), &imgSrcAllocCi,
    reinterpret_cast<VkBuffer*>(&imageSrcBuffer), &imgSrcAllocation, &allocInfo);

    memcpy(allocInfo.pMappedData, data, imgSrcbufferCi.size);

    vkUtils::ExecuteSingleTimeCb(vkData.device, vkData.commandPool, vkData.queue, [&](const vk::CommandBuffer& singleTimeCb)
    {
        vk::ImageMemoryBarrier2 barrierTexImage;
        barrierTexImage.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrierTexImage.srcStageMask = vk::PipelineStageFlagBits2::eNone;
        barrierTexImage.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrierTexImage.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrierTexImage.oldLayout = vk::ImageLayout::eUndefined;
        barrierTexImage.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrierTexImage.image = texture.image;
        barrierTexImage.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrierTexImage.subresourceRange.levelCount = 1;
        barrierTexImage.subresourceRange.layerCount = 1;

        vk::DependencyInfo barrierTexInfo;
        barrierTexInfo.imageMemoryBarrierCount = 1;
        barrierTexInfo.pImageMemoryBarriers = &barrierTexImage;

        singleTimeCb.pipelineBarrier2(barrierTexInfo);
        vk::BufferImageCopy bufferCopy;
        bufferCopy.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(0);
        bufferCopy.imageOffset.setX(0).setY(0).setZ(0);
        bufferCopy.imageExtent.setWidth(width).setHeight(height).setDepth(1);

        singleTimeCb.copyBufferToImage(imageSrcBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal, 1, &bufferCopy);

        vk::ImageMemoryBarrier2 barrierTexImageToRead;
        barrierTexImageToRead.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrierTexImageToRead.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrierTexImageToRead.dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
        barrierTexImageToRead.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        barrierTexImageToRead.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrierTexImageToRead.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrierTexImageToRead.image = texture.image;
        barrierTexImageToRead.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrierTexImageToRead.subresourceRange.levelCount = 1;
        barrierTexImageToRead.subresourceRange.layerCount = 1;

        vk::DependencyInfo barrierTexReadInfo;
        barrierTexReadInfo.imageMemoryBarrierCount = 1;
        barrierTexReadInfo.pImageMemoryBarriers = &barrierTexImageToRead;
        singleTimeCb.pipelineBarrier2(barrierTexReadInfo);
    });
    

    return texture;
}



void vkUtils::ExecuteSingleTimeCb(const vk::Device& device, const vk::CommandPool& commandPool, const vk::Queue& queue,const std::function<void(const vk::CommandBuffer &singleTimeCb)>& c)
{
    vk::CommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.setCommandPool(commandPool)
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandBufferCount(1);

    vk::CommandBuffer singleBuffer = device.allocateCommandBuffers(cbAllocInfo).front();
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    singleBuffer.begin(beginInfo);
    c(singleBuffer);
    singleBuffer.end();

    vk::SubmitInfo subInfo{};
    subInfo.setWaitSemaphoreCount(0)
    .setPWaitSemaphores(nullptr)
    .setCommandBufferCount(1)
    .setPCommandBuffers(&singleBuffer);
    vk::Fence fence;
    fence = device.createFence({});

    queue.submit(subInfo, fence);

    vk::Result result = device.waitForFences(1, &fence, vk::True, UINT64_MAX);

    device.destroyFence(fence);
    device.freeCommandBuffers(commandPool, singleBuffer);
}