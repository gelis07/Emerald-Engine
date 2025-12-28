#pragma once
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <iostream>
#include <glad/glad.h>
#define WWIDTH 1280
#define WHEIGHT 920
class Application
{
    public:
        void InitImGui();
        void OnUpdate(); //Every frame.
        void Init();
    private:
        GLFWwindow* window;
};