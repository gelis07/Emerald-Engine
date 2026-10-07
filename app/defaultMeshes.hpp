#pragma once
#include <resources/model.hpp>
#include <gtc/matrix_transform.hpp>

inline std::vector<uint32_t> CubeIndices {

    // Front          // Back           // Top
    0, 1, 2,  2, 1, 3,  4, 5, 6,  6, 5, 7,  8, 9, 10, 10, 9, 11,

    // Bottom         // Right          // Left
    12,13,14, 14,13,15, 16,17,18, 18,17,19, 20,21,22, 22,21,23
};

// GPUVertex layout: { position, texCoords, normals }
inline std::vector<engine::GPUVertex> CubeVertices {

    // --- Front Face (+Z) ---
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f) },
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3( 0.0f,  0.0f,  1.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3( 0.0f,  0.0f,  1.0f) },

    // --- Back Face (-Z) ---
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3( 0.0f,  0.0f, -1.0f) },

    // --- Top Face (+Y) ---
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3( 0.0f,  1.0f,  0.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3( 0.0f,  1.0f,  0.0f) },

    // --- Bottom Face (-Y) ---
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3( 0.0f, -1.0f,  0.0f) },

    // --- Right Face (+X) ---
    { glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f) },
    { glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f) },
    { glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec2(0.0f, 1.0f), glm::vec3( 1.0f,  0.0f,  0.0f) },
    { glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3( 1.0f,  0.0f,  0.0f) },

    // --- Left Face (-X) ---
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f) },
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f,  0.0f,  0.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f,  0.0f,  0.0f) }
};


inline std::vector<engine::GPUVertex> PlaneVertices
{
    // Bottom-Left
    { glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

    // Bottom-Right
    { glm::vec3( 1.0f, 0.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

    // Top-Left
    { glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },

    // Top-Right
    { glm::vec3( 1.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) }
};

inline std::vector<uint32_t> PlaneIndices
{
    0, 1, 2,
    2, 1, 3
};

inline std::vector<engine::GPUVertex> CreateSphereVertices()
{
    constexpr uint32_t segments = 24;
    constexpr uint32_t rings = 16;

    std::vector<engine::GPUVertex> vertices;

    // ---------------------------------------------------------
    // Top pole
    // ---------------------------------------------------------

    vertices.push_back({
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.5f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
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

            float x = radius * std::cos(theta);
            float z = radius * std::sin(theta);

            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);

            vertices.push_back({
                position,
                glm::vec2(u, v),
                normal
            });
        }
    }

    // ---------------------------------------------------------
    // Bottom pole
    // ---------------------------------------------------------

    vertices.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.5f, 1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)
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

inline std::vector<engine::GPUVertex> SphereVertices = CreateSphereVertices();
inline std::vector<uint32_t> SphereIndices = CreateSphereIndices();