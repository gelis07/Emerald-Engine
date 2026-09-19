#pragma once
#include <vkEngine/VkEngineApiBinding.h>
#include <core/Model.h>
#include <core/RenderSettings.h>
#include "ASManager.h"
#include "LightSampling.h"
#include "RtPipeline.h"
#include <core/Animator.h>
#include <vkEngine/VkSceneManager.h>


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



struct CameraShaderData
{
    alignas(16) glm::vec3 pos;
    uint32_t lightCount;
    int32_t frameIdx;

    alignas(16) glm::mat4 invProj;
    alignas(16) glm::mat4 invView;

    float skyboxProb;
    float totalSkyboxPower;
    uint32_t skybox;
    float intervalLength;
};
struct MeshDataGPU
{
    uint32_t globalIdx;
    uint32_t modelIdx;
    uint32_t matIdx;
    glm::mat4 localTransform;
};
struct MaterialShaderData
{
    glm::vec3 albedo;
    glm::vec3 emmColor;
    float roughness;
    float metalness;
    float idr;
    float transmittance;

    uint32_t albedoMap = UINT32_MAX;
    uint32_t roughnessMap = UINT32_MAX;
    uint32_t metallicnesMap = UINT32_MAX;
    uint32_t normalMap = UINT32_MAX;

};
struct ModelDataGPU
{
    glm::mat4 modelMatrix;
};

class vkRaytracer
{
    public:
        void Init(RaytracerInitInfo info);
        void Run(RaytracerRenderInfo info);
        void destroy(vk::Device device, VmaAllocator alloc);
        vk::Image renderTargetImage;
        vk::ImageView renderTargetImageView;
        vk::Image sumImage;
        vk::ImageView sumImageView;
        uint32_t frameIdx;
        uint32_t frameIdxForRender;
        VmaAllocationInfo shaderDataAllocInfo{};
        void ExportToPng(VmaAllocator alloc,vk::Device device, const std::string& filename);
        
        vkUtils::vkScene* mVkScene;
        void resetFrameIdx();
        Animator* anim;
    private:
        vk::detail::DispatchLoaderDynamic dynamicDispatchLoader;

        VkContext mContext;
        ASManager asManager;
        RtPip rtPip;
        LightSampler lightSampler;
        bool hasSkybox = false;

        vk::Fence fence;
        vk::CommandBuffer cb;
        vk::Queue mComputeQueue;
        vk::CommandPool mCommandPool;


        vk::Sampler texturSampler;
        TextureVk skybox;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSet descSet;
        vk::DescriptorSetLayout setLayout;
        vk::DescriptorPool descPool;

        VkUtilBuffer ModelBuffer;
        VkUtilBuffer MeshBuffer;
        VkUtilBuffer MaterialBuffer;

        int mLastModelSize = 0;
        void createSceneBuffers(VmaAllocator alloc, const Scene& scene);
        void fillSceneBuffers(const Scene& scene);
        void deleteSceneBuffers(VmaAllocator alloc);

        void exportAnimFrame(RaytracerRenderInfo info);
        void playAnim(RaytracerRenderInfo info);
        bool prevAnimState = false;
        void reloadScene(RaytracerRenderInfo info);
        void setUpDescriptors();
        //Descriptors that are submitted once in the init.
        void writeStaticDescriptors(const vkUtils::vkScene& vkScene, const Scene& scene);
        //Descriptors that change on update.
        void writeDynamicDescriptors(const vkUtils::vkScene& vkScene, const Scene& scene);
        void UpdateModels(const vk::Device& device, const VmaAllocator& alloc, Scene& scene);
        void CreateRayTracingCB(const vk::Device& device, BuildASOnGPUInfo ASBuildInfo, bool init);
};