#include "Renderer.h"
#include <glm.hpp>
#include <vector>

void Renderer::OnUpdate(RenderSettings& rs)
{
    if(rs.accumulate)
    {
        frames++;
    }else{
        frames = 1;
    }

    Raytracer.Bind();
    Raytracer.Uniform1f("AR", AR);
    Raytracer.Uniform1f("tfov", tfov);
    Raytracer.Uniform3f("CamPos", camPos);
    Raytracer.Uniform1i("SphereCount", rs.scene.hitObjects.size() + 1);

    for(int i = 0; i < rs.scene.hitObjects.size(); i++)
    {
        Hittable* HitObj = rs.scene.hitObjects[i];
        std::string indexString = std::to_string(i);
        Raytracer.Uniform3f(std::string("SPoint[" + indexString + "]"),HitObj->point);
        Raytracer.Uniform3f(std::string("Color[" + indexString + "]"), HitObj->mat.Color);
        if(HitObj->type == SPHERE)
        {
            Raytracer.Uniform1f(std::string("SRadius[" + indexString + "]"), static_cast<HitSphere*>(HitObj)->radius);
        }
    }


    glDispatchCompute((unsigned int)TEXTURE_WIDTH/16, (unsigned int)TEXTURE_HEIGHT/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    PostProcessing.Bind();

    glDispatchCompute((unsigned int)TEXTURE_WIDTH/16, (unsigned int)TEXTURE_HEIGHT/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    glBindVertexArray(VAO);
    Screen.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, PostProcessingImage);
    Screen.Uniform1i("RenderImage", 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void Renderer::Init(int width, int heigth)
{
    std::vector<float> QuadVertices = 
    {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, QuadVertices.size() * sizeof(float), QuadVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float)));


    const unsigned int TEXTURE_WIDTH = 1000, TEXTURE_HEIGHT = 1000;
    glGenTextures(1, &RenderImage);
    glBindTexture(GL_TEXTURE_2D, RenderImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);



    glGenTextures(1, &PostProcessingImage);
    glBindTexture(GL_TEXTURE_2D, PostProcessingImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(1, PostProcessingImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);

    Raytracer.Init();
    Screen.Init();
    PostProcessing.Init();

    Raytracer.LinkShader("../Shaders/shader.comp", GL_COMPUTE_SHADER);
    PostProcessing.LinkShader("../Shaders/PostProcessing.comp", GL_COMPUTE_SHADER);
    Screen.LinkShader("../Shaders/basic.vs", GL_VERTEX_SHADER);
    Screen.LinkShader("../Shaders/basic.fs", GL_FRAGMENT_SHADER);


    tfov = glm::tan(3.14159 / 8);
    AR = (double)width / (double)heigth;

}