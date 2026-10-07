#include "pathtracer.hpp"


namespace engine
{
    void Pathtracer::Init(core::context context, const Scene& scene, RenderSettings settings)
    {
        mSettings = settings;

        vk::CommandPoolCreateInfo commandPoolCi;
        commandPoolCi.setQueueFamilyIndex(context.device.getComputeFamily())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        mCommandPool = context.device.getDevice().createCommandPool(commandPoolCi);
        fence = context.device.getDevice().createFence({});
        
        mBlases.resize(scene.models.size());
        std::vector<core::BottomAccStructTlasRef> blasRef;
        blasRef.resize(scene.models.size());
        for(uint32_t i = 0; i < scene.models.size(); i++)
        {
            const Model& model = scene.models[i]; 
            for(uint32_t j = 0; j < model.meshes.size(); j++)
            {
                const Mesh& mesh = model.meshes[j];
                BLAS blas;
                blas.Init(context, mCommandPool, mesh.getBuffer().buffer, 
                sizeof(GPUVertex), mesh.getIndexCount(), mesh.getVertexCount());
                mBlases[i] = blas;

                blasRef[i].transform = model.transform * mesh.transform;
                blasRef[i].accel = mBlases[i].getBlas().accel;
            }
        }
        createRenderTarget(context);

        mTlas.Init(context, mCommandPool, blasRef);
        mResources.Init(context, scene, mRenderTargetView, mTlas.getTlas().accel);
        mPip.Init(context, mResources.getSetLayout());

        createCommandBuffer(context);
    }

    void Pathtracer::Run(core::context context)
    {
        vk::SubmitInfo subInfo;
        subInfo.setCommandBufferCount(1)
        .setPCommandBuffers(&mCb);

        context.device.getQueue().submit(subInfo, fence);

        vk::Result result = context.device.getDevice().waitForFences(1, &fence, true, UINT64_MAX);
        context.device.getDevice().resetFences(fence);
    }

    void Pathtracer::createCommandBuffer(core::context context)
    {
        vk::CommandBufferAllocateInfo cbAllocInfo;
        cbAllocInfo.setCommandPool(mCommandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);
        
        mCb = context.device.getDevice().allocateCommandBuffers(cbAllocInfo).front();
        vk::CommandBufferBeginInfo beginInfo{};
        mCb.begin(beginInfo);

        vk::MemoryBarrier2 barrierAcc{};
        barrierAcc.setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
            .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
            .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
            .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR);

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setMemoryBarrierCount(1)
                    .setPMemoryBarriers(&barrierAcc);

        mCb.pipelineBarrier2(dependencyInfo);

        vk::ImageMemoryBarrier2 barriers;

        barriers.setSrcAccessMask(vk::AccessFlagBits2::eNoneKHR)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setImage(mRenderTarget)
        .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);


        vk::DependencyInfo depInfo;
        depInfo.setPImageMemoryBarriers(&barriers)
        .setImageMemoryBarrierCount(1);
        mCb.pipelineBarrier2(depInfo);

        mCb.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,mPip.getPip());

        std::vector<vk::DescriptorSet> descSetsToBind = {mResources.getDescSet()};
        mCb.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, mPip.getPipLayout().getPipLayout(),
        0, descSetsToBind, nullptr);
        mCb.traceRaysKHR(
        mPip.getBindingTable().getRayGenAddressRegion(),
        mPip.getBindingTable().getMissAddressRegion(),
        mPip.getBindingTable().getChitAddressRegion(), {}
        , mSettings.ImageWidth, mSettings.ImageHeight, 1, context.device.getLoader());

        mCb.end();
    }

    void Pathtracer::createRenderTarget(core::context context)
    {
        //Creating render target.
        vk::ImageCreateInfo rendTargCi;
        rendTargCi.setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .extent.setWidth(mSettings.ImageWidth).setHeight(mSettings.ImageHeight).setDepth(1);
        rendTargCi.setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
        .setInitialLayout(vk::ImageLayout::eUndefined);
        
        VmaAllocation imgAlloc{};
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        
        vmaCreateImage(context.alloc.getAlloc(),
        reinterpret_cast<VkImageCreateInfo*>(&rendTargCi),
        &imgAllocInfo,
        reinterpret_cast<VkImage*>(&mRenderTarget),
        &imgAlloc, nullptr);
        
        vk::ImageViewCreateInfo rendTargImageViewCi;
        rendTargImageViewCi.setImage(mRenderTarget)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .components.r = vk::ComponentSwizzle::eR;
        rendTargImageViewCi.components.g = vk::ComponentSwizzle::eG;
        rendTargImageViewCi.components.b = vk::ComponentSwizzle::eB;
        rendTargImageViewCi.components.a = vk::ComponentSwizzle::eA;
        rendTargImageViewCi.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1);
        
        mRenderTargetView = context.device.getDevice().createImageView(rendTargImageViewCi);
    }
}