#pragma once
#include <buffer.hpp>
#include <glm.hpp>
#include <context.hpp>
namespace engine
{
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


    class Model
    {
        public:
        void InitBuffer(core::context context, const std::vector<Vertex> vertices, const std::vector<uint32_t> indices);

        core::buffer getBuffer() const {return vertBuffer;}


        uint32_t getVertexCount() const {return vCount;};
        uint32_t getIndexCount() const {return iCount;};

        uint32_t getVSize() const {return vSize;};
        uint32_t getISize() const {return iSize;};

        glm::mat4 transform;
        private:
        core::buffer vertBuffer; //Contains both index and vertex data.

        uint32_t vCount;
        uint32_t iCount;

        uint32_t vSize;
        uint32_t iSize;
    };
}