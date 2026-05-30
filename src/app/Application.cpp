#include "Application.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <iostream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stbi_write.h>
#include <chrono>

constexpr inline int spp = 1000; // samples per pixel


void Application::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Base Colors
    ImVec4 bgColor = ImVec4(0.10f, 0.105f, 0.11f, 1.00f);
    ImVec4 lightBgColor = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
    ImVec4 panelColor = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
    ImVec4 panelHoverColor = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    ImVec4 panelActiveColor = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
    ImVec4 textColor = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
    ImVec4 textDisabledColor = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    ImVec4 borderColor = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);

    // Text
    colors[ImGuiCol_Text] = textColor;
    colors[ImGuiCol_TextDisabled] = textDisabledColor;

    // Windows
    colors[ImGuiCol_WindowBg] = bgColor;
    colors[ImGuiCol_ChildBg] = bgColor;
    colors[ImGuiCol_PopupBg] = bgColor;
    colors[ImGuiCol_Border] = borderColor;
    colors[ImGuiCol_BorderShadow] = borderColor;

    // Headers
    colors[ImGuiCol_Header] = panelColor;
    colors[ImGuiCol_HeaderHovered] = panelHoverColor;
    colors[ImGuiCol_HeaderActive] = panelActiveColor;

    // Buttons
    colors[ImGuiCol_Button] = panelColor;
    colors[ImGuiCol_ButtonHovered] = panelHoverColor;
    colors[ImGuiCol_ButtonActive] = panelActiveColor;

    // Frame BG
    colors[ImGuiCol_FrameBg] = lightBgColor;
    colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
    colors[ImGuiCol_FrameBgActive] = panelActiveColor;

    // Tabs
    colors[ImGuiCol_Tab] = panelColor;
    colors[ImGuiCol_TabHovered] = panelHoverColor;
    colors[ImGuiCol_TabActive] = panelActiveColor;
    colors[ImGuiCol_TabUnfocused] = panelColor;
    colors[ImGuiCol_TabUnfocusedActive] = panelHoverColor;

    // Title
    colors[ImGuiCol_TitleBg] = bgColor;
    colors[ImGuiCol_TitleBgActive] = bgColor;
    colors[ImGuiCol_TitleBgCollapsed] = bgColor;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = bgColor;
    colors[ImGuiCol_ScrollbarGrab] = panelColor;
    colors[ImGuiCol_ScrollbarGrabHovered] = panelHoverColor;
    colors[ImGuiCol_ScrollbarGrabActive] = panelActiveColor;

    // Checkmark
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Slider
    colors[ImGuiCol_SliderGrab] = panelHoverColor;
    colors[ImGuiCol_SliderGrabActive] = panelActiveColor;

    // Resize Grip
    colors[ImGuiCol_ResizeGrip] = panelColor;
    colors[ImGuiCol_ResizeGripHovered] = panelHoverColor;
    colors[ImGuiCol_ResizeGripActive] = panelActiveColor;

    // Separator
    colors[ImGuiCol_Separator] = borderColor;
    colors[ImGuiCol_SeparatorHovered] = panelHoverColor;
    colors[ImGuiCol_SeparatorActive] = panelActiveColor;

    // Plot
    colors[ImGuiCol_PlotLines] = textColor;
    colors[ImGuiCol_PlotLinesHovered] = panelActiveColor;
    colors[ImGuiCol_PlotHistogram] = textColor;
    colors[ImGuiCol_PlotHistogramHovered] = panelActiveColor;

    // Text Selected BG
    colors[ImGuiCol_TextSelectedBg] = panelActiveColor;

    // Modal Window Dim Bg
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.105f, 0.11f, 0.5f);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = panelColor;
    colors[ImGuiCol_TableBorderStrong] = borderColor;
    colors[ImGuiCol_TableBorderLight] = borderColor;
    colors[ImGuiCol_TableRowBg] = bgColor;
    colors[ImGuiCol_TableRowBgAlt] = lightBgColor;

    // Styles
    style.FrameBorderSize = 1.0f;
    style.FrameRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabMinSize = 7.0f;
    style.GrabRounding = 2.0f;
    style.TabBorderSize = 1.0f;
    style.TabRounding = 2.0f;

    // Reduced Padding and Spacing
    style.WindowPadding = ImVec2(5.0f, 5.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}
void GladErrorCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei lenght, const GLchar* message, const void *userParam)
{
    std::cout << message << '\n';
}

void Application::Init()
{
    if(!glfwInit())
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "GLFW init error"));
        system("pause");
    }
    //Initialize window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(WWIDTH, WHEIGHT, "Raytracer", NULL, NULL);
    glfwMaximizeWindow(window);
    if(!window)
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "Couldn't initialize window"));
        system("pause");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(true);
    //Initlaize glad
    gladLoadGL();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    GLint flags;
    glDebugMessageCallback(GladErrorCallBack, NULL);
    glEnable(GL_DEPTH_TEST);


    InitImGui();
    camControl.Init(45.0f, 0.1f, 1000.0f);
    gui.loader = &assimpLoader;

    rend.Init(gui.settings, 600, 600);
    rast.Init(gui.settings, 600, 600);

}
void Application::CreateRenderImage(int width, int height)
{
    glGenTextures(1, &RenderImage);
    glBindTexture(GL_TEXTURE_2D, RenderImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);
}

void Application::OnUpdate()
{
    while(!glfwWindowShouldClose(window))
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dt = glfwGetTime() - mLastTime;
        mLastTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);
        camControl.OnUpdate(dt, gui.settings.ImgWidth, gui.settings.ImgHeight);
        
        gui.SceneModifier(dt, {rast.renderTexture}, camControl);
        gui.settings.scene.camera = camControl.GetCamera();
        rast.Update(gui.settings);
        if(gui.settings.Render)
        {
            auto iTime = std::chrono::high_resolution_clock::now();
            fmt::println("started rendering");
            CreateRenderImage(gui.settings.ImgWidth, gui.settings.ImgHeight);
            gui.settings.imageOut = RenderImage;
            gui.settings.spp = spp;
            //Rendering
            rend.Render(gui.settings);

            auto fTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> Dt = fTime - iTime;
            fmt::println("Render time: {} seconds", Dt.count());
            Export();

            gui.settings.Render = false;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
void Application::Export()
{

    //Exporting image
    glBindTexture(GL_TEXTURE_2D, RenderImage);
    std::vector<unsigned char> pixels(gui.settings.ImgWidth * gui.settings.ImgHeight * 4);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<unsigned char> flipped(gui.settings.ImgWidth * gui.settings.ImgHeight * 4);
    for (int y = 0; y < gui.settings.ImgHeight; y++)
    {
        memcpy(
            &flipped[y * gui.settings.ImgWidth * 4],
            &pixels[(gui.settings.ImgHeight - 1 - y) * gui.settings.ImgWidth * 4],
            gui.settings.ImgWidth * 4
        );
    }
    // Write PNG
    stbi_write_png("render.png", gui.settings.ImgWidth, gui.settings.ImgHeight, 4, 
    flipped.data(), gui.settings.ImgWidth * 4);
}