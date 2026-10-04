#include "resources.hpp"


namespace engine
{
    void Resources::Init(core::context context, Scene scene
    , vk::ImageView renderTargetView, vk::AccelerationStructureKHR accel)
    {
        vk::BufferCreateInfo sdCi;
        sdCi.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        .setSize(static_cast<uint32_t>(sizeof(GPUCamera)));

        VmaAllocationCreateInfo sdAllocCi{};
        sdAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        sdAllocCi.usage = VMA_MEMORY_USAGE_AUTO;
        
        vmaCreateBuffer(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&sdCi),
        &sdAllocCi, 
        reinterpret_cast<VkBuffer*>(&cameraUniformBuffer.buffer),
        &cameraUniformBuffer.allocation,
        &cameraUniformBuffer.allocInfo);

        std::memcpy(cameraUniformBuffer.allocInfo.pMappedData, &scene.camera, sizeof(GPUCamera));

        setUpDescriptors(context);
        writeDescriptors(context, scene, renderTargetView, accel);
    }

    void Resources::setUpDescriptors(core::context context)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.resize(3);
        bindings[0].setBinding(0)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setDescriptorCount(1)
        .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);
        bindings[1].setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
        .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
        bindings[2].setBinding(2)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);

        std::vector<vk::DescriptorBindingFlags> bindingFlags = {
        {},
        {},
        {},
        };

        vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
        flagsCreateInfo.setBindingFlags(bindingFlags);

        vk::DescriptorSetLayoutCreateInfo layoutCi;
        
        layoutCi.setBindingCount(static_cast<uint32_t>(bindings.size()))
        .setPNext(&flagsCreateInfo)
        .setPBindings(bindings.data());

        mSetLayout = context.device.getDevice().createDescriptorSetLayout(layoutCi);

        std::vector<vk::DescriptorPoolSize> poolSizes = 
        {
            {
                vk::DescriptorType::eAccelerationStructureKHR,
                1
            },
            {
                vk::DescriptorType::eStorageImage,
                2
            },
            {
                vk::DescriptorType::eCombinedImageSampler,
                1000
            },
            {
                vk::DescriptorType::eUniformBuffer,
                2
            },
            {
                vk::DescriptorType::eStorageBuffer,
                1500
            }
        };


        vk::DescriptorPoolCreateInfo descPoolCi;
        descPoolCi.setMaxSets(1)
        .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
        .setPPoolSizes(poolSizes.data())
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

        mDescPool = context.device.getDevice().createDescriptorPool(descPoolCi);
        
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool(mDescPool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&mSetLayout);

        mDescSet = context.device.getDevice().allocateDescriptorSets(allocInfo).front();
    }

    void Resources::writeDescriptors(core::context context, Scene scene
    , vk::ImageView renderTargetView, vk::AccelerationStructureKHR accel)
    {
        vk::DescriptorImageInfo renderTargetImageInfo;
        renderTargetImageInfo.setImageView(renderTargetView)
        .setImageLayout(vk::ImageLayout::eGeneral);

        vk::WriteDescriptorSetAccelerationStructureKHR accelWrite;
        accelWrite.setAccelerationStructureCount(1)
        .setAccelerationStructures(accel);

        vk::DescriptorBufferInfo sdInfo;
        sdInfo.setBuffer(cameraUniformBuffer.buffer)
        .setOffset(0)
        .setRange(sizeof(GPUCamera));

        std::vector<vk::WriteDescriptorSet> descWrites;
        descWrites.resize(3);
        descWrites[0].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setImageInfo(renderTargetImageInfo)
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[1].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
        .setPNext(&accelWrite)
        .setDstBinding(1)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[2].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setBufferInfo(sdInfo)
        .setDstBinding(2)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);

        context.device.getDevice().updateDescriptorSets(descWrites.size(), descWrites.data(), 0, nullptr);
    }
}