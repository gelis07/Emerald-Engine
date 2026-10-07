#pragma once
#include <buffer.hpp>
#include <glm.hpp>
#include <context.hpp>
#include "GPUData.hpp"
namespace engine
{
    class Mesh
    {
        public:
        void InitBuffer(core::context context, const std::vector<GPUVertex> vertices, const std::vector<uint32_t> indices);
        core::buffer getBuffer() const {return vertBuffer;}


        uint32_t getVertexCount() const {return vCount;};
        uint32_t getIndexCount() const {return iCount;};

        uint32_t getVSize() const {return vSize;};
        uint32_t getISize() const {return iSize;};
        glm::mat4 transform;
        uint32_t matId;
        private:
        core::buffer vertBuffer; //Contains both index and vertex data.

        uint32_t vCount;
        uint32_t iCount;

        uint32_t vSize;
        uint32_t iSize;
    };


    class Model
    {
        public:
        std::vector<Mesh> meshes;
        glm::mat4 transform;
    };
}