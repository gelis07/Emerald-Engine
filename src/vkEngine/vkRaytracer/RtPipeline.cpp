#include "RtPipeline.h"
#include <core/Utils.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <filesystem>

vk::ShaderModule RtPip::createShaderModule(vk::Device device, const std::string& path)
{
    std::vector<char> shaderCode = Utils::ReadFileBinary(path);
    vk::ShaderModuleCreateInfo ci;
    ci.setPCode(reinterpret_cast<const uint32_t*>(shaderCode.data()))
    .setCodeSize(shaderCode.size());
    vk::ShaderModule module = device.createShaderModule(ci);
    return module;
}

void RtPip::Init(VkContext context, vk::detail::DispatchLoaderDynamic loader, vk::DescriptorSetLayout* setLayout)
{
    device = context.device;
    alloc = context.alloc;
    dynamicDispatchLoader = loader;

    //Getting properties
    vk::PhysicalDeviceProperties2 rayTracingProperties{};
    rayTracingProperties.pNext = &mRtProperties;
    context.physicalDevice.getProperties2(&rayTracingProperties);

    //Setting up the pipeline
    vk::PipelineLayoutCreateInfo pipLayoutInfo;
    pipLayoutInfo.setSetLayoutCount(1)
    .setPSetLayouts(setLayout)
    .setPushConstantRangeCount(0)
    .setPPushConstantRanges(nullptr);
    fmt::println("{}", std::filesystem::current_path().string());
    pipLayout = device.createPipelineLayout(pipLayoutInfo);

    vk::ShaderModule rayGenModule = createShaderModule(device, "../Shaders/RT/shader.rgen.spv");
    vk::ShaderModule rayMissModule = createShaderModule(device, "../Shaders/RT/shader.rmiss.spv");
    vk::ShaderModule rayHitModule = createShaderModule(device, "../Shaders/RT/shader.rchit.spv");

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
    .setMaxPipelineRayRecursionDepth(mRtProperties.maxRayRecursionDepth)
    .setPLibraryInfo(&libCi)
    .setPLibraryInterface(nullptr)
    .setPLibraryInfo(nullptr)
    .setLayout(pipLayout)
    .setBasePipelineHandle(VK_NULL_HANDLE)
    .setBasePipelineIndex(0);

    pip = device.createRayTracingPipelineKHR(nullptr, nullptr, rtPipCi, nullptr, dynamicDispatchLoader).value;

    device.destroyShaderModule(rayGenModule);
    device.destroyShaderModule(rayMissModule);
    device.destroyShaderModule(rayHitModule);

    uint32_t baseAlign = mRtProperties.shaderGroupBaseAlignment;
    uint32_t handleSize = mRtProperties.shaderGroupHandleSize;

    const uint32_t shaderGroupCount = 3;
    vk::DeviceSize sbtBufferSize = baseAlign * 3;

    vk::Buffer shaderBindingTableBuffer;

    vk::BufferCreateInfo sbtBuffCi;
    sbtBuffCi.setUsage(vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress)
    .setSize(sbtBufferSize);

    VmaAllocation sbtAlloc{};
    VmaAllocationInfo sbtAllocInfo{};

    VmaAllocationCreateInfo sbtAllocCi{};
    sbtAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    sbtAllocCi.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBufferWithAlignment(alloc, reinterpret_cast<VkBufferCreateInfo*>(&sbtBuffCi), &sbtAllocCi, mRtProperties.shaderGroupBaseAlignment, 
    reinterpret_cast<VkBuffer*>(&shaderBindingTableBuffer), &sbtAlloc, &sbtAllocInfo);

    std::vector<uint8_t> handles = device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pip, 0 , shaderGroupCount,
    shaderGroupCount * handleSize, dynamicDispatchLoader);

    vk::BufferDeviceAddressInfo sbtBufferAddressInfo{};
    sbtBufferAddressInfo.setBuffer(shaderBindingTableBuffer);
    vk::DeviceAddress sbtAddress = device.getBufferAddress(sbtBufferAddressInfo);

    vk::StridedDeviceAddressRegionKHR addressRegion;
    addressRegion.setStride(baseAlign)
    .setSize(handleSize);

    sbtRayGenAddressRegion = addressRegion;
    sbtRayGenAddressRegion.setSize(baseAlign)
    .setDeviceAddress(sbtAddress);

    sbtMissAddressRegion = addressRegion;
    sbtMissAddressRegion.setDeviceAddress(sbtAddress + baseAlign);

    sbtCHitAddresRegion = addressRegion;
    sbtCHitAddresRegion.setDeviceAddress(sbtAddress + baseAlign * 2);
    uint8_t* sbtBufferData = static_cast<uint8_t*>(sbtAllocInfo.pMappedData);

    std::memcpy(sbtBufferData, handles.data(), handleSize);
    std::memcpy(sbtBufferData + baseAlign, handles.data() + handleSize, handleSize);
    std::memcpy(sbtBufferData + baseAlign * 2, handles.data() + handleSize * 2, handleSize);
}


void RtPip::destroy()
{
    device.destroyPipelineLayout(pipLayout);
    device.destroyPipeline(pip);
}