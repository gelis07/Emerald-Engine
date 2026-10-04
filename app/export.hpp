#pragma once
#include <context.hpp>
#include <string>
#include <utils.hpp>
#include <fpng.h>

inline void ExportToPng(core::context context
, vk::Image image
, vk::CommandPool commandPool
, const std::string& filename
, uint32_t width
, uint32_t height)
{
    uint32_t bytesPerPixel = 4; // Assuming 8-bit channels (RGBA/BGRA)
    vk::BufferCreateInfo dstBufferCi;
    dstBufferCi.setSize(width * height * bytesPerPixel);
    dstBufferCi.setUsage(vk::BufferUsageFlagBits::eTransferDst);

    VmaAllocation dstBufferAlloc;
    VmaAllocationInfo dstBufferAllocInfo;
    VmaAllocationCreateInfo dstBufferAllocCi{};
    dstBufferAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    dstBufferAllocCi.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    vk::Buffer dstBuffer;
    vmaCreateBuffer(context.alloc.getAlloc(), reinterpret_cast<VkBufferCreateInfo*>(&dstBufferCi), &dstBufferAllocCi, reinterpret_cast<VkBuffer*>(&dstBuffer),
    &dstBufferAlloc, &dstBufferAllocInfo);

    core::ExecuteSingleTimeCb(context.device.getDevice(), commandPool, context.device.getQueue(), [&](const vk::CommandBuffer& singleTimeCb)
    {
        vk::ImageMemoryBarrier2 bImgToDst;
        bImgToDst.setImage(image)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setOldLayout(vk::ImageLayout::eGeneral)
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
        .setHeight(height)
        .setWidth(width);

        singleTimeCb.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, dstBuffer, imgCopy);

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
        dstBufferAllocInfo.pMappedData,
        width,
        height,
        bytesPerPixel
    );
}