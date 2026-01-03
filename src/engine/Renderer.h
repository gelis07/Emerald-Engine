#pragma once
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/GPUBackend.h>

class Renderer
{
    public:
        void OnUpdate(RenderSettings& rs);
        void Init(int width, int heigth);
    private:
        GLuint VBO, VAO;

        Shader Screen;
        Shader Raytracer;
        Shader PostProcessing;

        GLuint RenderImage;
        GLuint PostProcessingImage;
        int frames;
        float tfov;
        float AR;
        glm::vec3 camPos = glm::vec3(0,0,0);
        const unsigned int TEXTURE_WIDTH = 1000, TEXTURE_HEIGHT = 1000;
};