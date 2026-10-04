#include "device.hpp"

namespace core
{
    device::device(vk::Instance instance)
    {
        std::vector<vk::PhysicalDevice> devices =
            instance.enumeratePhysicalDevices();
        mPhysicalDevice = devices[0];

        std::vector<vk::QueueFamilyProperties> queueFamilies =
            mPhysicalDevice.getQueueFamilyProperties();

        bool foundGraphics = false;
        bool foundCompute = false;
        for (int i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
            mQueueFamilyCompute = i;
            foundCompute = true;
            }
            if (foundCompute && foundGraphics)
            break;
        }
        const float qfPriorities{1.0f};
        const float qfPrioritiesCompute{1.0f};

        vk::DeviceQueueCreateInfo computeQueueCi;
        computeQueueCi.setQueueFamilyIndex(mQueueFamilyCompute)
            .setQueueCount(1)
            .setPQueuePriorities(&qfPrioritiesCompute);

        vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
        rayQueryFeatures.rayQuery = vk::True;

        vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{};
        accelFeature.accelerationStructure = true;
        accelFeature.pNext = &rayQueryFeatures;

        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipFeatures{};
        rtPipFeatures.rayTracingPipeline = true;
        rtPipFeatures.pNext = &accelFeature;

        vk::PhysicalDeviceVulkan12Features enabledV12Features;
        enabledV12Features.pNext = &rtPipFeatures;
        enabledV12Features.descriptorIndexing = true;
        enabledV12Features.shaderSampledImageArrayNonUniformIndexing = true;
        enabledV12Features.descriptorBindingVariableDescriptorCount = true;
        enabledV12Features.runtimeDescriptorArray = true;
        enabledV12Features.bufferDeviceAddress = true;
        enabledV12Features.descriptorBindingPartiallyBound = true;
        enabledV12Features.scalarBlockLayout = true;
        enabledV12Features.shaderStorageBufferArrayNonUniformIndexing = true;
        vk::PhysicalDeviceVulkan13Features enabledVk13Features;
        enabledVk13Features.pNext = &enabledV12Features;
        enabledVk13Features.synchronization2 = true;
        enabledVk13Features.dynamicRendering = true;

        vk::PhysicalDeviceFeatures2 enabledVk10Features;
        enabledVk10Features.pNext = &enabledVk13Features;
        enabledVk10Features.features.samplerAnisotropy = VK_TRUE;
        enabledVk10Features.features.geometryShader = vk::True;
        enabledVk10Features.features.shaderInt64 = vk::True;
        std::vector<const char *> deviceExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME};
        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.pNext = &enabledVk10Features;
        deviceCreateInfo.queueCreateInfoCount = 1;
        std::array<vk::DeviceQueueCreateInfo, 1> deviceQueueInfos = {
            computeQueueCi};
        deviceCreateInfo.pQueueCreateInfos = deviceQueueInfos.data();
        deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

        mDevice = mPhysicalDevice.createDevice(deviceCreateInfo);
        mDevice.getQueue(mQueueFamilyCompute, 0, &mComputeQueue);

        dynamicDispatchLoader = vk::detail::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, mDevice);
    }
}