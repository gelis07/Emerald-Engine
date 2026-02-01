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
        float fuzz = 0.0f;
        float refractionIndex = 0.0f;
        glm::vec3 albedo;
        int scatter;
};
class Hittable
{
    public:
        int matIndex;
        HitType type;
};
class HitTriangle : public Hittable
{
    public:
        HitTriangle() {type = TRIANGLE;}
        glm::vec3 a,b,c;
};
class HitSphere : public Hittable
{
    public:
        HitSphere() {type = SPHERE;}
        float radius;
        glm::vec3 point;
};
class Scene
{
    public:
        Camera camera;
        Model model;


        std::vector<Material*> materials;
        std::vector<Hittable*> hitObjects;
        inline Scene()
        {
            Material* mat = new Material;
            mat->albedo = glm::vec3(1.0f);
            mat->fuzz = 0.0f;
            mat->refractionIndex = 0.0f;
            mat->scatter = 1;
            materials.push_back(mat);
        }
        inline ~Scene()
        {
            for (int i ; i < hitObjects.size(); i++)
            {
                delete hitObjects[i];
            }
            for (int i ; i < materials.size(); i++)
            {
                delete materials[i];
            }
        }
};