#include "Rasterizer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/base.h>
#include <core/Utils.h>

void Rasterizer::Init(RenderSettings& rs, int width, int height)
{
    AddModels(rs);
    rastModels.reserve(100);

    glBindVertexArray(0);

    glCreateBuffers(1, &cubeVbo);
    glNamedBufferStorage(cubeVbo, sizeof(float) * CubeVertices.size(), CubeVertices.data(), 0);
    glCreateBuffers(1, &cubeIbo);
    glNamedBufferStorage(cubeIbo, sizeof(unsigned int) * CubeIndices.size(), CubeIndices.data(), 0);

    glCreateVertexArrays(1, &cubeVao);
    glBindVertexArray(cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);

    CreateTexture(width, height);


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        fmt::println("error with framebuffers");

    RastShader.Init();
    RastShader.LinkShader("../Shaders/rasterizer.vs", GL_VERTEX_SHADER);
    RastShader.LinkShader("../Shaders/rasterizer.fs", GL_FRAGMENT_SHADER);
}
void Rasterizer::CreateTexture(int width, int height)
{
    glDeleteTextures(1, &renderTexture);

    glCreateTextures(GL_TEXTURE_2D, 1, &renderTexture);
    glTextureStorage2D(renderTexture, 1, GL_RGBA8, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width ,height,0,GL_RGBA,GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glCreateFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);

    glCreateRenderbuffers(1, &renderBuffer);
    glNamedRenderbufferStorage(renderBuffer, GL_DEPTH24_STENCIL8, width, height);
    glNamedFramebufferRenderbuffer(frameBuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
}
void Rasterizer::AddModels(RenderSettings& rs)
{
    for (int i = 0; i < rastModels.size(); i++)
    {
        glDeleteBuffers(1, &rastModels[i].vbo);
        glDeleteBuffers(1, &rastModels[i].ibo);
        glDeleteVertexArrays(1, &rastModels[i].vao);
    }
    rastModels.clear();

    for (int i = 0; i < rs.scene.models.size(); i++)
    {
        RasterizedModel rastModel;
        rastModel.model = &rs.scene.models[i];
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices; 

        for(int i = 0; i < rastModel.model->mMeshes.size(); i++)
        {
            rastModel.indicesCount += rastModel.model->mMeshes[i].indices.size();
            for(int j = 0; j < rastModel.model->mMeshes[i].vertices.size(); j++)
            {
                vertices.push_back(rastModel.model->mMeshes[i].vertices[j]);
            }
            for(int j = 0; j < rastModel.model->mMeshes[i].indices.size(); j++)
            {
                indices.push_back(rastModel.model->mMeshes[i].indices[j]);
            }
        }


        glCreateBuffers(1, &rastModel.vbo);
        glNamedBufferStorage(rastModel.vbo, sizeof(Vertex) * vertices.size(),vertices.data(), 0);
        glCreateBuffers(1, &rastModel.ibo);
        glNamedBufferStorage(rastModel.ibo, sizeof(unsigned int) * indices.size(), indices.data(), 0);

        glCreateVertexArrays(1, &rastModel.vao);
        glBindVertexArray(rastModel.vao);
        glBindBuffer(GL_ARRAY_BUFFER, rastModel.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rastModel.ibo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float)));

        rastModels.push_back(rastModel);
    }

    lastModelCount = rs.scene.models.size();
}


void Rasterizer::Update(RenderSettings& rs)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, rs.ImgWidth, rs.ImgHeight);
    

    if(rs.scene.models.size() != lastModelCount)
    {
        AddModels(rs);
    }
    if(prevWindowWidth != rs.ImgWidth || prevWindowHeight != rs.ImgHeight)
    {
        CreateTexture(rs.ImgWidth, rs.ImgHeight);
        prevWindowHeight = rs.ImgHeight;
        prevWindowWidth = rs.ImgWidth;
    }

    for (int i = 0; i < rastModels.size(); i++)
    {
        RasterizedModel& rastModel = rastModels[i];

        RastShader.Bind();
        glm::mat4 model(1.0f);
        glm::mat4 mvp = rs.scene.camera.GetProjection() * rs.scene.camera.GetView() * rastModel.model->model;
        RastShader.UniformMat4("uMvp", mvp);
        RastShader.Uniform4f("uColor", glm::vec4(1.0f));
        RastShader.Uniform1i("randomColor", 1);
        
        glBindVertexArray(rastModel.vao);
        glDrawElements(GL_TRIANGLES, rastModel.indicesCount, GL_UNSIGNED_INT, (void*)0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}