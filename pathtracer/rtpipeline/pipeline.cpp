#include "pipeline.hpp"

namespace engine
{
    void Pipeline::Init(core::context context, vk::DescriptorSetLayout setLayout)
    {
        mRtPipLayout.Init(context, setLayout);

        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};

        //Getting properties
        vk::PhysicalDeviceProperties2 rayTracingProperties{};
        rayTracingProperties.pNext = &rtProperties;
        context.device.getPhysicalDevice().getProperties2(&rayTracingProperties);

        vk::ShaderModule rayGenModule = core::ShaderModule(context, "shaders/shader.rgen.spv").getModule();
        vk::ShaderModule rayMissModule = core::ShaderModule(context, "shaders/shader.rmiss.spv").getModule();
        vk::ShaderModule rayHitModule = core::ShaderModule(context, "shaders/shader.rchit.spv").getModule();

        std::vector<vk::PipelineShaderStageCreateInfo> stages;
        stages.resize(3);
        stages[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR)
        .setModule(rayGenModule)
        .setPName("main");
        stages[1].setStage(vk::ShaderStageFlagBits::eMissKHR)
        .setModule(rayMissModule)
        .setPName("main");
        stages[2].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
        .setModule(rayHitModule)
        .setPName("main");

        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;
        groups.resize(3);
        groups[0].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(0)
        .setClosestHitShader(vk::ShaderUnusedKHR)
        .setAnyHitShader(vk::ShaderUnusedKHR)
        .setIntersectionShader(vk::ShaderUnusedKHR);
        groups[1].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(1)
        .setIntersectionShader(vk::ShaderUnusedKHR)
        .setClosestHitShader(vk::ShaderUnusedKHR)
        .setAnyHitShader(vk::ShaderUnusedKHR);
        groups[2].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
        .setGeneralShader(vk::ShaderUnusedKHR)
        .setAnyHitShader(vk::ShaderUnusedKHR)
        .setClosestHitShader(2)
        .setIntersectionShader(vk::ShaderUnusedKHR);

        vk::PipelineLibraryCreateInfoKHR libCi;
        libCi.setLibraryCount(0);
        
        vk::RayTracingPipelineCreateInfoKHR rtPipCi;
        rtPipCi.setStageCount(static_cast<uint32_t>(stages.size()))
        .setPStages(stages.data())
        .setGroupCount(static_cast<uint32_t>(groups.size()))
        .setPGroups(groups.data())
        .setMaxPipelineRayRecursionDepth(rtProperties.maxRayRecursionDepth)
        .setPLibraryInfo(&libCi)
        .setPLibraryInterface(nullptr)
        .setPLibraryInfo(nullptr)
        .setLayout(mRtPipLayout.getPipLayout())
        .setBasePipelineHandle(VK_NULL_HANDLE)
        .setBasePipelineIndex(0);

        mPipeline = context.device.getDevice().createRayTracingPipelineKHR(nullptr, nullptr, rtPipCi, nullptr, context.device.getLoader()).value;

        context.device.getDevice().destroyShaderModule(rayGenModule);
        context.device.getDevice().destroyShaderModule(rayMissModule);
        context.device.getDevice().destroyShaderModule(rayHitModule);

        mBTable.Init(context, rtProperties, mPipeline);
    }
}