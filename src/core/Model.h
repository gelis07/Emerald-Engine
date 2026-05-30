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



struct Vertex
{
    glm::vec3 position;
    glm::vec2 texCoords;
};
struct Texture
{
    unsigned int id=-1;
    std::string path="";
};
struct Triangle
{
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 min = glm::vec3(INFINITY);
    glm::vec3 max = glm::vec3(-INFINITY);
    glm::vec2 texA = glm::vec2(-1);
    glm::vec2 texB = glm::vec2(-1);
    glm::vec2 texC = glm::vec2(-1);
    int texId;
    int meshIdx;
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

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Texture texture;
};
struct ModelConstructData
{
    std::vector<Mesh> meshes;
    std::string path;
};
class Model
{
    public:
        void Load(const ModelConstructData& data);
        void Transform();
        //AABB characteristics
        AABB ModelAabb;
        std::vector<AABB> aabbs;

        //Transformation
        glm::mat4 GetModelInverse();
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        int matIndex = 0;


        void GetAABBTriangles();
        void SliceAABB(int idx, int axis);
        void ConstructAABBBounds(AABB& aabb);
        int ChooseSliceAxis(const AABB& aabb);

        std::vector<Mesh> mMeshes;
        ModelType type;
        std::string fileSource;
};

inline std::vector<unsigned int> CubeIndices {
    //Top
    2, 6, 7,
    2, 3, 7,

    //Bottom
    0, 4, 5,
    0, 1, 5,

    //Left
    0, 2, 6,
    0, 4, 6,

    //Right
    1, 3, 7,
    1, 5, 7,

    //Front
    0, 2, 3,
    0, 1, 3,

    //Back
    4, 6, 7,
    4, 5, 7
};
inline std::vector<Vertex> CubeVertices {
    // 0: Bottom-Front-Left -> mapped to bottom-left of texture
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f) }, 
    // 1: Bottom-Front-Right -> mapped to bottom-right of texture
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f) }, 
    // 2: Top-Front-Left -> mapped to top-left of texture
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f) }, 
    // 3: Top-Front-Right -> mapped to top-right of texture
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f) }, 
    // 4: Bottom-Back-Left -> mapped to bottom-right (for clean wrapping from Left face)
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f) }, 
    // 5: Bottom-Back-Right -> mapped to bottom-left (for clean wrapping from Right face)
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f) }, 
    // 6: Top-Back-Left -> mapped to top-right 
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f) }, 
    // 7: Top-Back-Right -> mapped to top-left
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f) }  
};
inline std::vector<glm::vec3> CubeVerticesVec3 {
    glm::vec3(-1, -1,  1), //0
        glm::vec3(1, -1,  1), //1
    glm::vec3(-1,  1,  1), //2
        glm::vec3(1,  1,  1), //3
    glm::vec3(-1, -1, -1), //4
        glm::vec3(1, -1, -1), //5
    glm::vec3(-1,  1, -1), //6
        glm::vec3(1,  1, -1)  //7
};