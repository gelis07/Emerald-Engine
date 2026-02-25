#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
class Model
{
    public:
        void Load(const std::string& path);
        std::vector<float> mVertices;
        std::vector<int> mTriangles;
};