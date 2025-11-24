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
#define WWIDTH 640
#define WHEIGHT 480

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
    glBindImageTexture(0, RenderImage, 0, GL_FALSE,0 ,GL_READ_ONLY, GL_RGBA32F);
    
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

    glm::vec3 SPoint(0, 0, -5);
    float radius = 1.0f;
    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);


        glUseProgram(ComputeShaderID);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"AR"), AR);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"tfov"), tfov);
        
        glUniform3f(glGetUniformLocation(ComputeShaderID,"SPoint"), SPoint.x, SPoint.y, SPoint.z);
        glUniform1f(glGetUniformLocation(ComputeShaderID,"SRadius"), radius);

        glDispatchCompute((unsigned int)TEXTURE_WIDTH, (unsigned int)TEXTURE_HEIGHT, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindVertexArray(VAO);
        glUseProgram(BasicProgram);
        glUniform1i(glGetUniformLocation(BasicProgram, "RenderImage"), 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}