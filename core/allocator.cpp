#define VMA_IMPLEMENTATION
#include "allocator.hpp"


namespace core
{
    allocator::allocator(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                        vk::Device device) {
        VmaVulkanFunctions vkFunctions;
        vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCI{};
        allocatorCI.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        allocatorCI.physicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
        allocatorCI.device = static_cast<VkDevice>(device);
        allocatorCI.instance = static_cast<VkInstance>(instance);
        allocatorCI.pVulkanFunctions = nullptr;
        allocatorCI.vulkanApiVersion = VK_API_VERSION_1_4;
        vmaCreateAllocator(&allocatorCI, &mAlloc);
    }
}