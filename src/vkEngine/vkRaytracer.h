#pragma once
#include <vkEngine/VkEngineApiBinding.h>
#include <core/Model.h>
#include <core/RenderSettings.h>



struct RaytracerInitInfo
{
    vk::Instance instance;
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    VmaAllocator alloc;
    RenderSettings* rs;
    vk::Queue computeQueue;
    uint32_t queueFamily;
};

struct RaytracerRenderInfo
{
    vk::Device device;
    VmaAllocator alloc;
    RenderSettings* rs;
};

struct AccelerationStruct
{
    vk::AccelerationStructureKHR accel;
    VkUtilBuffer StructureBuffer;
    VkUtilBuffer ScratchBuffer;
    VkUtilBuffer InstBuffer;
};

class vkRaytracer
{
    public:
        void Init(RaytracerInitInfo info);
        void Run(RaytracerRenderInfo info);
        void destroy(vk::Device device, VmaAllocator alloc);
        VmaAllocationInfo shaderDataAllocInfo{};
        vk::Image renderTargetImage;
        vk::ImageView renderTargetImageView;

        vk::Image sumImage;
        vk::ImageView sumImageView;
        uint32_t frameIdx;
        uint32_t frameIdxForRender;
        vk::Pipeline pip;
        vk::PipelineLayout pipLayout;
        vk::StridedDeviceAddressRegionKHR sbtRayGenAddressRegion, sbtMissAddressRegion, sbtCHitAddresRegion;
        void ExportToPng(VmaAllocator alloc,vk::Device device);

    private:
    
        vk::detail::DispatchLoaderDynamic dynamicDispatchLoader;
        vk::DescriptorSet descSet;
        vk::DescriptorSetLayout setLayout;
        vk::DescriptorPool descPool;
        vk::Fence fence;
        vk::CommandBuffer cb;
        vk::Queue mComputeQueue;
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR mRtProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR mAsProperties{};
        vk::CommandPool mCommandPool;
        vkUtils::vkScene mVkScene;
        std::vector<AccelerationStruct> bottomAccStructures;
        AccelerationStruct topAccStructure;
        vk::Sampler texturSampler;

        TextureVk skybox;
        
        int mLastModelSize = 0;
        

        void createDescriptorSetLayout(RaytracerInitInfo info);
        [[nodiscard]]AccelerationStruct CreateBLAS(const vk::Device& device,const VmaAllocator alloc, const vk::Queue& queue,
            const vkUtils::vkModel& vkModel);
        vk::ShaderModule createShaderModule(const vk::Device& device, const std::string& path);
        [[nodiscard]]AccelerationStruct creatTLAS(const vk::Device& device,const VmaAllocator alloc, const vk::Queue& queue);
        void UpdateModels(const vk::Device& device, const VmaAllocator& alloc, Scene& scene);
        void CreateRayTracingCB(const vk::Device& device, vk::AccelerationStructureBuildGeometryInfoKHR buildInfo, vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo, bool init);
};