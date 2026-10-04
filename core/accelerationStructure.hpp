#pragma once
#include "buffer.hpp"
#include <glm.hpp>
namespace core
{
    
    struct BottomAccStruct
    {
        vk::AccelerationStructureKHR accel = VK_NULL_HANDLE;
        core::buffer StructureBuffer;
        core::buffer ScratchBuffer;
    };

    struct BottomAccStructTlasRef
    {
        vk::AccelerationStructureKHR accel = VK_NULL_HANDLE;
        glm::mat4 transform;  
    };

    struct TopAccStruct
    {
        vk::AccelerationStructureKHR accel = VK_NULL_HANDLE;
        core::buffer StructureBuffer;
        core::buffer ScratchBuffer;
        core::buffer InstBuffer;
    };
}