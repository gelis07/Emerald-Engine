#include "Rasterizer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fmt/base.h>


void Rasterizer::Init(Scene* scene, int width, int height)
{
    activeScene = scene;

    rastModel.model = &activeScene->models[0];
    
    glCreateBuffers(1, &rastModel.vbo);
    glNamedBufferStorage(rastModel.vbo, sizeof(float) * rastModel.model->mVertices.size(), rastModel.model->mVertices.data(), 0);
    glCreateBuffers(1, &rastModel.ibo);
    glNamedBufferStorage(rastModel.ibo, sizeof(unsigned int) * rastModel.model->mTriangles.size(), rastModel.model->mTriangles.data(), 0);

    glCreateVertexArrays(1, &rastModel.vao);
    glBindVertexArray(rastModel.vao);
    glBindBuffer(GL_ARRAY_BUFFER, rastModel.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rastModel.ibo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);



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

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        fmt::println("error with framebuffers");

    RastShader.Init();
    RastShader.LinkShader("../Shaders/rasterizer.vs", GL_VERTEX_SHADER);
    RastShader.LinkShader("../Shaders/rasterizer.fs", GL_FRAGMENT_SHADER);
}


void Rasterizer::Update(const RenderSettings& rs)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, rs.ImgWidth, rs.ImgHeight);
    
    RastShader.Bind();
    glm::mat4 model(1.0f);
    model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
    glm::mat4 mvp = activeScene->camera.GetProjection() * activeScene->camera.GetView();
    RastShader.UniformMat4("uMvp", mvp);
    RastShader.Uniform4f("uColor", glm::vec4(1.0f));
    
    glBindVertexArray(rastModel.vao);
    glDrawElements(GL_TRIANGLES, rastModel.model->mTriangles.size(), GL_UNSIGNED_INT, (void*)0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}