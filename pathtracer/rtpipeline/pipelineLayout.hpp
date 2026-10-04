#pragma once
#include <context.hpp>

namespace engine
{
    class RtPipelineLayout
    {
        public:
        void Init(core::context context, vk::DescriptorSetLayout setLayout);

        vk::PipelineLayout getPipLayout() const {return  mPipLayout;}
        private:
        vk::PipelineLayout mPipLayout;
    };
}