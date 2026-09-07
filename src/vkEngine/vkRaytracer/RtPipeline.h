#pragma once
#include <vkEngine/VkEngineApiBinding.h>

class RtPip
{
    public:
        void Init(VkContext context, vk::detail::DispatchLoaderDynamic loader, vk::DescriptorSetLayout* setLayout);
        void destroy();
        
        vk::Pipeline pip;
        vk::PipelineLayout pipLayout;
        vk::StridedDeviceAddressRegionKHR sbtRayGenAddressRegion, sbtMissAddressRegion, sbtCHitAddresRegion;
    private:
        vk::Device device;
        VmaAllocator alloc;
        vk::detail::DispatchLoaderDynamic dynamicDispatchLoader;

        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR mRtProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR mAsProperties{};


        vk::ShaderModule createShaderModule(vk::Device device, const std::string& path);
};