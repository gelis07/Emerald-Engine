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
#include <app/CameraControl.h>
#include <core/Model.h>
#include <engine/Rasterizer.h>

#define WWIDTH 1280
#define WHEIGHT 920

class Application
{
    public:
        void OnUpdate(); //Every frame.
        void Init();
    private:
        CameraControl camControl;
        Shader postProcessing;
        void InitImGui();
        GLFWwindow* window;
        Renderer rend;
        Rasterizer rast;
        GUI gui;
        double mLastTime = 0.0f;
        double dt = 0.0f;
};