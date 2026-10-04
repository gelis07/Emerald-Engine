#pragma once
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

/*
    Creates vulkan Instance, also handles validation layers.
    VALIDATIONS_LAYERS definition comes from CMake
*/

namespace core
{
    class instance
    {
    public:
        instance(std::vector<const char *> extensions);

        vk::Instance getInstance() const { return mInstance; }
    private:
        vk::ApplicationInfo mAppInfo;
        vk::Instance mInstance;
    };
}