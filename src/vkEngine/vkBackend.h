#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <functional>
struct TextureVk
{
    vk::Image image;
    VmaAllocation alloc;
    vk::ImageView view;
    std::string path ="";
};


struct VkImageCreateData
{
    vk::CommandPool commandPool;
    vk::Device device;
    VmaAllocator allocator;
    vk::Queue queue;

    //Settings for hdr loading.
    vk::Format format = vk::Format::eR8G8B8A8Srgb;
    uint32_t sizePerByte = 1;
};

struct VkUtilBuffer
{
    vk::Buffer buffer;
    VmaAllocationInfo allocInfo;
    VmaAllocation allocation;
    vk::DeviceAddress adress;
};


namespace vkUtils
{
    TextureVk LoadTexture(int width, int height, int channels, unsigned char* data, const VkImageCreateData& vkData);
    void ExecuteSingleTimeCb(const vk::Device& device, const vk::CommandPool& commandPool, const vk::Queue& queue,const std::function<void(const vk::CommandBuffer &singleTimeCb)>& c);
};
