#pragma once
#include <context.hpp>
#include <accelerationStructure.hpp>
#include <utils.hpp>

/*
    Creating bottom level acceleration structures using vulkan.
*/

namespace engine
{
    class BLAS
    {
        public:
        /*
        buffer is the buffer containing vertex and index data.
        */
        void Init(core::context context, vk::CommandPool commandPool
        , vk::Buffer buffer, uint32_t vSize, uint32_t iCount, uint32_t vCount);

        core::BottomAccStruct getBlas() {
            if(blas.accel == VK_NULL_HANDLE)
            {
                CORE_PRINT("This blas doesn't exist");
                std::exit(EXIT_FAILURE);
            }
            return blas;
        }
        private:
        core::BottomAccStruct blas;
    };
}