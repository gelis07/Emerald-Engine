#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>


/*
Creates vma allocator.
*/

namespace core
{
    class allocator
    {
    public:
        allocator(vk::Instance instance, vk::PhysicalDevice physicalDevice,
        vk::Device device);

        VmaAllocator getAlloc() const {return mAlloc;}
    private:
        VmaAllocator mAlloc;
    };
}