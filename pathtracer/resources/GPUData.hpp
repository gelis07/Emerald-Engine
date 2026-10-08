#pragma once
#include <buffer.hpp>
#include <glm.hpp>
#include <context.hpp>
namespace engine
{
    struct GPUVertex
    {
        glm::vec3 position;
        glm::vec2 texCoords;
        glm::vec3 normals;
    };

    struct GPUMesh
    {
        glm::mat4 transform;
        uint32_t modelId;
        uint32_t matId;
    };

    struct GPUModel
    {
        glm::mat4 transform;
    };

    struct GPUMaterial
    {
        glm::vec3 albedo;
        float metalness;
        float roughness;
        glm::vec3 emission;
    };
}