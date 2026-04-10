#pragma once
#include "core/Scene.h"
#include <engine/GPUBackend.h>
#include <core/RenderSettings.h>

struct RasterizedModel
{
    Model* model;
    GLuint vbo;
    GLuint vao;
    GLuint ibo;
};

class Rasterizer
{
    public:
        void Init(Scene*, int width, int height);
        void Update(const RenderSettings& rs);
        GLuint renderTexture;
    private:
        Shader RastShader;
        Scene* activeScene;
        RasterizedModel rastModel;
        GLuint frameBuffer;
        GLuint depthTexture;
        GLuint renderBuffer;
};