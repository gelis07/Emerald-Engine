#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>


/*
    Initializing vulkan device for raytracing cores
    Bundles together a physical device, compute queue and virtual device
*/

namespace core {
class device {
    public:
    device(vk::Instance instance);

    vk::Device getDevice() const { return mDevice; }
    vk::Queue getQueue() const { return mComputeQueue; }
    vk::PhysicalDevice getPhysicalDevice() const {return mPhysicalDevice;}
    uint32_t getComputeFamily() const { return mQueueFamilyCompute; }
    vk::detail::DispatchLoaderDynamic getLoader() const {return dynamicDispatchLoader;}
    private:
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mDevice;
    vk::Queue mComputeQueue;
    vk::detail::DispatchLoaderDynamic dynamicDispatchLoader;
    uint32_t mQueueFamilyCompute;
};
} // namespace core