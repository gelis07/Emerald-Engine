#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum ModelType
{
    CUBE,
    CUSTOM
};

class Model
{
    public:
        void Load(const std::string& path);
        void Load(const std::vector<float>& iVertices, const std::vector<unsigned int>& iIndices);
        void Transform();
        std::vector<float> mVertices;
        std::vector<unsigned int> mTriangles;
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        int matIndex = 0;
        ModelType type;
        std::string fileSource;
};

