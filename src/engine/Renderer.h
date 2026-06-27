#pragma once
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/GPUBackend.h>


class Renderer
{
    public:
        void RenderSample(RenderSettings& rs);
        void Init(const RenderSettings& rs, int width, int heigth);
        void Render(RenderSettings& rs);
        int frameCount = 0;
    private:
        void AABBSetupGPU(const Scene& scene);
        void UpdateSettings(RenderSettings& rs);
        Shader Raytracer;
        Shader postProcessing;

        unsigned int ModelTexture;
        std::vector<int> mModelAabbIdcs;
        int frames = 0;
        float tfov;
        float AR;
        int mSceneTriCount;
        int prevWindowWidth, prevWindowHeight, prevModelCount;
        double lastTime;
        
        std::vector<int> TextureIds;
        bool hasCreatedBuffers = false;
        GLuint SkyTexture;
        GLuint ModelInfo;
        GLuint MeshInfo;
        GLuint AABBIndices;
        GLuint TrianglesSSBO;
        GLuint AABBInfo;

};