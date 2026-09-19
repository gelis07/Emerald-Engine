#pragma once
#include "VkEngineApiBinding.h"



struct pushConstants
{
    uint32_t modelId;
    uint32_t vertCount;
};

class VkSceneManager
{
    public:
        //Init doesn't create a scene.
        void Init(VkContext aContext);
        void UpdateBoneTransforms(Scene& scene);
        void SkinMeshes(Scene& scene, uint32_t renderMode);
        void updateScene(Scene& scene);
        void deleteSceneModels();
        void deleteScene();
        vkUtils::vkScene* GetScenePointer() { return &vkScene; }; 
        vk::Semaphore skinningDoneSem;
        vk::Fence fence;
    private:

        vk::CommandBuffer cb;

        vk::DescriptorSet descSet;
        vk::DescriptorSetLayout setLayout;
        vk::DescriptorPool descPool;

        void InitDesc(VkContext context);
        void WriteDynamicDescs(VkContext context);

        vk::Pipeline compPip;
        vk::PipelineLayout compPipLayout;
        VkContext context;
        vkUtils::vkScene vkScene;
};