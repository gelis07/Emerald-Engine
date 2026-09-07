#include "vkApp.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif



void vkApp::Init()
{
    //Initialize glfw and vulkan app
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(WWIDTH, WHEIGHT, "Raytracer", NULL, NULL);
    glfwMaximizeWindow(mWindow);
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr) {
    throw std::runtime_error("Failed to find required Vulkan instance extensions! "
                             "Is a compatible Vulkan driver installed?");
    }
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    InitInstance(extensions);

    VkSurfaceKHR rawSurface;
    if(glfwCreateWindowSurface(mInstance, mWindow, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create surface");
    }

    mSurface = rawSurface;

    //Create device and allocator.
    CreateVirtualDevice();
    VmaVulkanFunctions vkFunctions;
    vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vkFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCI {};
    allocatorCI.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    allocatorCI.physicalDevice = static_cast<VkPhysicalDevice>(mPhysicalDevice);
    allocatorCI.device = static_cast<VkDevice>(mDevice);
    allocatorCI.instance = static_cast<VkInstance>(mInstance);
    allocatorCI.pVulkanFunctions = nullptr;
    allocatorCI.vulkanApiVersion = VK_API_VERSION_1_4;
    vmaCreateAllocator(&allocatorCI, &mAllocator);


    CreateSwapchain();

    //Create a default scene.
    Model cube;
    ModelConstructData data;
    Mesh mesh;
    mesh.vertices = CubeVertices;
    mesh.indices = CubeIndices;
    mesh.matIndex = 0;
    mesh.nodeId = 0;
    cube.type = CUBE;
    data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
    data.meshes.push_back(mesh);
    cube.Load(data);
    gui.settings.scene.models.push_back(cube);

    vk::CommandPoolCreateInfo commandPoolCi;
    commandPoolCi.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .setQueueFamilyIndex(queueFamilyCompute);
    mCommandPool = mDevice.createCommandPool(commandPoolCi);

    //Initialize engines.
    VkContext context;
    context.alloc = mAllocator;
    context.commandPool = mCommandPool;
    context.device = mDevice;
    context.physicalDevice = mPhysicalDevice;
    context.queue = computeQueue;
    context.queueFamily = queueFamilyCompute;
    vkSceneManager.Init(context);
    vkSceneManager.updateScene(gui.settings.scene);
    mLastModelCount = gui.settings.scene.models.size();
    raytracer.mVkScene = vkSceneManager.GetScenePointer();
    rasterizer.mVkScene = vkSceneManager.GetScenePointer();


    int wWidth, wHeight;
    glfwGetWindowSize(mWindow, &wWidth, &wHeight);
    camControl.Init(45.0f, 0.1f, 1000.0f);

    gui.settings.ImgWidth = RTXimgWidth;
    gui.settings.ImgHeight = RTXimgHeight;

    RasterizerInitInfo rastInitInfo;
    rastInitInfo.physicalDevice = mPhysicalDevice;
    rastInitInfo.device = mDevice;
    rastInitInfo.wHeight = wHeight;
    rastInitInfo.wWidth = wWidth;
    rastInitInfo.allocator = mAllocator;
    rastInitInfo.rs = &gui.settings;
    rastInitInfo.swapchainImgCount = mSwapchainImages.size();
    rastInitInfo.queueFamily = queueFamilyGraphics;
    rastInitInfo.imageFormat = imageFormat;
    rastInitInfo.queue = computeQueue;

    rasterizer.Init(rastInitInfo);


    RaytracerInitInfo raytracerInitInfo;
    raytracerInitInfo.alloc = mAllocator;
    raytracerInitInfo.instance = mInstance;
    raytracerInitInfo.device = mDevice;
    raytracerInitInfo.physicalDevice = mPhysicalDevice;
    raytracerInitInfo.rs = &gui.settings;
    raytracerInitInfo.computeQueue = computeQueue;
    raytracerInitInfo.queueFamily = queueFamilyCompute;

    raytracer.Init(raytracerInitInfo);
    mLastWindowWidth = wWidth;
    mLastWindowHeight = wHeight;

    
    gui.loader = &mLoader;

    vk::SamplerCreateInfo samplerCi;
    samplerCi.setMagFilter(vk::Filter::eLinear)
    .setMinFilter(vk::Filter::eLinear)
    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear);

    imguiSampler = mDevice.createSampler(samplerCi);

    InitImGui();
    InitImGuiStyles();

    raytracingImgSet = ImGui_ImplVulkan_AddTexture(static_cast<VkImageView>(raytracer.sumImageView),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    mRastImageViews.resize(maxFramesInFlight);
    mRastImages.resize(maxFramesInFlight);
    rastImgSets.resize(maxFramesInFlight);
    UpdateRastImgs(1280, 720);
    mLastImgWidth = 1280;
    mLastImgHeight = 720;



    vk::SemaphoreCreateInfo semaphoreCi;
    vk::FenceCreateInfo fenceCi;
    fenceCi.flags = vk::FenceCreateFlagBits::eSignaled;
    for(int i = 0; i < maxFramesInFlight; i++)
    {
        mFences[i] = mDevice.createFence(fenceCi);
        mImageAcquiredSemaphores[i] = mDevice.createSemaphore(semaphoreCi);
    }
    mRenderCompleteSemaphores.resize(mSwapchainImages.size());
    for(int i = 0; i < mSwapchainImages.size(); i++)
    {
        mRenderCompleteSemaphores[i] = mDevice.createSemaphore(semaphoreCi);
    }

    vk::CommandBufferAllocateInfo cbAllocInfo;
    cbAllocInfo.commandPool = mCommandPool;
    cbAllocInfo.commandBufferCount = maxFramesInFlight;
    uiDrawCbs = mDevice.allocateCommandBuffers(cbAllocInfo);
}


void vkApp::Update()
{
    while(!glfwWindowShouldClose(mWindow))
    {
        float dt = glfwGetTime() - mLastTime;
        mLastTime = glfwGetTime();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        
        
        LoadSceneInfo loadSceneInfo;
        loadSceneInfo.prevTextCount = gui.settings.scene.textures.size();
        loadSceneInfo.alloc = mAllocator;
        loadSceneInfo.cPool = mCommandPool;
        loadSceneInfo.queue = computeQueue;
        loadSceneInfo.device = mDevice;
        

        vkSceneManager.UpdateBoneTransforms(gui.settings.scene);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        VkDescriptorSet renderTargetSet = gui.renderMode == RenderMode::PathTracing ? raytracingImgSet : rastImgSets[frameIdx];

        gui.SceneModifier(dt, {(uint64_t)renderTargetSet}, camControl, loadSceneInfo);
        ImGui::Render();


        int ImgWidth = gui.settings.ImgWidth;
        int ImgHeight = gui.settings.ImgHeight;
        int WindowWidth, WindowHeight;
        glfwGetWindowSize(mWindow, &WindowWidth, &WindowHeight);

        if(mLastModelCount != gui.settings.scene.models.size())
        {
            vkSceneManager.deleteSceneModels();
            vkSceneManager.updateScene(gui.settings.scene);
            mLastModelCount = gui.settings.scene.models.size();
        }
        if (ImgWidth > 0 && ImgHeight > 0) 
        {
            if(ImgWidth != mLastImgWidth || ImgHeight != mLastImgHeight)
            {
                UpdateRastImgs(ImgWidth, ImgHeight);
                mLastImgHeight = ImgHeight;
                mLastImgWidth = ImgWidth;
            }

            camControl.OnUpdate(mWindow, dt, ImgWidth, ImgHeight);
            gui.settings.scene.camera = camControl.GetCamera();


            vkSceneManager.SkinMeshes(gui.settings.scene, static_cast<uint32_t>(gui.renderMode));

            if(gui.renderMode == RenderMode::PathTracing)
            {
                if(prevRendMode != gui.renderMode)
                {
                    raytracer.resetFrameIdx();
                    gui.settings.ReloadScene = true;
                }

                RaytracerRenderInfo raytracingRenderInfo;
                raytracingRenderInfo.alloc = mAllocator;
                raytracingRenderInfo.device = mDevice;
                raytracingRenderInfo.rs = &gui.settings;
    
                raytracer.Run(raytracingRenderInfo);
            }

            if(gui.renderMode == RenderMode::Rasterizer)
            {
                RasterizerRenderInfo renderInfo;
                renderInfo.device = mDevice;
                renderInfo.frameIdx = frameIdx;
                renderInfo.fence = &mFences[frameIdx];
                renderInfo.rs = &gui.settings;
                renderInfo.drawImgs = mRastImages;
                renderInfo.drawImgViews = mRastImageViews;
                renderInfo.queue = graphicsQueue;
                renderInfo.alloc = mAllocator;

                rasterizer.Render(renderInfo);
            }
            
            drawUi(WindowWidth, WindowHeight);
            if(gui.settings.Render)
            {
                raytracer.ExportToPng(mAllocator, mDevice, "render.png");
                gui.settings.Render = false;
            }
        }

        if(mLastWindowWidth != WindowWidth || mLastWindowHeight != WindowHeight)
        {
            mLastWindowHeight = WindowHeight;
            mLastWindowWidth = WindowWidth;
            UpdateSwapchain(WindowWidth, WindowHeight);
        }
        prevRendMode = gui.renderMode;
        glfwPollEvents();
        frameIdx = (frameIdx + 1) % maxFramesInFlight;
    }
    mDevice.waitIdle();
    vkSceneManager.deleteScene();
    rasterizer.destroy(mDevice, mAllocator);
    raytracer.destroy(mDevice, mAllocator);
    Destroy();
}

void vkApp::Destroy()
{
    for(int i = 0; i < mFences.size(); i++)
    {
        mDevice.destroyFence(mFences[i]);
    }
    for(int i = 0; i < mImageAcquiredSemaphores.size(); i++)
    {
        mDevice.destroySemaphore(mImageAcquiredSemaphores[i]);
    }
    for(int i = 0; i < mRenderCompleteSemaphores.size(); i++)
    {
        mDevice.destroySemaphore(mRenderCompleteSemaphores[i]);
    }
    for(int i = 0; i < mRastImages.size(); i++)
    {
        mDevice.destroyImage(mRastImages[i]);
        mDevice.destroyImageView(mRastImageViews[i]);
    }
    mDevice.destroyDescriptorPool(imguiPool);
    mDevice.destroySampler(imguiSampler);
    
    vmaDestroyAllocator(mAllocator);
    mDevice.destroySwapchainKHR(mSwapchain);
    mDevice.destroy();
    mInstance.destroy();

}
void vkApp::UpdateSwapchain(int width, int height)
{
    mDevice.waitIdle();
    vk::SurfaceCapabilitiesKHR surfCap = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);



    swapchainCi.oldSwapchain = mSwapchain;
    swapchainCi.imageExtent.width = width;
    swapchainCi.imageExtent.height = height;

    mSwapchain = mDevice.createSwapchainKHR(swapchainCi);
    int imageCount = mSwapchainImages.size();
    for(int i = 0; i < imageCount; i++)
    {
        mDevice.destroyImageView(mSwapchainImagesViews[i]);
    }
    for (int i = 0; i < imageCount; i++)
    {
        mDevice.destroySemaphore(mRenderCompleteSemaphores[i]);
    }
    mRenderCompleteSemaphores.resize(imageCount);
    for(int i = 0; i < imageCount; i++)
    {
        mRenderCompleteSemaphores[i] = mDevice.createSemaphore({});
    }
    mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
    imageCount = mSwapchainImages.size();
    for(int i = 0; i < imageCount; i++)
    {
        vk::ImageViewCreateInfo viewCi;
        viewCi.setImage(mSwapchainImages[i])
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(imageFormat)
        .setComponents({vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity,
        vk::ComponentSwizzle::eIdentity})
        .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setLayerCount(1).setLevelCount(1);

        mSwapchainImagesViews[i] = mDevice.createImageView(viewCi);

    }
    rasterizer.UpdateSwapchain(mAllocator, width, height, imageCount, mDevice);
    mDevice.destroySwapchainKHR(swapchainCi.oldSwapchain);
}


void vkApp::UpdateRastImgs(uint32_t width, uint32_t height)
{
    mDevice.waitIdle();

    for(int i = 0; i < mRastImages.size(); i++)
    {
        if(initImg)
        {
            mDevice.destroyImage(mRastImages[i]);
            mDevice.destroyImageView(mRastImageViews[i]);
            ImGui_ImplVulkan_RemoveTexture(rastImgSets[i]);
        }


        vk::ImageCreateInfo rastImgCi;
        rastImgCi.setImageType(vk::ImageType::e2D)
        .setFormat(imageFormat)
        .setExtent({width, height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment)
        .setInitialLayout(vk::ImageLayout::eUndefined);

        VmaAllocationCreateInfo rastAllocInfo{};
        rastAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        rastAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        
        VmaAllocation rastImgAlloc{};
        vmaCreateImage(mAllocator, reinterpret_cast<const VkImageCreateInfo*>(&rastImgCi)
        , &rastAllocInfo, reinterpret_cast<VkImage*>(&mRastImages[i]), &rastImgAlloc, nullptr);

        vk::ImageViewCreateInfo rastImgViewCi;
        rastImgViewCi.setImage(mRastImages[i])
        .setViewType(vk::ImageViewType::e2D)
        .setComponents({vk::ComponentSwizzle::eIdentity
                    , vk::ComponentSwizzle::eIdentity
                    , vk::ComponentSwizzle::eIdentity
                    , vk::ComponentSwizzle::eIdentity})
        .setFormat(imageFormat)
        .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setLayerCount(1)
        .setLevelCount(1);

        mRastImageViews[i] = mDevice.createImageView(rastImgViewCi);

        rastImgSets[i] = ImGui_ImplVulkan_AddTexture(static_cast<VkImageView>(mRastImageViews[i]),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if(!initImg)
            initImg = true;
    }


    mDevice.waitIdle();
}


void vkApp::InitImGuiStyles()
{
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    io.Fonts->AddFontFromFileTTF("Lato-Regular.ttf", 16.0f);


    // Base Colors
    ImVec4 bgColor = ImVec4(0.10f, 0.105f, 0.11f, 1.00f);
    ImVec4 lightBgColor = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
    ImVec4 panelColor = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
    ImVec4 panelHoverColor = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    ImVec4 panelActiveColor = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
    ImVec4 textColor = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
    ImVec4 textDisabledColor = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    ImVec4 borderColor = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);

    // Text
    colors[ImGuiCol_Text] = textColor;
    colors[ImGuiCol_TextDisabled] = textDisabledColor;

    // Windows
    colors[ImGuiCol_WindowBg] = bgColor;
    colors[ImGuiCol_ChildBg] = bgColor;
    colors[ImGuiCol_PopupBg] = bgColor;
    colors[ImGuiCol_Border] = borderColor;
    colors[ImGuiCol_BorderShadow] = borderColor;

    // Headers
    colors[ImGuiCol_Header] = panelColor;
    colors[ImGuiCol_HeaderHovered] = panelHoverColor;
    colors[ImGuiCol_HeaderActive] = panelActiveColor;

    // Buttons
    colors[ImGuiCol_Button] = panelColor;
    colors[ImGuiCol_ButtonHovered] = panelHoverColor;
    colors[ImGuiCol_ButtonActive] = panelActiveColor;

    // Frame BG
    colors[ImGuiCol_FrameBg] = lightBgColor;
    colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
    colors[ImGuiCol_FrameBgActive] = panelActiveColor;

    // Tabs
    colors[ImGuiCol_Tab] = panelColor;
    colors[ImGuiCol_TabHovered] = panelHoverColor;
    colors[ImGuiCol_TabActive] = panelActiveColor;
    colors[ImGuiCol_TabUnfocused] = panelColor;
    colors[ImGuiCol_TabUnfocusedActive] = panelHoverColor;

    // Title
    colors[ImGuiCol_TitleBg] = bgColor;
    colors[ImGuiCol_TitleBgActive] = bgColor;
    colors[ImGuiCol_TitleBgCollapsed] = bgColor;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = bgColor;
    colors[ImGuiCol_ScrollbarGrab] = panelColor;
    colors[ImGuiCol_ScrollbarGrabHovered] = panelHoverColor;
    colors[ImGuiCol_ScrollbarGrabActive] = panelActiveColor;

    // Checkmark
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Slider
    colors[ImGuiCol_SliderGrab] = panelHoverColor;
    colors[ImGuiCol_SliderGrabActive] = panelActiveColor;

    // Resize Grip
    colors[ImGuiCol_ResizeGrip] = panelColor;
    colors[ImGuiCol_ResizeGripHovered] = panelHoverColor;
    colors[ImGuiCol_ResizeGripActive] = panelActiveColor;

    // Separator
    colors[ImGuiCol_Separator] = borderColor;
    colors[ImGuiCol_SeparatorHovered] = panelHoverColor;
    colors[ImGuiCol_SeparatorActive] = panelActiveColor;

    // Plot
    colors[ImGuiCol_PlotLines] = textColor;
    colors[ImGuiCol_PlotLinesHovered] = panelActiveColor;
    colors[ImGuiCol_PlotHistogram] = textColor;
    colors[ImGuiCol_PlotHistogramHovered] = panelActiveColor;

    // Text Selected BG
    colors[ImGuiCol_TextSelectedBg] = panelActiveColor;

    // Modal Window Dim Bg
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.105f, 0.11f, 0.5f);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = panelColor;
    colors[ImGuiCol_TableBorderStrong] = borderColor;
    colors[ImGuiCol_TableBorderLight] = borderColor;
    colors[ImGuiCol_TableRowBg] = bgColor;
    colors[ImGuiCol_TableRowBgAlt] = lightBgColor;

    // Styles
    style.FrameBorderSize = 1.0f;
    style.FrameRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabMinSize = 7.0f;
    style.GrabRounding = 2.0f;
    style.TabBorderSize = 1.0f;
    style.TabRounding = 2.0f;

    // Reduced Padding and Spacing
    style.WindowPadding = ImVec2(5.0f, 5.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    gui.window = mWindow;
}


static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    fmt::println("-------Vulkan-------");
    fmt::println("Vulkan Validation layers: {}", pCallbackData->pMessage);

    return VK_FALSE;
}
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void vkApp::InitInstance(std::vector<const char*> extensions)
{
    appInfo.setPApplicationName("Raytracer")
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
        .setPEngineName("Raytracer")
        .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
        .setApiVersion(VK_API_VERSION_1_4);

    if(enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);
    createInfo.setPEnabledExtensionNames(extensions);
    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    
    if (enableValidationLayers)
    {
        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        for(const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for(const auto& layerProperties : availableLayers)
            {
                if(strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            if(!layerFound) {
                throw std::runtime_error("Validation layers are requested, but not available on this system!");
            }
        }
        createInfo.setPEnabledLayerNames(validationLayers);
    }
    vk::DebugUtilsMessengerCreateInfoEXT messengerCi;
    messengerCi.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | 
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    messengerCi.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    messengerCi.pfnUserCallback = reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(debugCallback);
    if(enableValidationLayers)
    {
        createInfo.pNext = &messengerCi;
    }
    mInstance = vk::createInstance(createInfo);
    VkDebugUtilsMessengerEXT debugMessenger;
    if(enableValidationLayers)
    {
        if (CreateDebugUtilsMessengerEXT(static_cast<VkInstance>(mInstance), reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&messengerCi), nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
}

void vkApp::CreateVirtualDevice()
{
    std::vector<vk::PhysicalDevice> devices = mInstance.enumeratePhysicalDevices();
    mPhysicalDevice = devices[0];

    std::vector<vk::QueueFamilyProperties> queueFamilies = mPhysicalDevice.getQueueFamilyProperties();


    bool foundGraphics = false;
    bool foundCompute = false;
    for(int i = 0; i < queueFamilies.size(); i++)
    {
        if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queueFamilyGraphics = i;
            foundGraphics = true;
        }
        if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)
        {
            queueFamilyCompute = i;
            foundCompute = true;
        }
        if(foundCompute && foundGraphics)
            break;
    }
    const float qfPriorities{1.0f};
    const float qfPrioritiesCompute{1.0f};
    vk::DeviceQueueCreateInfo graphicsQueueCI;
    graphicsQueueCI.queueFamilyIndex = queueFamilyGraphics;
    graphicsQueueCI.queueCount = 1;
    graphicsQueueCI.pQueuePriorities = &qfPriorities;

    vk::DeviceQueueCreateInfo computeQueueCi;
    computeQueueCi.setQueueFamilyIndex(queueFamilyCompute)
    .setQueueCount(1)
    .setPQueuePriorities(&qfPrioritiesCompute);


    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    rayQueryFeatures.rayQuery = vk::True;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{};
    accelFeature.accelerationStructure = true;
    accelFeature.pNext = &rayQueryFeatures;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipFeatures{};
    rtPipFeatures.rayTracingPipeline = true;
    rtPipFeatures.pNext = &accelFeature;

    vk::PhysicalDeviceVulkan12Features enabledV12Features;
    enabledV12Features.pNext = &rtPipFeatures;
    enabledV12Features.descriptorIndexing = true;
    enabledV12Features.shaderSampledImageArrayNonUniformIndexing = true;
    enabledV12Features.descriptorBindingVariableDescriptorCount = true;
    enabledV12Features.runtimeDescriptorArray = true;
    enabledV12Features.bufferDeviceAddress = true;
    enabledV12Features.descriptorBindingPartiallyBound = true;
    enabledV12Features.scalarBlockLayout = true;
    enabledV12Features.shaderStorageBufferArrayNonUniformIndexing = true;
    vk::PhysicalDeviceVulkan13Features enabledVk13Features;
    enabledVk13Features.pNext = &enabledV12Features;
    enabledVk13Features.synchronization2 = true;
    enabledVk13Features.dynamicRendering = true;

    vk::PhysicalDeviceFeatures2 enabledVk10Features;
    enabledVk10Features.pNext = &enabledVk13Features;
    enabledVk10Features.features.samplerAnisotropy = VK_TRUE;
    enabledVk10Features.features.geometryShader = vk::True;
    enabledVk10Features.features.shaderInt64 = vk::True;
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME};
    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pNext = &enabledVk10Features;
    deviceCreateInfo.queueCreateInfoCount = 2;
    std::array<vk::DeviceQueueCreateInfo, 2> deviceQueueInfos = {graphicsQueueCI, computeQueueCi};
    deviceCreateInfo.pQueueCreateInfos = deviceQueueInfos.data();
    deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

    
    mDevice = mPhysicalDevice.createDevice(deviceCreateInfo);
    mDevice.getQueue(queueFamilyGraphics, 0, &graphicsQueue);
    mDevice.getQueue(queueFamilyCompute, 0, &computeQueue);
}


void vkApp::CreateSwapchain()
{
    surfaceCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
    vk::Extent2D swapchainExtent{surfaceCaps.currentExtent};
    int wWidth, wHeight;
    glfwGetWindowSize(mWindow, &wWidth, &wHeight);
    if(surfaceCaps.currentExtent.width == 0xFFFFFFFF)
    {
        swapchainExtent.width = static_cast<uint32_t>(wWidth);
        swapchainExtent.height = static_cast<uint32_t>(wHeight);
    }

    imageFormat = vk::Format::eB8G8R8A8Unorm;
    
    swapchainCi.surface = mSurface;
    swapchainCi.minImageCount = surfaceCaps.minImageCount;
    swapchainCi.imageFormat = imageFormat;
    swapchainCi.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchainCi.imageExtent.width = wWidth;
    swapchainCi.imageExtent.height = wHeight;
    swapchainCi.imageArrayLayers = 1;
    swapchainCi.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCi.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    swapchainCi.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCi.presentMode = vk::PresentModeKHR::eFifo;

    mSwapchain = mDevice.createSwapchainKHR(swapchainCi);

    mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
    mSwapchainImagesViews.resize(mSwapchainImages.size());
    for (size_t i = 0; i < mSwapchainImages.size(); i++) {
        
        vk::ImageViewCreateInfo createInfo(
            vk::ImageViewCreateFlags(),             
            mSwapchainImages[i],                     
            vk::ImageViewType::e2D,       
            imageFormat,                    
            vk::ComponentMapping{                   
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            },
            vk::ImageSubresourceRange{              
                vk::ImageAspectFlagBits::eColor,    
                0,                                  
                1,                                  
                0,                                  
                1                                   
            }
        );
        mSwapchainImagesViews[i] = mDevice.createImageView(createInfo);
    }

}


void vkApp::InitImGui()
{
    vk::DescriptorPoolSize poolSizes[] =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};
    vk::DescriptorPoolCreateInfo descPoolCi;
    descPoolCi.setMaxSets(1000)
    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
    .setPoolSizeCount(std::size(poolSizes))
    .setPPoolSizes(poolSizes);
    imguiPool = mDevice.createDescriptorPool(descPoolCi);

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(mWindow, true);
    // this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = static_cast<VkInstance>(mInstance);
    initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(mPhysicalDevice);
    initInfo.Device = static_cast<VkDevice>(mDevice);
    initInfo.Queue = graphicsQueue;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount =  surfaceCaps.minImageCount;
    initInfo.ImageCount = surfaceCaps.minImageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = reinterpret_cast<const VkFormat*>(&imageFormat);
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = static_cast<VkFormat>(vk::Format::eD32Sfloat);
	ImGui_ImplVulkan_Init(&initInfo);
}

void vkApp::drawUi(uint32_t width, uint32_t height)
{
    if(gui.renderMode != RenderMode::Rasterizer)
    {
        mDevice.waitForFences(1, &mFences[frameIdx], vk::True, UINT64_MAX);
        mDevice.resetFences(1, &mFences[frameIdx]);
    }
    
    uint32_t imageIdx = mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAcquiredSemaphores[frameIdx]).value;
    vk::CommandBuffer uiDrawCb = uiDrawCbs[frameIdx];
    uiDrawCb.reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    uiDrawCb.begin(beginInfo);
    vk::ImageMemoryBarrier2 midBarrier;
    midBarrier.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setSrcAccessMask(vk::AccessFlagBits2::eNone)
    .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
    .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
    .setImage(mSwapchainImages[imageIdx]);
    midBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    midBarrier.subresourceRange.levelCount = 1;
    midBarrier.subresourceRange.layerCount = 1;

    vk::DependencyInfo midBarriersDepInfo{};
    midBarriersDepInfo.setImageMemoryBarrierCount(1)
    .setPImageMemoryBarriers(&midBarrier);


    uiDrawCb.pipelineBarrier2(midBarriersDepInfo);
    vk::RenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.setImageView(mRastImageViews[frameIdx])
    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eStore)
    .clearValue.setColor({0.0f, 0.0f, 0.2f, 1.0f});
    colorAttachmentInfo.setLoadOp(vk::AttachmentLoadOp::eLoad);
    colorAttachmentInfo.setImageView(mSwapchainImagesViews[imageIdx]);

    vk::RenderingInfo renderingInfo{};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.renderArea.extent.width = static_cast<uint32_t>(width);
    renderingInfo.renderArea.extent.height = static_cast<uint32_t>(height);

    uiDrawCb.beginRendering(renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(uiDrawCb));
    uiDrawCb.endRendering();

    vk::ImageMemoryBarrier2 barrierPresent{};
    barrierPresent.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrierPresent.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrierPresent.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrierPresent.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrierPresent.oldLayout = vk::ImageLayout::eAttachmentOptimal;
    barrierPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrierPresent.image = mSwapchainImages[imageIdx];
    barrierPresent.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrierPresent.subresourceRange.levelCount = 1;
    barrierPresent.subresourceRange.layerCount = 1;

    vk::DependencyInfo barrierPresentDepInfo{};
    barrierPresentDepInfo.setImageMemoryBarrierCount(1)
    .setPImageMemoryBarriers(&barrierPresent);
    uiDrawCb.pipelineBarrier2(barrierPresentDepInfo);

    uiDrawCb.end();
    std::vector<vk::PipelineStageFlags> waitStages = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
    };

    if(gui.renderMode == RenderMode::Rasterizer)
        waitStages.push_back(vk::PipelineStageFlagBits::eVertexInput);

    std::vector<vk::CommandBuffer> cbs;
    if(gui.renderMode == RenderMode::Rasterizer)
    {
        cbs.push_back(rasterizer.getActiveCb(frameIdx));
    }
    cbs.push_back(uiDrawCb);

    std::vector<vk::Semaphore> semaphoreWait;
    semaphoreWait.resize(1);
    semaphoreWait[0] = mImageAcquiredSemaphores[frameIdx];
    if(gui.renderMode == RenderMode::Rasterizer)
        semaphoreWait.push_back(vkSceneManager.skinningDoneSem);


    vk::SubmitInfo submitInfo;
    submitInfo.setWaitSemaphoreCount(semaphoreWait.size())
    .setPWaitSemaphores(semaphoreWait.data())
    .setPWaitDstStageMask(waitStages.data())
    .setCommandBufferCount(cbs.size())
    .setPCommandBuffers(cbs.data())
    .setSignalSemaphoreCount(1)
    .setPSignalSemaphores(&mRenderCompleteSemaphores[imageIdx]);
    graphicsQueue.submit(submitInfo, mFences[frameIdx]);


    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphoreCount(1)
    .setPWaitSemaphores(&mRenderCompleteSemaphores[imageIdx])
    .setSwapchainCount(1)
    .setPSwapchains(&mSwapchain)
    .setPImageIndices(&imageIdx);

    graphicsQueue.presentKHR(presentInfo);
}