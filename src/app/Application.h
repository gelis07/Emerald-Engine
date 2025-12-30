#pragma once
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#define FMT_HEADER_ONLY
#define FMT_USE_LOCALE 0
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <engine/Renderer.h>
#include <app/GUI.h>

#define WWIDTH 1280
#define WHEIGHT 920
class Application
{
    public:
        void OnUpdate(); //Every frame.
        void Init();
    private:
        void InitImGui();
        GLFWwindow* window;
        Renderer rend;
        GUI gui;
};