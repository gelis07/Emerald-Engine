#pragma once
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/GPUBackend.h>

class Renderer
{
    public:
        void Render(RenderSettings& rs);
        void Init(const RenderSettings& rs, int width, int heigth);
        GLuint RenderImage;

    private:
        void CreateRenderImage(int width, int height);
        void CreateTriangleSSBO(const RenderSettings& rs);
        void AABBSetupGPU(const Scene& scene);
        std::vector<float> BakeModel(const std::vector<Model>& models);

        Shader Raytracer;

        std::vector<int> mModelAabbIdcs;
        GLuint VerticesSSBO;
        GLuint AABBInfo;
        int frames = 1;
        float tfov;
        float AR;
        int prevWindowWidth, prevWindowHeight, prevModelCount;

};