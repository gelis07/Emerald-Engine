#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vkEngine/vkBackend.h>
#include <unordered_map>

enum ModelType
{
    CUBE,
    CUSTOM,
    PLANE,
    SPHERE
};
const std::string ROOT_NODE = "root";

struct Vertex
{
    glm::vec3 position;
    glm::vec2 texCoords;
    glm::vec3 normals;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    uint32_t boneOffset = 0;
    uint32_t boneCount = 0;
};

struct Material
{
    float roughness = 1.0;
    float metalness = 1.0;
    float idr = 1.0f;
    float transmittance = 0.0f;
    glm::vec3 albedo;
    glm::vec3 emmColor = glm::vec3(0,0,0);


    uint32_t albedoTexture = -1;
    uint32_t roughnessTexture = -1;
    uint32_t metallicnesTexture = -1;
    uint32_t normalTexture = -1;

    std::string name;
};

struct TextureData
{
    unsigned char* data;
    int width, height, channels;
    std::string path;
};


struct transformAnimation
{
  float sTime;
  float lTime;
  uint32_t nodeId;
  uint32_t modelId;
};
struct NodeData
{
    glm::mat4 transform;
    uint32_t parentId = -1;
    std::string parentName;
    std::string name;
    std::vector<transformAnimation> transAnims;

};
struct Bone
{
    glm::mat4 offset;
    uint32_t nodeId = -1;
    std::string name;
};

struct BoneInfluece
{
    uint32_t boneId;
    float weight;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    uint32_t matIndex = 0;
    uint32_t nodeId = -1;
    std::vector<Bone> bones;
    std::unordered_map<uint32_t, std::vector<BoneInfluece>> boneInfluencesMap;
};
struct ModelConstructData
{
    std::vector<Mesh> meshes;
    std::vector<TextureVk> textureData;
    std::vector<Material> materials;
    std::vector<NodeData> nodeData;
    std::string path;
};
class Model
{
    public:
      std::string name;
        void Load(const ModelConstructData& data);

        glm::mat4 GetModelInverse();
        glm::mat4 model = glm::mat4(1.0f);

        std::vector<NodeData> nodeData;
        std::vector<Mesh> meshes;
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
    // T = +X, B = +Y
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },


    // --- Back Face (-Z) ---
    // T = -X, B = +Y
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },


    // --- Top Face (+Y) ---
    // T = +X, B = -Z
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f) },

    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f) },

    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f) },

    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f) },


    // --- Bottom Face (-Y) ---
    // T = +X, B = +Z
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3( 0.0f, -1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f) },

    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3( 0.0f, -1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f) },

    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3( 0.0f, -1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f) },

    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3( 0.0f, -1.0f,  0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f) },


    // --- Right Face (+X) ---
    // T = -Z, B = +Y
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3( 1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f, -1.0f),
      glm::vec3( 0.0f, 1.0f, 0.0f) },


    // --- Left Face (-X) ---
    // T = +Z, B = +Y
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) },

    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f),
      glm::vec3(-1.0f,  0.0f,  0.0f),
      glm::vec3( 0.0f,  0.0f,  1.0f),
      glm::vec3( 0.0f,  1.0f,  0.0f) }
};


inline std::vector<Vertex> PlaneVertices
{
    // Bottom-Left
    {
        glm::vec3(-1.0f, 0.0f,  1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    },

    // Bottom-Right
    {
        glm::vec3( 1.0f, 0.0f,  1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    },

    // Top-Left
    {
        glm::vec3(-1.0f, 0.0f, -1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    },

    // Top-Right
    {
        glm::vec3( 1.0f, 0.0f, -1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    }
};

inline std::vector<uint32_t> PlaneIndices
{
    0, 1, 2,
    2, 1, 3
};

inline std::vector<Vertex> CreateSphereVertices()
{
    constexpr uint32_t segments = 24;
    constexpr uint32_t rings = 16;

    std::vector<Vertex> vertices;

    // ---------------------------------------------------------
    // Top pole
    // ---------------------------------------------------------

    vertices.push_back({
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.5f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),

        // Tangent is mathematically undefined at the pole.
        glm::vec3(1.0f, 0.0f, 0.0f),

        // B = N x T
        glm::vec3(0.0f, 0.0f, -1.0f)
    });

    // ---------------------------------------------------------
    // Middle rings
    // ---------------------------------------------------------

    for (uint32_t ring = 1; ring < rings; ring++)
    {
        float v = float(ring) / float(rings);
        float phi = glm::pi<float>() * v;

        float y = std::cos(phi);
        float radius = std::sin(phi);

        for (uint32_t segment = 0; segment < segments; segment++)
        {
            float u = float(segment) / float(segments);
            float theta = 2.0f * glm::pi<float>() * u;

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = radius * cosTheta;
            float z = radius * sinTheta;

            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);

            glm::vec3 tangent(
                -sinTheta,
                 0.0f,
                 cosTheta
            );

            glm::vec3 bitangent(
                cosPhi * cosTheta,
                -sinPhi,
                cosPhi * sinTheta
            );

            vertices.push_back({
                position,
                glm::vec2(u, v),
                normal,
                tangent,
                bitangent
            });
        }
    }

    // ---------------------------------------------------------
    // Bottom pole
    // ---------------------------------------------------------

    vertices.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.5f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),

        // Arbitrary tangent at pole
        glm::vec3(1.0f, 0.0f, 0.0f),

        // N x T
        glm::vec3(0.0f, -0.0f, 1.0f)
    });

    return vertices;
}

inline std::vector<uint32_t> CreateSphereIndices()
{
    constexpr uint32_t segments = 24;
    constexpr uint32_t rings = 16;

    std::vector<uint32_t> indices;

    const uint32_t topIndex = 0;
    const uint32_t bottomIndex = 1 + (rings - 1) * segments;

    // ---------------------------------------------------------
    // Top cap
    // ---------------------------------------------------------

    for (uint32_t segment = 0; segment < segments; segment++)
    {
        uint32_t current = 1 + segment;
        uint32_t next = 1 + ((segment + 1) % segments);

        indices.push_back(topIndex);
        indices.push_back(next);
        indices.push_back(current);
    }

    // ---------------------------------------------------------
    // Middle
    // ---------------------------------------------------------

    for (uint32_t ring = 0; ring < rings - 2; ring++)
    {
        uint32_t currentRing = 1 + ring * segments;
        uint32_t nextRing = currentRing + segments;

        for (uint32_t segment = 0; segment < segments; segment++)
        {
            uint32_t current = currentRing + segment;
            uint32_t next = currentRing + ((segment + 1) % segments);

            uint32_t below = nextRing + segment;
            uint32_t belowNext = nextRing + ((segment + 1) % segments);

            // First triangle
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(below);

            // Second triangle
            indices.push_back(next);
            indices.push_back(belowNext);
            indices.push_back(below);
        }
    }

    // ---------------------------------------------------------
    // Bottom cap
    // ---------------------------------------------------------

    uint32_t bottomRing = bottomIndex - segments;

    for (uint32_t segment = 0; segment < segments; segment++)
    {
        uint32_t current = bottomRing + segment;
        uint32_t next = bottomRing + ((segment + 1) % segments);

        indices.push_back(current);
        indices.push_back(next);
        indices.push_back(bottomIndex);
    }

    return indices;
}

inline std::vector<Vertex> SphereVertices = CreateSphereVertices();
inline std::vector<uint32_t> SphereIndices = CreateSphereIndices();