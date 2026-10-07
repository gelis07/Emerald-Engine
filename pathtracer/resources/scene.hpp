#pragma once
#include "model.hpp"
#include <gtc/matrix_transform.hpp>

namespace engine
{

    inline static glm::mat4 CalcInvViewFromDirection(glm::vec3 pos, glm::vec3 dir)
    {
        glm::mat4 view(1.0f);
        view = glm::lookAt(pos, pos + dir, glm::vec3(0, 1, 0));
        return glm::inverse(view);
    }

    struct GPUCamera
    {
        glm::vec3 pos;
        uint32_t frameIdx;
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