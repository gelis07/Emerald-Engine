#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <core/Camera.h>
#include <core/Model.h>
#include <vkEngine/vkBackend.h>

class Scene
{
    public:
        Camera camera;
        std::vector<Model> models;
        std::vector<Material> materials;
        std::vector<TextureVk> textures;


        inline void AddModel(Model&& model)
        { 
            models.push_back(model);
        }
        inline Scene()
        {
            models.reserve(20);
            Material mat;
            mat.albedo = glm::vec3(1.0f);
            mat.name = "Default";
            materials.push_back(mat);
        }
};




