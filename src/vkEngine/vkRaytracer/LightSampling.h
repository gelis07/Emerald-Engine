#pragma once
#include <vkEngine/VkEngineApiBinding.h>
#include <cstdint>
#include <vector>
//See walkers alias method for sampling a discrete pdf in O(1)
struct WalkersAlias //Matches GPU description.
{
    float prob;
    uint32_t startIdx;
    uint32_t aliasIdx;
};
struct triPickingDataGPU
{
    float prob;
};
struct placeInVector
{
    float prob;
    uint32_t idx;
};

//for lighting.
struct vkMeshLightProp
{
    float prob; // probability of the mesh compared to other meshes.
    uint32_t idx; // idx that corresponds to a vkModel
    float totalPower = 0.0f;
    std::vector<triPickingDataGPU> trianglesData;
};

class LightSampler
{
    public:
        void setUpSceneLightPbs(VkContext context,Scene* scene, const vkUtils::vkScene& vkScene);
        void deleteScene(VkContext context);
        void setUpSkyboxLightSampler(VkContext context, float* pixels, uint32_t count);

        VkUtilBuffer WalkersAliasLights; 
        VkUtilBuffer LightProbs;
        VkUtilBuffer SkyboxWalkersAlias;
        std::vector<VkUtilBuffer> WalkersAliasMeshTriangles;
        std::vector<VkUtilBuffer> TriangleProbs; 
        uint32_t lightCount = 0;
        float skyboxPower = 0.0f;
        float sceneLightsPower = 0.0f;
        float mTotalPower = 0.0f;   
    private:
        std::vector<WalkersAlias> createAliasVector(const std::vector<float>& pdf);
        void createBuffer(VkContext context, uint32_t size, uint32_t* data, VkUtilBuffer* buffer);
        bool hasSkybox = false;
        std::vector<vkMeshLightProp> mMeshLighProps;
};