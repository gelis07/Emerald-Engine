#include "resources.hpp"
#include <utils.hpp>

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

        setUpMaterialsBuffer(context, scene);
        setUpModelsBuffer(context, scene);
        setUpMeshesBuffer(context, scene);
        
        setUpDescriptors(context);
        writeDescriptors(context, scene, renderTargetView, accel);
    }

    void Resources::setUpDescriptors(core::context context)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.resize(8);
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
        bindings[3].setBinding(5)
        .setDescriptorCount(214)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
        bindings[4].setBinding(6)
        .setDescriptorCount(214)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
        bindings[5].setBinding(7)
        .setDescriptorCount(214)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
        bindings[6].setBinding(3)
        .setDescriptorCount(214)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);
        bindings[7].setBinding(4)
        .setDescriptorCount(214)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

        std::vector<vk::DescriptorBindingFlags> bindingFlags = {
        {},
        {},
        {},
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
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

        vk::DescriptorBufferInfo meshesBufferInfo;
        meshesBufferInfo.setBuffer(meshesBuffer.buffer)
        .setOffset(0)
        .setRange(meshesBuffer.size);

        vk::DescriptorBufferInfo modelsBufferInfo;
        modelsBufferInfo.setBuffer(modelsBuffer.buffer)
        .setOffset(0)
        .setRange(modelsBuffer.size);

        vk::DescriptorBufferInfo materialsBufferInfo;
        materialsBufferInfo.setBuffer(materialsBuffer.buffer)
        .setOffset(0)
        .setRange(materialsBuffer.size);


        std::vector<vk::DescriptorBufferInfo> vertexBuffersInfos;
        std::vector<vk::DescriptorBufferInfo> indexBuffersInfos;
        for(int i = 0; i < scene.models.size(); i++)
        {
            const Model& model = scene.models[i];
            for(int j = 0; j < model.meshes.size(); j++)
            {
                const Mesh& mesh = model.meshes[j];
                vk::DescriptorBufferInfo vBuffer;
                vBuffer.setBuffer(mesh.getBuffer().buffer)
                .setOffset(0)
                .setRange(sizeof(GPUVertex) * mesh.getVertexCount());
                vertexBuffersInfos.push_back(vBuffer);

                vk::DescriptorBufferInfo iBuffer;
                iBuffer.setBuffer(mesh.getBuffer().buffer)
                .setOffset(sizeof(GPUVertex) * mesh.getVertexCount())
                .setRange(sizeof(uint32_t) * mesh.getIndexCount());
                indexBuffersInfos.push_back(iBuffer);
            }
        }

        std::vector<vk::WriteDescriptorSet> descWrites;
        descWrites.resize(8);
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
        descWrites[3].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setBufferInfo(meshesBufferInfo)
        .setDstBinding(7)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[4].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setBufferInfo(modelsBufferInfo)
        .setDstBinding(5)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[5].setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setBufferInfo(materialsBufferInfo)
        .setDstBinding(6)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[6].setDescriptorCount(indexBuffersInfos.size())
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(indexBuffersInfos.data())
        .setDstBinding(3)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);
        descWrites[7].setDescriptorCount(vertexBuffersInfos.size())
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setPBufferInfo(vertexBuffersInfos.data())
        .setDstBinding(4)
        .setDstArrayElement(0)
        .setDstSet(mDescSet);

        context.device.getDevice().updateDescriptorSets(descWrites.size(), descWrites.data(), 0, nullptr);
    }
    void Resources::createSceneBuffer(core::context context, core::buffer& buffer,unsigned char* data, uint32_t size)
    {
        vk::BufferCreateInfo meshBufferCi;
        meshBufferCi.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setSize(size);

        VmaAllocationCreateInfo meshBuffAllocCi{};
        meshBuffAllocCi.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        meshBuffAllocCi.usage = VMA_MEMORY_USAGE_AUTO;
        
        vmaCreateBuffer(context.alloc.getAlloc(), 
        reinterpret_cast<VkBufferCreateInfo*>(&meshBufferCi),
        &meshBuffAllocCi, 
        reinterpret_cast<VkBuffer*>(&buffer.buffer),
        &buffer.allocation,
        &buffer.allocInfo);

        std::memcpy(buffer.allocInfo.pMappedData, data, size);
        buffer.size = size;
    }

    void Resources::setUpMeshesBuffer(core::context context, Scene scene)
    {
        std::vector<GPUMesh> gpuMeshes;
        for(int i = 0; i < scene.models.size(); i++)
        {
            const Model& model = scene.models[i];
            for(int j = 0; j < model.meshes.size(); j++)
            {
                const Mesh& mesh = model.meshes[j];
                GPUMesh gpuMesh;
                gpuMesh.matId = mesh.matId;
                gpuMesh.modelId = i;
                gpuMesh.transform = mesh.transform;
                gpuMeshes.push_back(gpuMesh);
            }
        }
        

        createSceneBuffer(context, meshesBuffer, reinterpret_cast<unsigned char*>(gpuMeshes.data())
        ,sizeof(Mesh) * gpuMeshes.size());
    }

    void Resources::setUpModelsBuffer(core::context context, Scene scene)
    {
        std::vector<GPUModel> gpuModels;
        gpuModels.resize(scene.models.size());
        for(int i = 0; i < scene.models.size(); i++)
        {
            const Model& model = scene.models[i];
            GPUModel gpuModel;
            gpuModel.transform = model.transform;
            gpuModels[i] = gpuModel;
        }

        createSceneBuffer(context, modelsBuffer, reinterpret_cast<unsigned char*>(gpuModels.data())
        ,sizeof(Model) * gpuModels.size());
    }

    void Resources::setUpMaterialsBuffer(core::context context, Scene scene)
    {
        std::vector<GPUMaterial> gpuMaterials;
        gpuMaterials.resize(scene.materials.size());
        for(int i = 0; i < scene.materials.size(); i++)
        {
            //Making a different struct for future proofing.
            const Material& material = scene.materials[i];
            GPUMaterial gpuMaterial;
            gpuMaterial.albedo = material.albedo;
            gpuMaterial.metalness = material.metalness;
            gpuMaterial.roughness = material.roughness;
            gpuMaterials[i] = gpuMaterial;
        }

        createSceneBuffer(context, materialsBuffer, reinterpret_cast<unsigned char*>(gpuMaterials.data())
        ,sizeof(GPUMaterial) * gpuMaterials.size());
    }
}