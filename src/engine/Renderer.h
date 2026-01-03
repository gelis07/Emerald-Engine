#pragma once
#include <glad/glad.h>
#include <core/RenderSettings.h>
#include <gtc/type_ptr.hpp>
#include <engine/GPUBackend.h>

class Renderer
{
    public:
        void OnUpdate(RenderSettings& rs);
        void Init(int width, int heigth);
    private:
        Shader Compute;
        Shader Screen;

        GLuint VBO, VAO;
        GLuint RenderImage;
        int frames;
        float tfov;
        float AR;
        glm::vec3 camPos = glm::vec3(0,0,0);
        const unsigned int TEXTURE_WIDTH = 1000, TEXTURE_HEIGHT = 1000;
};