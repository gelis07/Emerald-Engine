#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <core/Camera.h>
#include <core/Model.h>
enum HitType
{
    SPHERE,
    TRIANGLE
};
class Material
{
    public:
        float metallic = 1.0;
        float roughness = 1.0;
        glm::vec3 albedo;
        glm::vec3 emmColor = glm::vec3(0,0,0);
        int scatter;
};

class Scene
{
    public:
        Camera camera;
        std::vector<Model> models;
        std::vector<Material*> materials;

        //AABB characteristics
        AABB aabb;
        std::vector<AABB> allAABBs; 
        inline void AddModel(Model&& model)
        { 
            models.push_back(model);
        }
        inline Scene()
        {
            models.reserve(20);
            Material* mat = new Material;
            mat->albedo = glm::vec3(1.0f);
            mat->scatter = 1;
            materials.push_back(mat);
        }
        inline ~Scene()
        {
            for (int i ; i < materials.size(); i++)
            {
                delete materials[i];
            }
        }
};




