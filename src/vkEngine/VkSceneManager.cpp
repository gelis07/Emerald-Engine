#include "VkSceneManager.h"
#include <core/Utils.h>


void VkSceneManager::Init(VkContext aContext)
{
    context = aContext;




    InitDesc(context);

    std::vector<char> code = Utils::ReadFileBinary("../Shaders/SkinningMesh.comp.spv");

    vk::ShaderModule shaderModule;
    vk::ShaderModuleCreateInfo moduleCi;
    moduleCi.setPCode(reinterpret_cast<const uint32_t*>(code.data()))
    .setCodeSize(code.size());

    shaderModule = context.device.createShaderModule(moduleCi);

    vk::PipelineShaderStageCreateInfo shaderStageCi;
    shaderStageCi.setStage(vk::ShaderStageFlagBits::eCompute)
    .setModule(shaderModule)
    .setPName("main");

    
    vk::PushConstantRange range;
    range.setOffset(0)
    .setSize(sizeof(pushConstants))
    .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    
    vk::PipelineLayoutCreateInfo layoutCi;
    layoutCi.setSetLayoutCount(1)
    .setPSetLayouts(&setLayout)
    .setPushConstantRangeCount(1)
    .setPPushConstantRanges(&range);
    
    compPipLayout = context.device.createPipelineLayout(layoutCi);

    vk::ComputePipelineCreateInfo compPipCi;
    compPipCi.setLayout(compPipLayout)
    .setStage(shaderStageCi);

    compPip = context.device.createComputePipelines({}, compPipCi)->front();

    vk::CommandBufferAllocateInfo cbAllocCi;
    cbAllocCi.setCommandPool(context.commandPool)
    .setCommandBufferCount(1)
    .setLevel(vk::CommandBufferLevel::ePrimary);

    cb = context.device.allocateCommandBuffers(cbAllocCi).front();

    vk::FenceCreateInfo fCi;
    fence = context.device.createFence(fCi);

    vk::SemaphoreCreateInfo sCi;
    skinningDoneSem = context.device.createSemaphore(sCi);

}


void VkSceneManager::UpdateBoneTransforms(Scene& scene)
{
    std::vector<vkUtils::vkBone> bones;
    vkUtils::evaluateBoneTransforms(scene, bones);
    if(bones.empty())
        bones.push_back({glm::mat4(1.0f)});
    
    vkScene.bones = bones;
    std::memcpy(vkScene.boneTransforms.allocInfo.pMappedData, bones.data(), vkScene.boneTransforms.size);
}


constexpr uint32_t RasterizationMode = 0;
constexpr uint32_t PathTracingMode = 1;

void VkSceneManager::SkinMeshes(Scene& scene, uint32_t renderMode)
{
    context.device.waitForFences(fence, vk::True, UINT64_MAX);
    
    context.device.resetFences(fence);
    vk::CommandBufferBeginInfo beginInfo;
    cb.begin(beginInfo);

    cb.bindPipeline(vk::PipelineBindPoint::eCompute, compPip);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compPipLayout, 0, descSet, nullptr);
    for(int i = 0; i < vkScene.vkMeshes.size(); i++)
    {

        if(scene.models[vkScene.vkMeshes[i].modelIdx].meshes[vkScene.vkMeshes[i].localMeshIdx].bones.empty())
            continue;
        
        pushConstants pc;
        pc.modelId = i;
        pc.vertCount = vkScene.vkMeshes[i].vertCount;
        cb.pushConstants(compPipLayout, vk::ShaderStageFlagBits::eCompute, 0,sizeof(pushConstants), &pc);
        cb.dispatch((vkScene.vkMeshes[i].vertCount + 255) / 256, 1, 1);
        if(renderMode == PathTracingMode)
        {
            vk::BufferMemoryBarrier2 barrier{};
    
            barrier
                .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
                .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR)
                .setBuffer(vkScene.vkMeshes[i].buffer)
                .setOffset(0)
                .setSize(vkScene.vkMeshes[i].vBufSize);
    
            vk::DependencyInfo dependencyInfo{};
            dependencyInfo
                .setBufferMemoryBarrierCount(1)
                .setPBufferMemoryBarriers(&barrier);
    
            cb.pipelineBarrier2(dependencyInfo);
        }
    }


    cb.end();

    std::vector<vk::Semaphore> signalSemaphores;
    if(renderMode == RasterizationMode)
        signalSemaphores.push_back(skinningDoneSem);

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1)
    .setPCommandBuffers(&cb)
    .setSignalSemaphoreCount(signalSemaphores.size());
    if(!signalSemaphores.empty())
        submitInfo.setPSignalSemaphores(signalSemaphores.data());

    context.queue.submit(submitInfo, fence);
}

void VkSceneManager::updateScene(Scene& scene)
{
    vkScene = vkUtils::LoadScene(context, context.commandPool, scene);
    WriteDynamicDescs(context);
}

void VkSceneManager::deleteSceneModels()
{
    for(int i = 0; i < vkScene.vkMeshes.size(); i++)
    {
        vmaDestroyBuffer(context.alloc, vkScene.vkMeshes[i].buffer, vkScene.vkMeshes[i].bufferAllocation);
        vmaDestroyBuffer(context.alloc, vkScene.vkMeshes[i].OriginalVertBuffer.buffer, vkScene.vkMeshes[i].OriginalVertBuffer.allocation);
    }

    vmaDestroyBuffer(context.alloc, vkScene.boneInfluenceBuffer.buffer, vkScene.boneInfluenceBuffer.allocation);
    vmaDestroyBuffer(context.alloc, vkScene.boneTransforms.buffer, vkScene.boneTransforms.allocation);


    vkScene.boneInfluenceVec.clear();
    vkScene.bones.clear();
    vkScene.vkTextures.clear();
    vkScene.vkModels.clear();
    vkScene.vkMeshes.clear();
}

void VkSceneManager::deleteScene()
{
    deleteSceneModels();
    for(int i = 0; i < vkScene.vkTextures.size(); i++)
    {
        vmaDestroyImage(context.alloc, vkScene.vkTextures[i].image, vkScene.vkTextures[i].alloc);
    }
    vkScene.vkTextures.clear();
}

void VkSceneManager::InitDesc(VkContext context)
{

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.resize(4);
    bindings[0].setBinding(0)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    bindings[1].setBinding(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    bindings[2].setBinding(2)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setDescriptorCount(500)
    .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    bindings[3].setBinding(3)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setDescriptorCount(500)
    .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    std::vector<vk::DescriptorBindingFlags> bindingFlags = 
    {
        {},
        {},
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
    flagsCreateInfo.setBindingFlags(bindingFlags);

    vk::DescriptorSetLayoutCreateInfo layoutCi;
    layoutCi.setBindingCount(bindings.size())
    .setPNext(&flagsCreateInfo)
    .setPBindings(bindings.data());

    setLayout = context.device.createDescriptorSetLayout(layoutCi);
        std::vector<vk::DescriptorPoolSize> poolSizes = 
    {
        {
            vk::DescriptorType::eStorageBuffer,
            1002
        }
    };


    vk::DescriptorPoolCreateInfo descPoolCi;
    descPoolCi.setMaxSets(1)
    .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
    .setPPoolSizes(poolSizes.data())
    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    descPool = context.device.createDescriptorPool(descPoolCi);
    
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(descPool)
    .setDescriptorSetCount(1)
    .setPSetLayouts(&setLayout);

    descSet = context.device.allocateDescriptorSets(allocInfo).front();
}
void VkSceneManager::WriteDynamicDescs(VkContext context)
{
    vk::DescriptorBufferInfo bonesBufferInfo;
    bonesBufferInfo.setBuffer(vkScene.boneTransforms.buffer)
    .setOffset(0)
    .setRange(vkScene.boneTransforms.size);

    vk::DescriptorBufferInfo bonesInfluenceBufferInfo;
    bonesInfluenceBufferInfo.setBuffer(vkScene.boneInfluenceBuffer.buffer)
    .setOffset(0)
    .setRange(vkScene.boneInfluenceBuffer.size);

    std::vector<vk::DescriptorBufferInfo> vertexBuffersInfo;
    vertexBuffersInfo.resize(vkScene.vkMeshes.size());
    for(int i = 0; i < vertexBuffersInfo.size(); i++)
    {
        vertexBuffersInfo[i].setBuffer(vkScene.vkMeshes[i].buffer)
        .setOffset(0)
        .setRange(vkScene.vkMeshes[i].vBufSize);
    }

    std::vector<vk::DescriptorBufferInfo> originalVertexBuffersInfo;
    originalVertexBuffersInfo.resize(vkScene.vkMeshes.size());
    for(int i = 0; i < originalVertexBuffersInfo.size(); i++)
    {
        originalVertexBuffersInfo[i].setBuffer(vkScene.vkMeshes[i].OriginalVertBuffer.buffer)
        .setOffset(0)
        .setRange(vkScene.vkMeshes[i].OriginalVertBuffer.size);
    }


    std::vector<vk::WriteDescriptorSet> descWrites;
    descWrites.resize(4);
    descWrites[0].setDstSet(descSet)
    .setDstBinding(0)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&bonesInfluenceBufferInfo);
    descWrites[1].setDstSet(descSet)
    .setDstBinding(1)
    .setDstArrayElement(0)
    .setDescriptorCount(1)
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(&bonesBufferInfo);
    descWrites[2].setDstSet(descSet)
    .setDstBinding(2)
    .setDstArrayElement(0)
    .setDescriptorCount(vertexBuffersInfo.size())
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(vertexBuffersInfo.data());
    descWrites[3].setDstSet(descSet)
    .setDstBinding(3)
    .setDstArrayElement(0)
    .setDescriptorCount(originalVertexBuffersInfo.size())
    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
    .setPBufferInfo(originalVertexBuffersInfo.data());

    context.device.updateDescriptorSets(static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);
}