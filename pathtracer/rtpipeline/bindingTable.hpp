#pragma once 
#include <context.hpp>
#include <buffer.hpp>
#include <utils.hpp>
namespace engine
{
    class bindingTable
    {
        public:
        void Init(core::context context
            , vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties, vk::Pipeline pip);

        vk::StridedDeviceAddressRegionKHR getRayGenAddressRegion() const
        {return core::checkVulkanNull(mSbtRayGenAddressRegion);}
        vk::StridedDeviceAddressRegionKHR getMissAddressRegion() const
        {return core::checkVulkanNull(mSbtMissAddressRegion);}
        vk::StridedDeviceAddressRegionKHR getChitAddressRegion() const
        {return core::checkVulkanNull(mSbtCHitAddresRegion);}
        private:
        core::buffer buffer;
        vk::StridedDeviceAddressRegionKHR mSbtRayGenAddressRegion, mSbtMissAddressRegion, mSbtCHitAddresRegion;
    };
}