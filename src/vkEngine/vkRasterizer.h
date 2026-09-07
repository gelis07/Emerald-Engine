#pragma once
#include "vkBackend.h"
#include <array>
#include "VkEngineApiBinding.h"
#include <core/RenderSettings.h>
constexpr uint32_t maxFramesInFlight { 1 };



struct RasterizerInitInfo
{
    vk::PhysicalDevice physicalDevice; 
    int wWidth; 
    int wHeight; 
    VmaAllocator allocator; 
    vk::Device device;
    RenderSettings* rs;
    uint32_t swapchainImgCount; 
    uint32_t queueFamily; 
    vk::Queue queue;
    vk::Format imageFormat;
};


struct RasterizerRenderInfo
{
    vk::Device device;
    int frameIdx;
    RenderSettings* rs;
    std::vector<vk::Image> drawImgs; 
    std::vector<vk::ImageView> drawImgViews;
    vk::Fence* fence;
    vk::Queue queue;
    VmaAllocator alloc;
};

struct ShaderData
{
    glm::mat4 mvp;
};

struct pushConstantsStruct
{
    glm::mat4 mat;
    uint32_t objId;
    uint32_t modelId;
};
struct boneGPU
{
    uint32_t nodeId;
};

struct nodeDataGPU
{
    glm::mat4 transform;
    uint32_t parentId;
};


class vkRasterizer
{
    public:
        void Init(RasterizerInitInfo info);

        void Render(RasterizerRenderInfo info);

        void UpdateSwapchain(const VmaAllocator& allocator,int width, int height,int imageCount, const vk::Device& device);
        vk::CommandBuffer getActiveCb(uint32_t frameIdx) {return mCommandBuffers[frameIdx];}
        void destroy(vk::Device device, VmaAllocator alloc);
        vkUtils::vkScene* mVkScene;
    private:
        vk::Format depthFormat{ vk::Format::eUndefined};
        vk::Image mDepthImage;
        VmaAllocation depthImageAllocation;
        vk::ImageView mDepthImageView;
        glm::vec2 mWindowDim;
        vk::CommandPool mCommandPool;
        
        vk::DescriptorSet descSet;
        vk::DescriptorSetLayout setLayout;
        vk::DescriptorPool descPool;

        std::array<VkUtilBuffer, maxFramesInFlight> mShaderBuffers;
        std::array<vk::CommandBuffer, maxFramesInFlight> mCommandBuffers;

        vk::PipelineLayout pipLayout;
        vk::Pipeline mPip;



        uint32_t mLastModelSize = 0;
        VkContext context;

        void InitDescPool(VkContext context);
        void WriteDynamicDescriptors(VkContext context);
        void CreateDepthImg(const RasterizerInitInfo& info);
        void CreateGraphicsPipeline(const RasterizerInitInfo& info, vk::ShaderModule vertModule, vk::ShaderModule fragModule);
};

