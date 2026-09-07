#include "LightSampling.h"
#include <queue>

struct MeshLightGPU
{
    float prob;
    uint32_t triCount;
    uint32_t modelIdx;
};


void LightSampler::setUpSceneLightPbs(VkContext context,Scene* scene, const vkUtils::vkScene& vkScene)
{
    sceneLightsPower = 0.0f;
    lightCount = 0;
    for(int m = 0; m < vkScene.vkMeshes.size(); m++)
    {
        const vkUtils::VkMesh& model = vkScene.vkMeshes[m];
        if(scene->materials[*model.matIdx].emmColor.x == 0.0
        && scene->materials[*model.matIdx].emmColor.y == 0.0
        && scene->materials[*model.matIdx].emmColor.z == 0.0)
        {
            continue;
        }
        vkMeshLightProp meshLightProp;
        Mesh& mesh = scene->models[model.modelIdx].meshes[model.localMeshIdx];

        std::vector<float> pdf;
        for(int i = 0; i < mesh.indices.size(); i+=3)
        {
            triPickingDataGPU triData;
            glm::vec3 a = mesh.vertices[mesh.indices[i]].position;
            glm::vec3 b = mesh.vertices[mesh.indices[i+1]].position;
            glm::vec3 c = mesh.vertices[mesh.indices[i+2]].position;

            glm::vec3 ab = b-a;
            glm::vec3 ac = c-a;

            float area = 0.5f * glm::length(glm::cross(ab, ac));
            float brightness = glm::length(scene->materials[mesh.matIndex].emmColor);
            float power = area * brightness;
            meshLightProp.totalPower += power;
            triData.prob = power; // Not yet the probability. Gotta normalize with total power.
            meshLightProp.trianglesData.push_back(triData);
        }
        meshLightProp.idx = m;
        meshLightProp.prob = meshLightProp.totalPower;
        sceneLightsPower += meshLightProp.totalPower;
        for(int i = 0; i < meshLightProp.trianglesData.size(); i++)
        {
            meshLightProp.trianglesData[i].prob /= meshLightProp.totalPower;
            pdf.push_back(meshLightProp.trianglesData[i].prob);
        }

        std::vector<WalkersAlias> walkersAliasTriangles = createAliasVector(pdf);

        VkUtilBuffer triangleWalkersAliasBuffer;
        createBuffer(context, sizeof(WalkersAlias) * walkersAliasTriangles.size(), (uint32_t*)walkersAliasTriangles.data()
        ,&triangleWalkersAliasBuffer);

        VkUtilBuffer triangleProbBuffer;

        createBuffer(context, sizeof(triPickingDataGPU) * meshLightProp.trianglesData.size()
        , (uint32_t*)meshLightProp.trianglesData.data(), &triangleProbBuffer);

        mMeshLighProps.push_back(meshLightProp);

        WalkersAliasMeshTriangles.push_back(triangleWalkersAliasBuffer);
        TriangleProbs.push_back(triangleProbBuffer);
        lightCount++;
    }

    std::vector<MeshLightGPU> meshLights;
    std::vector<float> pdf;
    for(int i = 0; i < mMeshLighProps.size(); i++)
    {
        mMeshLighProps[i].prob /= sceneLightsPower;
        pdf.push_back(mMeshLighProps[i].prob);
        MeshLightGPU meshLight;
        meshLight.prob = mMeshLighProps[i].prob;
        meshLight.modelIdx = mMeshLighProps[i].idx;
        meshLight.triCount = mMeshLighProps[i].trianglesData.size();
        meshLights.push_back(meshLight);
    }

    std::vector<WalkersAlias> meshesWalkersAlias = createAliasVector(pdf);
    createBuffer(context, sizeof(WalkersAlias) * meshesWalkersAlias.size(), (uint32_t*)meshesWalkersAlias.data()
    , &WalkersAliasLights);

    createBuffer(context, sizeof(MeshLightGPU) * meshLights.size(), (uint32_t*)meshLights.data()
    , &LightProbs);

    mTotalPower += sceneLightsPower;
}

//the float pdf indices should correspond to an actual vector of the stuff you need.
std::vector<WalkersAlias> LightSampler::createAliasVector(const std::vector<float>& pdf)
{
    std::queue<placeInVector> lessThatAvg;
    std::queue<placeInVector> greaterThatAvg;
    std::vector<WalkersAlias> final;
    const uint32_t n = pdf.size();
    for (uint32_t i = 0; i < n; i++)
    {
        if(std::abs(pdf[i] - 1.0f/n) < 0.0001)
        {
            final.push_back({1.0f/n, i, UINT32_MAX});
        }
        else if(pdf[i] < 1.0f/n)
        {
            lessThatAvg.push({pdf[i], i});
        }else if(pdf[i] > 1.0f/n)
        {
            greaterThatAvg.push({pdf[i], i});
        }
    }

    while(!lessThatAvg.empty())
    {
        float underProb = lessThatAvg.front().prob;
        float& overProb = greaterThatAvg.front().prob;
        overProb -= 1.0/n - underProb;

        final.push_back({underProb * n, lessThatAvg.front().idx, greaterThatAvg.front().idx});
        lessThatAvg.pop();

        if(std::abs(overProb - 1.0f/n) < 0.0001f)
        {
            final.push_back({1.0f/n, greaterThatAvg.front().idx, UINT32_MAX});
            greaterThatAvg.pop();
        }
        else if(overProb < 1.0f/n)
        {
            lessThatAvg.push({overProb, greaterThatAvg.front().idx});
            greaterThatAvg.pop();
        }
    }

    return final;
}

void LightSampler::createBuffer(VkContext context, uint32_t size, uint32_t* data, VkUtilBuffer* buffer)
{
    vk::BufferCreateInfo bufferCi;
    bufferCi.setSize(size)
    .setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

    VmaAllocationCreateInfo allocCi{};
    allocCi.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocCi.usage = VMA_MEMORY_USAGE_AUTO;


    vmaCreateBuffer(context.alloc, reinterpret_cast<VkBufferCreateInfo*>(&bufferCi), &allocCi
    , reinterpret_cast<VkBuffer*>(&buffer->buffer), &buffer->allocation
    , &buffer->allocInfo);

    std::memcpy(buffer->allocInfo.pMappedData, data, size);
    buffer->size = size;
}


void LightSampler::deleteScene(VkContext context)
{
    WalkersAliasMeshTriangles.clear();
    TriangleProbs.clear();
    mMeshLighProps.clear();

    // if(hasSkybox)
    // {
        // vmaDestroyBuffer(context.alloc, static_cast<VkBuffer>(SkyboxWalkersAlias.buffer), SkyboxWalkersAlias.allocation);
        // hasSkybox = false;
    // }
    if(lightCount == 0) return;
    mTotalPower = skyboxPower;
    
    vmaDestroyBuffer(context.alloc, static_cast<VkBuffer>(WalkersAliasLights.buffer), WalkersAliasLights.allocation);
    vmaDestroyBuffer(context.alloc, static_cast<VkBuffer>(LightProbs.buffer), LightProbs.allocation);
    for(int i = 0; i < WalkersAliasMeshTriangles.size(); i++)
    {
        vmaDestroyBuffer(context.alloc, static_cast<VkBuffer>(WalkersAliasMeshTriangles[i].buffer), WalkersAliasMeshTriangles[i].allocation);
        vmaDestroyBuffer(context.alloc, static_cast<VkBuffer>(TriangleProbs[i].buffer), TriangleProbs[i].allocation);
    }

}


void LightSampler::setUpSkyboxLightSampler(VkContext context, float* pixels, uint32_t count)
{
    float totalPower = 0.0f;
    std::vector<float> pdf;
    pdf.resize(count);
    for(int i = 0; i < count; i++)
    {
        pdf[i] = pixels[i*4];
        totalPower += pixels[i*4];
    }
    for(int i = 0; i < count; i++)
    {
        pdf[i] /= totalPower;
    }
    mTotalPower += totalPower;
    skyboxPower = totalPower;
    std::vector<WalkersAlias> skybox = createAliasVector(pdf);
    createBuffer(context, sizeof(WalkersAlias) * skybox.size(), (uint32_t*)skybox.data()
    , &SkyboxWalkersAlias);
    hasSkybox = true;
}