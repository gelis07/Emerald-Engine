#pragma once
#include <vkEngine/VkEngineApiBinding.h>
#include <core/Model.h>

struct AccelerationStruct
{
    vk::AccelerationStructureKHR accel;
    VkUtilBuffer StructureBuffer;
    VkUtilBuffer ScratchBuffer;
    VkUtilBuffer InstBuffer;
};

struct BuildASOnGPUInfo
{
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
};

class ASManager
{
    public:
        void Init(VkContext context, vk::detail::DispatchLoaderDynamic loader);
        
        void DeleteSceneAS();
        void CreateSceneAS(const vkUtils::vkScene& scene);
        BuildASOnGPUInfo SetUpASForScene(const vkUtils::vkScene& scene);
        void UpdateBLASes(const std::vector<vkUtils::VkMesh>& vkMeshes);
        AccelerationStruct topAccStructure;
    private:
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR mAsProperties{};
        std::vector<AccelerationStruct> bottomAccStructures;
        
        vk::AccelerationStructureGeometryKHR geometry{};

        vk::Queue queue;
        vk::Device device;
        VmaAllocator alloc;
        vk::CommandPool commandPool;
        vk::detail::DispatchLoaderDynamic dynamicDispatchLoader;

        [[nodiscard]]AccelerationStruct CreateBLAS(const vkUtils::VkMesh& vkModel);
        vk::ShaderModule createShaderModule(const std::string& path);
        [[nodiscard]]AccelerationStruct creatTLAS(const vkUtils::vkScene& scene);
};