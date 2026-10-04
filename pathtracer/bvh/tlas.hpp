#pragma once
#include <context.hpp>
#include <accelerationStructure.hpp>
#include <utils.hpp>
#include <cstdlib>

namespace engine
{
    class TLAS
    {
        public:
        void Init(core::context context,
        vk::CommandPool commandPool, const std::vector<core::BottomAccStructTlasRef> bottomAccel);

        core::TopAccStruct getTlas()
        {
            if(tlas.accel == VK_NULL_HANDLE)
            {
                CORE_PRINT("this tlas doesn't exist");
                std::exit(EXIT_FAILURE);
            }

            return tlas;
        }
        private:
        core::TopAccStruct tlas;
    };
}