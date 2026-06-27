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
#include <engine/Rasterizer.h>
#include <app/AssimpLoader.h>
#include <chrono>

#define WWIDTH 1280
#define WHEIGHT 920

class Application
{
    public:
        void OnUpdate(); //Every frame.
        void Init();
    private:
        void Export();
        void InitImGui();
        void CreateRenderImage(int width, int height);
        CameraControl camControl;
        Renderer rend;
        Rasterizer rast;
        AssimpLoader assimpLoader;
        GUI gui;
        GLuint RenderImage;

        GLFWwindow* window;
        double mLastTime = 0.0f;
        double dt = 0.0f;
        std::chrono::time_point<std::chrono::high_resolution_clock> iTime;

};