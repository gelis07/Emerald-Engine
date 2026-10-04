#include "pipelineLayout.hpp"


namespace engine
{
    void RtPipelineLayout::Init(core::context context, vk::DescriptorSetLayout setLayout)
    {
        vk::PipelineLayoutCreateInfo pipLayoutInfo;
        pipLayoutInfo.setSetLayoutCount(1)
        .setSetLayouts(setLayout)
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges(nullptr);
        mPipLayout = context.device.getDevice().createPipelineLayout(pipLayoutInfo);
    }
}