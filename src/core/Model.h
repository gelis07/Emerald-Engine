#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define BVH_LENGTH 2

enum ModelType
{
    CUBE,
    CUSTOM
};

struct Triangle
{
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 min = glm::vec3(INFINITY);
    glm::vec3 max = glm::vec3(-INFINITY);
};

struct AABB
{
    bool leaf = false;
    glm::vec3 min = glm::vec3(INFINITY);
    glm::vec3 max = glm::vec3(-INFINITY);
    std::vector<Triangle> mTriangleList;
    int nodeA;
    int nodeB;
    int nodeNum = 0;
    int listId;
};

class Model
{
    public:
        void Load(const std::string& path);
        void Load(const std::vector<float>& iVertices, const std::vector<unsigned int>& iIndices);
        void Transform();

        void GetAABBTriangles();
        void SliceAABB(int idx, int axis);
        void ConstructAABBBounds(AABB& aabb);
        int ChooseSliceAxis(const AABB& aabb);


        std::vector<float> mVertices;

        std::vector<unsigned int> mTriangles;
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        //AABB characteristics
        AABB ModelAabb;
        std::vector<AABB> aabbs;
    

        int matIndex = 0;
        ModelType type;
        std::string fileSource;
};

