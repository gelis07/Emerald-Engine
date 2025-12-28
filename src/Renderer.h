#pragma once
#include <glad/glad.h>
#include <vector>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "Scene.h"
#include <gtc/type_ptr.hpp>


class Renderer
{
    public:
        void OnUpdate();
        void Init(int width, int heigth);
    private:
        GLuint VBO, VAO;
        GLuint BasicProgram;
        GLuint ComputeShaderID;
        GLuint RenderImage;
        Scene scene;
        int frames;
        bool accumulate;
        float tfov;
        float AR;
        glm::vec3 camPos = glm::vec3(0,0,0);
};