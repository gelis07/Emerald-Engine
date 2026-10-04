#include "shaderModules.hpp"
#include "utils.hpp"

namespace core
{
    ShaderModule::ShaderModule(core::context context, const std::string& path)
    {
        std::vector<char> shaderCode = core::ReadFileBinary(path);
        vk::ShaderModuleCreateInfo ci;
        ci.setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()))
        .setCodeSize(shaderCode.size());
        mModule = context.device.getDevice().createShaderModule(ci);
    }
}