#pragma once
#include <vector>
#include <array>
#include <string>
#include <glm/glm.hpp>
class Model
{
    public:
        void Load(const std::string& path);
        std::vector<glm::vec3> mVertices;
        std::vector<std::array<int,3>> mTriangles;
};