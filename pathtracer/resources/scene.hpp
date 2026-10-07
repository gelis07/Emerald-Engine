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
    //Making a different struct for future proofing.
    struct Material
    {
        float roughness;
        glm::vec3 albedo;
        float metalness;
    };

    struct Scene
    {
        std::vector<Model> models;
        std::vector<Material> materials;
        GPUCamera camera;
    };
}