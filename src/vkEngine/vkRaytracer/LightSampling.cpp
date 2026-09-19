#include "LightSampling.h"
#include "fmt/base.h"
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

        std::vector<WalkersAlias> walkersAliasTriangles = createAliasVector(pdf, 1.0, 1.0);

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

    std::vector<WalkersAlias> meshesWalkersAlias = createAliasVector(pdf, 1.0, 1.0);
    createBuffer(context, sizeof(WalkersAlias) * meshesWalkersAlias.size(), (uint32_t*)meshesWalkersAlias.data()
    , &WalkersAliasLights);

    createBuffer(context, sizeof(MeshLightGPU) * meshLights.size(), (uint32_t*)meshLights.data()
    , &LightProbs);

    mTotalPower += sceneLightsPower;
}

//the float pdf indices should correspond to an actual vector of the stuff you need.
std::vector<WalkersAlias>
LightSampler::createAliasVector(const std::vector<float>& pdf, float total, float axisLength)
{
    const uint32_t n = static_cast<uint32_t>(pdf.size());

    std::vector<WalkersAlias> table(n);

    std::vector<float> scaled(n);

    std::queue<uint32_t> small;
    std::queue<uint32_t> large;

    float intervalSize = axisLength / n;

    for (uint32_t i = 0; i < n; ++i)
    {
        scaled[i] = pdf[i] * axisLength / total;

        table[i].startIdx = i;
        table[i].aliasIdx = UINT32_MAX;
        table[i].prob = intervalSize;

        if (scaled[i] < intervalSize)
            small.push(i);
        else
            large.push(i);
    }

    while (!small.empty() && !large.empty())
    {
        uint32_t s = small.front();
        small.pop();

        uint32_t l = large.front();
        large.pop();

        table[s].prob = scaled[s];
        table[s].aliasIdx = l;

        scaled[l] -= (intervalSize - scaled[s]);

        if (scaled[l] < intervalSize)
            small.push(l);
        else
            large.push(l);
    }

    // Anything left is exactly full.
    while (!large.empty())
    {
        uint32_t i = large.front();
        large.pop();

        table[i].prob = intervalSize;
        table[i].aliasIdx = UINT32_MAX;
    }

    while (!small.empty())
    {
        uint32_t i = small.front();
        small.pop();

        table[i].prob = intervalSize;
        table[i].aliasIdx = UINT32_MAX;
    }

    return table;
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

const float pi = 3.14159265359;
const float axisLength = 10000.0;
void LightSampler::setUpSkyboxLightSampler(VkContext context, float* pixels, uint32_t count, uint32_t width, uint32_t height)
{
    float totalPower = 0.0f;
    std::vector<float> pdf;
    pdf.resize(count);

    for(int i = 0; i < count; i++)
    {
        pdf[i] = pixels[i * 4];
        totalPower += pixels[i*4];
    }
    uint32_t idx = 0;
    float maxProb = 0.0f;
    float maxPower;

    for(int i = 0; i < count; i++)
    {
        float test = pdf[i];
        // pdf[i] /= totalPower;
        if(pdf[i] > maxProb)
        {
            maxPower = test;
            maxProb = pdf[i];
            idx = i;
        }
    }

    fmt::println("max power: {}", maxPower);

    uint32_t u = idx % width;
    uint32_t v = idx / width;

    fmt::println("uvs: {}, {}", u, v);
    
    float uf = (float(u)) / float(width);
    float vf = (float(v)) / float(height);
    fmt::println("uvsf: {}, {}", uf, vf);

    float theta = pi * vf;
    float phi = 2 * pi * uf - pi;
    fmt::println("spherical {}, {}", theta, phi);
    glm::vec3 dir;
    dir.x = cos(phi) * sin(theta);
    dir.y = cos(theta);
    dir.z = sin(phi) * sin(theta);
    fmt::println("direction {}, {}, {}", dir.x, dir.y, dir.z);

    mTotalPower += totalPower;
    skyboxPower = totalPower;
    intervalLength = axisLength / pdf.size();
    std::vector<WalkersAlias> skybox = createAliasVector(pdf, totalPower, axisLength);
    createBuffer(context, sizeof(WalkersAlias) * skybox.size(), (uint32_t*)skybox.data()
    , &SkyboxWalkersAlias);
    hasSkybox = true;
}