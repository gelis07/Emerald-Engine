#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vkEngine/vkBackend.h>
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
    glm::vec3 normals;
};

struct Material
{
    float roughness = 1.0;
    glm::vec3 albedo;
    glm::vec3 emmColor = glm::vec3(0,0,0);
    uint32_t albedoTexture = -1;

    std::string name;
};

struct TextureData
{
    unsigned char* data;
    int width, height, channels;
    std::string path;
};
struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int matIndex = 0;

};
struct ModelConstructData
{
    std::vector<Mesh> meshes;
    std::vector<TextureVk> textureData;
    std::vector<Material> materials;
    std::string path;
};
class Model
{
    public:
        void Load(const ModelConstructData& data);
        void Transform();

        //Transformation
        glm::mat4 GetModelInverse();
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        std::vector<Mesh> mMeshes;
        ModelType type;
        std::string fileSource;
};

inline std::vector<uint32_t> CubeIndices {
    // Front          // Back           // Top
    0, 1, 2,  2, 1, 3,  4, 5, 6,  6, 5, 7,  8, 9, 10, 10, 9, 11,
    // Bottom         // Right          // Left
    12,13,14, 14,13,15, 16,17,18, 18,17,19, 20,21,22, 22,21,23
};
inline std::vector<Vertex> CubeVertices {
    // --- Front Face (+Z) ---
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) },

    // --- Back Face (-Z) ---
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f) },

    // --- Top Face (+Y) ---
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

    // --- Bottom Face (-Y) ---
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },

    // --- Right Face (+X) ---
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

    // --- Left Face (-X) ---
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) }
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