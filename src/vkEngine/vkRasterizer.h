#pragma once
#include "vkBackend.h"
#include <array>
#include "VkEngineApiBinding.h"
#include <core/RenderSettings.h>
constexpr uint32_t maxFramesInFlight { 2 };



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
    vk::SwapchainKHR swapchain;
    RenderSettings* rs;
     std::vector<vk::Image> drawImgs; 
    std::vector<vk::ImageView> drawImgViews;
    std::vector<vk::Image> swapchainImgs;
    std::vector<vk::ImageView> swapchainImgViews;
    vk::Queue queue;
    VmaAllocator alloc;
};

struct ShaderData
{
    glm::mat4 mvp;
};



class vkRasterizer
{
    public:
        void Init(RasterizerInitInfo info);

        void Render(RasterizerRenderInfo info);

        void UpdateSwapchain(const VmaAllocator& allocator,int width, int height,int imageCount, const vk::Device& device);

        void destroy(vk::Device device, VmaAllocator alloc);
    private:
        vk::Format depthFormat{ vk::Format::eUndefined};
        vk::Image mDepthImage;
        VmaAllocation depthImageAllocation;
        vk::ImageView mDepthImageView;
        glm::vec2 mWindowDim;
        vk::CommandPool mCommandPool;
        std::vector<vkUtils::vkModel> mVkModels;
        std::array<VkUtilBuffer, maxFramesInFlight> mShaderBuffers;
        std::array<vk::CommandBuffer, maxFramesInFlight> mCommandBuffers;
        std::array<vk::Fence, maxFramesInFlight> mFences;
        std::array<vk::Semaphore, maxFramesInFlight> mImageAcquiredSemaphores;
        std::vector<vk::Semaphore> mRenderCompleteSemaphores;
        vk::PipelineLayout pipLayout;
        vk::Pipeline mPip;

        void CreateDepthImg(const RasterizerInitInfo& info);
        void CreateGraphicsPipeline(const RasterizerInitInfo& info, vk::ShaderModule vertModule, vk::ShaderModule fragModule);

        int lastModelCount;
};

