#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>


/*
    Bundles together objects that go together with a buffer.
*/
namespace core
{
    struct buffer
    {
        vk::Buffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocInfo;
        uint32_t size;
    };
}