#pragma once
#include <vector>
#include <glm.hpp>

enum HitType
{
    SPHERE
};
class Material
{
    public:
        glm::vec3 Color;
        float EmmisionPower;
        float mult;
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