#pragma once
#include <context.hpp>
#include <shaderModules.hpp>
#include "bindingTable.hpp"
#include "pipelineLayout.hpp"


/*
    The pipeline gets created here. It also acts as an orchestrator for the rest of the scripts in the folder
    (binding table, layout etc)
*/

namespace engine
{
    class Pipeline
    {
        public:
        void Init(core::context context, vk::DescriptorSetLayout setLayout);

        vk::Pipeline getPip() const {return mPipeline;}
        bindingTable getBindingTable() const {return mBTable;}
        RtPipelineLayout getPipLayout() const {return mRtPipLayout;}
        private:
        vk::Pipeline mPipeline;
        bindingTable mBTable;
        RtPipelineLayout mRtPipLayout;
    };
}