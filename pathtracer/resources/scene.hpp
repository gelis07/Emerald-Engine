#pragma once
#include "model.hpp"
namespace engine
{

    struct GPUCamera
    {
        glm::vec3 pos;
        uint32_t pad;
        glm::mat4 invProj;
        glm::mat4 invView;
    };

    struct Scene
    {
        std::vector<Model> models;
        GPUCamera camera;
    };
}