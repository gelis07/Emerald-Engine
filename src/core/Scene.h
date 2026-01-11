#pragma once
#include <vector>
#include <glm/glm.hpp>

enum HitType
{
    SPHERE
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
        Material mat;
        glm::vec3 point;
        HitType type;
};

class HitSphere : public Hittable
{
    public:
        HitSphere() {type = SPHERE;}
        float radius;
};
class Scene
{
    public:
        std::vector<Hittable*> hitObjects;
        inline ~Scene()
        {
            for (int i ; i < hitObjects.size(); i++)
            {
                delete hitObjects[i];
            }
        }
};