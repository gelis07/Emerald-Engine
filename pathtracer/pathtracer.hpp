#pragma once
#include <context.hpp>
#include "bvh/tlas.hpp"
#include "bvh/blas.hpp"
#include "resources/resources.hpp"
#include "rtpipeline/pipeline.hpp"

namespace engine
{
    struct RenderSettings
    {
        uint32_t ImageWidth, ImageHeight;
    };


    class Pathtracer
    {
        public:
        void Init(core::context context, const Scene& scene, RenderSettings settings);
        void Run(core::context context);

        vk::Image getRenderTarget () const {return mRenderTarget;}
        vk::CommandPool getCommandPool () const {return mCommandPool;}
        private:

        void createCommandBuffer(core::context context);
        void createRenderTarget(core::context context);

        vk::Fence fence;
        RenderSettings mSettings;
        std::vector<BLAS> mBlases;
        vk::CommandPool mCommandPool;
        vk::CommandBuffer mCb;
        Resources mResources;
        Pipeline mPip;
        TLAS mTlas;

        vk::Image mRenderTarget;
        vk::ImageView mRenderTargetView;
    };
}