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
        GLuint PostProcessingImage;
    private:
        void CreateRenderImage(int width, int height);
        void CreateTriangleSSBO(const RenderSettings& rs);
        std::vector<float> BakeModel(const std::vector<Model>& models);

        Shader Raytracer;
        Shader PostProcessing;

        GLuint ITriSSBO;
        GLuint VerticesSSBO;
        GLuint RenderImage;
        int frames = 1;
        float tfov;
        float AR;
        int prevWindowWidth, prevWindowHeight, prevModelCount;

};