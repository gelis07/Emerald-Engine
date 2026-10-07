#pragma once
#include "scene.hpp"
#include <context.hpp>

namespace engine
{
    class Resources
    {
        public:
        void Init(core::context context, Scene scene
        , vk::ImageView renderTargetView, vk::AccelerationStructureKHR accel);

        vk::DescriptorSetLayout getSetLayout() const {return mSetLayout;}
        vk::DescriptorSet getDescSet() const {return mDescSet;}
        private:
        void setUpDescriptors(core::context context);
        void writeDescriptors(core::context context, Scene scene
            , vk::ImageView renderTargetView, vk::AccelerationStructureKHR accel);
        
        
        void createSceneBuffer(core::context context, core::buffer& buffer, unsigned char* data, uint32_t size);
        void setUpMeshesBuffer(core::context context, Scene scene);
        void setUpModelsBuffer(core::context context, Scene scene);
        void setUpMaterialsBuffer(core::context context, Scene scene);
        vk::DescriptorSetLayout mSetLayout;
        vk::DescriptorSet mDescSet;
        vk::DescriptorPool mDescPool;

        core::buffer meshesBuffer;
        core::buffer modelsBuffer;
        core::buffer materialsBuffer;
        core::buffer cameraUniformBuffer;
    };
}