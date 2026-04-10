#include "Renderer.h"
#include <glm/glm.hpp>
#include <vector>
#include <fmt/base.h>

void Renderer::OnUpdate(RenderSettings& rs)
{
    if(rs.accumulate)
    {
        frames++;
    }else{
        frames = 1;
    }
    if(rs.scene.camera.moved)
    {
        frames = 1;
        rs.accumulate = false;
    }


    if(prevModelCount != rs.scene.models.size() || rs.ReloadScene)
    {
        CreateTriangleSSBO(rs);
        prevModelCount = rs.scene.models.size();
        rs.ReloadScene = false;
    }

    if(prevWindowWidth != rs.ImgWidth || prevWindowHeight != rs.ImgHeight)
    {
        CreateRenderImage(rs.ImgWidth, rs.ImgHeight);
        prevWindowHeight = rs.ImgHeight;
        prevWindowWidth = rs.ImgWidth;
    }
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, PostProcessingImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);
    Raytracer.Bind();
    Raytracer.Uniform1f("AR", AR);
    Raytracer.Uniform1f("tfov", tfov);
    Raytracer.Uniform3f("CamPos", rs.scene.camera.GetPos());
    Raytracer.Uniform1i("SphereCount", rs.scene.hitObjects.size());

    Raytracer.Uniform1i("frameIndex", frames);
    Raytracer.Uniform1i("accumulate", rs.accumulate);
    Raytracer.Uniform1i("skyColor", rs.EnvLight);
    Raytracer.UniformMat4("InvProj", rs.scene.camera.GetInvProjection());
    Raytracer.UniformMat4("InvView", rs.scene.camera.GetInvView());
    for(int i = 0; i < rs.scene.hitObjects.size(); i++)
    {
        Hittable* HitObj = rs.scene.hitObjects[i];
        std::string indexString = std::to_string(i);
        
        if(HitObj->type == SPHERE)
        {
            Raytracer.Uniform1i(std::string("Spheres[" + indexString + "].matIndex"), HitObj->matIndex);
            Raytracer.Uniform1f(std::string("Spheres[" + indexString + "].radius"), static_cast<HitSphere*>(HitObj)->radius);
            Raytracer.Uniform3f(std::string("Spheres[" + indexString + "].point"),static_cast<HitSphere*>(HitObj)->point);
        }
    }

    for(int i = 0; i < rs.scene.models.size(); i++)
    {
        std::string indexString = std::to_string(i);
        Raytracer.UniformMat4(std::string("modelsInfo[" + indexString + "].matrixModel"), rs.scene.models[i].model);
        Raytracer.Uniform1i(std::string("modelsInfo[" + indexString + "].matIndex"), rs.scene.models[i].matIndex);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, VerticesSSBO);

    size_t triCount = 0;
    for (int i = 0; i < rs.scene.models.size(); i++)
    {
        triCount += rs.scene.models[i].mTriangles.size();
    }

    Raytracer.Uniform1i("TriCount", triCount);


    for(int i = 0; i < rs.scene.materials.size(); i++)
    {
        const Material& mat = (*rs.scene.materials[i]);
        std::string indexString = std::to_string(i);
        Raytracer.Uniform3f(std::string("materials[" + indexString + "].albedo"), mat.albedo);
        Raytracer.Uniform3f(std::string("materials[" + indexString + "].emmColor"), mat.emmColor);
        Raytracer.Uniform1i(std::string("materials[" + indexString + "].materialType"), mat.scatter);
        Raytracer.Uniform1f(std::string("materials[" + indexString + "].fuzz"), mat.fuzz);
        Raytracer.Uniform1f(std::string("materials[" + indexString + "].refractionIndex"), mat.refractionIndex);
    }

    glDispatchCompute((unsigned int)rs.ImgWidth/16, (unsigned int)rs.ImgHeight/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    PostProcessing.Bind();
    PostProcessing.Uniform1i("PPframeIndex", frames);

    glDispatchCompute((unsigned int)rs.ImgWidth/16, (unsigned int)rs.ImgHeight/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Renderer::Init(const RenderSettings& rs, int width, int heigth)
{

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, rs.ImgWidth, rs.ImgHeight);

    std::vector<float> QuadVertices = 
    {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    CreateRenderImage(rs.ImgWidth, rs.ImgHeight);


    CreateTriangleSSBO(rs);

    Raytracer.Init();
    PostProcessing.Init();

    Raytracer.LinkShader("../Shaders/shader.comp", GL_COMPUTE_SHADER);
    PostProcessing.LinkShader("../Shaders/PostProcessing.comp", GL_COMPUTE_SHADER);


    tfov = glm::tan(3.14159 / 8);
    AR = (double)width / (double)heigth;

}
void Renderer::CreateTriangleSSBO(const RenderSettings& rs)
{
    std::vector<float> bake = BakeModel(rs.scene.models);

    glCreateBuffers(1, &VerticesSSBO);
    glNamedBufferStorage(VerticesSSBO, sizeof(float) * bake.size(), (const void*) bake.data(), 0);
}
std::vector<float> Renderer::BakeModel(const std::vector<Model>& models)
{
    std::vector<float> vertices;
    for (int m = 0; m < models.size(); m++)
    {
        const Model& model = models[m];
        for (int i =0; i < model.mTriangles.size(); i++)
        {
            vertices.push_back(model.mVertices[(model.mTriangles[i]) * 3]);
            vertices.push_back(model.mVertices[(model.mTriangles[i]) * 3 + 1]);
            vertices.push_back(model.mVertices[(model.mTriangles[i]) * 3 + 2]);
            vertices.push_back(m);
        }
    }
    return vertices;
}

void Renderer::CreateRenderImage(int width, int height)
{
    glDeleteTextures(1, &RenderImage);
    glDeleteTextures(1, &PostProcessingImage);

    glGenTextures(1, &RenderImage);
    glBindTexture(GL_TEXTURE_2D, RenderImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);

    glGenTextures(1, &PostProcessingImage);
    glBindTexture(GL_TEXTURE_2D, PostProcessingImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(1, PostProcessingImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);

}