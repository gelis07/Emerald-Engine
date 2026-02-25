#pragma once
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/GPUBackend.h>

class Renderer
{
    public:
        void OnUpdate(RenderSettings& rs);
        void Init(const RenderSettings& rs, int width, int heigth);
    private:

        std::vector<float> BakeModel(const Model& model);

        GLuint VBO, VAO;

        Shader Screen;
        Shader Raytracer;
        Shader PostProcessing;

        GLuint ITriSSBO;
        GLuint VerticesSSBO;
        GLuint RenderImage;
        GLuint PostProcessingImage;
        int frames = 1;
        float tfov;
        float AR;
        const unsigned int TEXTURE_WIDTH = 1000, TEXTURE_HEIGHT = 1000;
};