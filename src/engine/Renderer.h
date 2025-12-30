#pragma once
#include <glad/glad.h>
#include <core/RenderSettings.h>
#include <gtc/type_ptr.hpp>


class Renderer
{
    public:
        void OnUpdate(RenderSettings rs);
        void Init(int width, int heigth);
    private:
        GLuint VBO, VAO;
        GLuint BasicProgram;
        GLuint ComputeShaderID;
        GLuint RenderImage;
        int frames;
        float tfov;
        float AR;
        glm::vec3 camPos = glm::vec3(0,0,0);
        const unsigned int TEXTURE_WIDTH = 1000, TEXTURE_HEIGHT = 1000;
};