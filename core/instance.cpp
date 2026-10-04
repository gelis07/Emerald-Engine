#include "instance.hpp"
#include "utils.hpp"

namespace core
{

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {

        CORE_PRINT("-------Vulkan-------");
        CORE_PRINT("Vulkan Validation layers: {}", pCallbackData->pMessage);

        return VK_FALSE;
    }

    
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
        
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    instance::instance(std::vector<const char*> extensions)
    {
        mAppInfo.setPApplicationName("Raytracer")
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("Raytracer")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_4);

        #ifdef VALIDATION_LAYERS
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif
        vk::InstanceCreateInfo createInfo;
        createInfo.setPApplicationInfo(&mAppInfo);
        createInfo.setPEnabledExtensionNames(extensions);
        #ifdef VALIDATION_LAYERS
        std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};
            std::vector<vk::LayerProperties> availableLayers =
                vk::enumerateInstanceLayerProperties();
            for (const char *layerName : validationLayers) {
                bool layerFound = false;
                for (const auto &layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                    }
                }
                if (!layerFound) {
                    throw std::runtime_error("Validation layers are requested, but not "
                                            "available on this system!");
                }
            }
            createInfo.setPEnabledLayerNames(validationLayers);
            vk::DebugUtilsMessengerCreateInfoEXT messengerCi;
            messengerCi.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            messengerCi.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
            messengerCi.pfnUserCallback =
            reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(
                debugCallback);
                createInfo.pNext = &messengerCi;
        #endif
        mInstance = vk::createInstance(createInfo);
        #ifdef VALIDATION_LAYERS
            VkDebugUtilsMessengerEXT debugMessenger;
            if (CreateDebugUtilsMessengerEXT(
                    static_cast<VkInstance>(mInstance),
                    reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(
                        &messengerCi),
                    nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
            }
        #endif
    }
}