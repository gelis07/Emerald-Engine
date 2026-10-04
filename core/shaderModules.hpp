#pragma once
#include <string>
#include <vulkan/vulkan.hpp>
#include "context.hpp"

namespace core
{
    class ShaderModule
    {
        public:
        ShaderModule(core::context context, const std::string& path);

        vk::ShaderModule getModule() const {return mModule;}
        private:
        vk::ShaderModule mModule;
    };
}