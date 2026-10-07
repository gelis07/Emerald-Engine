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
        uint32_t samples;
    };


    class Pathtracer
    {
        public:
        void Init(core::context context, const Scene& scene, RenderSettings settings);
        void Run(core::context context, const Scene& scene);

        vk::Image getRenderTarget () const {return mRenderTarget;}
        vk::Image getSumImage () const {return mSumImage;}
        vk::CommandPool getCommandPool () const {return mCommandPool;}
        private:

        void createCommandBuffer(core::context context);
        void createRenderTarget(core::context context);
        void createSumImage(core::context context);
        vk::Fence fence;
        RenderSettings mSettings;
        std::vector<BLAS> mBlases;
        vk::CommandPool mCommandPool;
        vk::CommandBuffer mCb;
        Resources mResources;
        Pipeline mPip;
        TLAS mTlas;

        uint32_t frameIdx = 0;
        vk::Image mSumImage;
        vk::ImageView mSumImageView;
        vk::Image mRenderTarget;
        vk::ImageView mRenderTargetView;
    };
}