#pragma once
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>

struct RasterizedModel
{
    Model* model;
    GLuint vbo;
    GLuint vao;
    GLuint ibo;
    int indicesCount = 0;
};

class Rasterizer
{
    public:
        void Init(RenderSettings& rs, int width, int height);
        void Update(RenderSettings& rs);
        GLuint renderTexture;
    private:
        void AddModels(RenderSettings& rs);
        void CalculateSceneAABB(RenderSettings& rs);
        void CreateTexture(int width, int height);
        Shader RastShader;
        std::vector<RasterizedModel> rastModels;
        GLuint frameBuffer;
        GLuint depthTexture;
        GLuint renderBuffer;
        int prevWindowWidth, prevWindowHeight;

        int lastModelCount = 0;
        GLuint cubeVbo, cubeIbo, cubeVao;
};