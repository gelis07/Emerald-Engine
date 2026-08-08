#pragma once
#define GLFW_INCLUDE_VULKAN
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#define FMT_HEADER_ONLY
#define FMT_USE_LOCALE 0
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <vkEngine/vkRasterizer.h>
#include "CameraControl.h"
#include "AssimpLoader.h"
#include <vkEngine/vkRaytracer.h>
#include "GUI.h"

#define WWIDTH 1280
#define WHEIGHT 920

class vkApp
{
    public:
        void Init();
        void Update();
    private:
        //App specifics
        vk::Instance mInstance;
        vk::ApplicationInfo appInfo;

        //Window
        GLFWwindow* mWindow;
        vk::SurfaceKHR mSurface; 
        vk::SwapchainKHR mSwapchain;
        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapchainImagesViews;
        vk::SurfaceCapabilitiesKHR surfaceCaps;
        vk::SwapchainCreateInfoKHR swapchainCi; //To keep for recreating
        vk::Format imageFormat;

        //Devices
        vk::Device mDevice;
        vk::PhysicalDevice mPhysicalDevice;
        VmaAllocator mAllocator;
        AssimpLoader mLoader;

        //Queues.
        vk::Queue graphicsQueue;
        vk::Queue computeQueue;
        uint32_t queueFamilyCompute = 0;
        uint32_t queueFamilyGraphics = 0;

        //Engines
        vk::CommandPool mCommandPool;
        vkRasterizer rasterizer;
        vkRaytracer raytracer;
        CameraControl camControl;
        std::vector<vk::Image> mRastImages;
        std::vector<vk::ImageView> mRastImageViews;
        
        //ImGui
        VkDescriptorSet raytracingImgSet;
        std::vector<VkDescriptorSet> rastImgSets; //Its handled by imGui so C types here.
        vk::DescriptorPool imguiPool;
        vk::Sampler imguiSampler;
        GUI gui;
        
        //Functions.
        void InitInstance(std::vector<const char*> extensions);
        void CreateVirtualDevice();
        void CreateSwapchain();
        void InitImGui();
        
        void UpdateSwapchain(int width, int height);
        void UpdateRastImgs(uint32_t width, uint32_t height);
        void InitImGuiStyles();


        void Destroy();
        //Tracking variables
        bool initImg = false;
        uint32_t frameIdx = 0;
        double mLastTime;
        int mLastImgWidth;
        int mLastImgHeight;
        int mLastWindowWidth;
        int mLastWindowHeight;
};