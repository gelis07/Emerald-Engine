#include <iostream>
#define FMT_HEADER_ONLY
#define FMT_USE_LOCALE 0
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <cstring>
#include <vector>
#include <glm.hpp>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
 #include <gtc/type_ptr.hpp>
#include "imgui_impl_glfw.h"
#define WWIDTH 1280
#define WHEIGHT 920

char* ReadFile(const std::string& path)
{
    std::ifstream file(path);
    if(!file)
        return nullptr;

    std::string text((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    char* buffer = new char[text.size() + 1];
    memcpy(buffer, text.c_str(), text.size() + 1); // includes '\0'
    return buffer;
}

void checkCompileErrors(GLuint shader, std::string type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n ---------------------------------------------" << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n --------------------------------------------------" << std::endl;
        }
    }
}
void GladErrorCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei lenght, const GLchar* message, const void *userParam)
{
    std::cout << message << '\n';
}


void InitImGui(GLFWwindow* window)
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
    io.ConfigFlags = ImGuiConfigFlags_DockingEnable;
}

int main()
{
    if(!glfwInit())
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "GLFW init error"));
        system("pause");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WWIDTH, WHEIGHT, "Raytracer", NULL, NULL);
    if(!window)
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "Couldn't initialize window"));
        system("pause");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(true);
    gladLoadGL();
    GLint flags;
    glDebugMessageCallback(GladErrorCallBack, NULL);
    
    InitImGui(window);

    std::vector<float> QuadVertices = 
    {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    GLuint VBO, VAO;
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
    GLuint RenderImage;
    glGenTextures(1, &RenderImage);
    glBindTexture(GL_TEXTURE_2D, RenderImage);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT,0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);

    
    GLuint compute;
    compute = glCreateShader(GL_COMPUTE_SHADER);
    const char* shaderCode = ReadFile("shader.comp");
    glShaderSource(compute, 1, &shaderCode, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");
    delete[] shaderCode;

    GLuint VShader, FShader;
    VShader = glCreateShader(GL_VERTEX_SHADER);
    FShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* VShaderCode = ReadFile("basic.vs");
    const char* FShaderCode = ReadFile("basic.fs");
    glShaderSource(VShader, 1, &VShaderCode, NULL);
    glShaderSource(FShader, 1, &FShaderCode, NULL);
    glCompileShader(VShader);
    glCompileShader(FShader);
    checkCompileErrors(VShader, "VERTEX");
    checkCompileErrors(FShader, "FRAGMENT");
    delete[] VShaderCode;
    delete[] FShaderCode;

    //Raytracing shader
    GLuint ComputeShaderID;
    ComputeShaderID = glCreateProgram();
    glAttachShader(ComputeShaderID, compute);
    glLinkProgram(ComputeShaderID);
    checkCompileErrors(ComputeShaderID, "PROGRAM");


    //Render quad on screen shader.
    GLuint BasicProgram;
    BasicProgram = glCreateProgram();
    glAttachShader(BasicProgram, VShader);
    glAttachShader(BasicProgram, FShader);
    glLinkProgram(BasicProgram);
    checkCompileErrors(BasicProgram, "PROGRAM");
    float tfov = glm::tan(3.14159 / 8);
    float AR = (double)WWIDTH / (double)WHEIGHT;

    // glm::vec3 SPoint(0, 0, 5);
    // glm::vec3 SPoint1(3, 0, 5);
    // float radius = 1.0f;
    // glm::vec3 color1(0.0, 0.0, 1.0);
    // glm::vec3 color2(1.0, 1.0, 1.0);
    // float emPower = 0.0f;
    // float emPower1 = 1.0f;
    std::vector<glm::vec3> SPoints = {glm::vec3(0,0,5), glm::vec3(3,0,5)};
    std::vector<float> radius = {1.0f, 1.0f};
    std::vector<glm::vec3> colors = {glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0, 1.0, 1.0)};
    std::vector<float> emPowers = {0.0f, 1.0f};
    std::vector<float> mult = {0.3f, 0.3f};
    bool environmentLight = true;
    bool accumalate = false;
    int frames = 0;
    float angle = 0.0f;
    glm::vec3 camPos(0);
    while(!glfwWindowShouldClose(window))
    {
        if(accumalate)
        {
            frames++;
        }else{
            frames = 1;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Begin("test");
        ImGui::DragFloat("angle", &angle, 0.01f);
        ImGui::DragFloat3("cam pos", glm::value_ptr(camPos), 0.01f);
        for(int i = 0; i < SPoints.size(); i++)
        {
            ImGui::PushID(i);
            ImGui::SeparatorText(std::string("Sphere: " + std::to_string(i)).c_str());
            ImGui::DragFloat3("position", glm::value_ptr(SPoints[i]), 0.01f);
            ImGui::DragFloat3("color", glm::value_ptr(colors[i]), 0.01f);
            ImGui::DragFloat("radius", &radius[i], 0.01f);
            ImGui::DragFloat("mult", &mult[i], 0.01f);
            ImGui::DragFloat("emmision power", &emPowers[i], 0.01f);
            ImGui::PopID();
        }
        ImGui::Checkbox("environment light", &environmentLight);
        ImGui::Checkbox("accumalate", &accumalate);
        if(ImGui::Button("add"))
        {
            SPoints.push_back({0,0,0});
            radius.push_back(1.0f);
            colors.push_back({0.0f, 1.0f, 0.0f});
            emPowers.push_back({0.0f});
        }
        ImGui::End();
        glUseProgram(ComputeShaderID);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"AR"), AR);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"tfov"), tfov);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"angle"), angle);
        glUniform3fv(glGetUniformLocation(ComputeShaderID,"CamPos"), 1, glm::value_ptr(camPos));
        glUniform1i(glGetUniformLocation(ComputeShaderID, "FrameIndex"), frames);
        glUniform1i(glGetUniformLocation(ComputeShaderID, "accumalate"), accumalate);

        
        glUniform1i(glGetUniformLocation(ComputeShaderID, "SphereCount"), SPoints.size() + 1);
        glUniform1i(glGetUniformLocation(ComputeShaderID, "enableEnvironment"), environmentLight);
        for(int i = 0; i < SPoints.size(); i++)
        {
            std::string indexString = std::to_string(i);
            glUniform3f(glGetUniformLocation(ComputeShaderID,std::string("SPoint[" + indexString + "]").c_str()), SPoints[i].x, SPoints[i].y, SPoints[i].z);
            glUniform3f(glGetUniformLocation(ComputeShaderID,std::string("Color[" + indexString + "]").c_str()), colors[i].x, colors[i].y, colors[i].z);
            glUniform1f(glGetUniformLocation(ComputeShaderID,std::string("SRadius[" + indexString + "]").c_str()), radius[i]);
            glUniform1f(glGetUniformLocation(ComputeShaderID,std::string("mult[" + indexString + "]").c_str()), mult[i]);
            glUniform1f(glGetUniformLocation(ComputeShaderID,std::string("EmIntensity[" + indexString + "]").c_str()), emPowers[i]);
        }


        glDispatchCompute((unsigned int)TEXTURE_WIDTH/16, (unsigned int)TEXTURE_HEIGHT/16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


        glBindVertexArray(VAO);
        glUseProgram(BasicProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, RenderImage);
        glUniform1i(glGetUniformLocation(BasicProgram, "RenderImage"), 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}