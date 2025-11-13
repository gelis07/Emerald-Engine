#include <iostream>
#define FMT_HEADER_ONLY
#define FMT_USE_LOCALE 0
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


void checkCompileErrors(GLuint shader, std::string type)
{
    GLint success;
    GLchar infoLog[1024];;
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

int main()
{
    if(!glfwInit())
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "GLFW init error"));
        system("pause");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Raytracer", NULL, NULL);
    if(!window)
    {
        fmt::println("{}", fmt::format(fg(fmt::rgb(0xFF0000)), "Couldn't initialize window"));
        system("pause");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(true);

    GLuint compute;
    compute = glCreateShader(GL_COMPUTE_SHADER);
    const char* shaderCode;
    glShaderSource(compute, 1, &shaderCode, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");
    
    GLuint id;
    id = glCreateProgram();
    glAttachShader(id, compute);
    glLinkProgram(id);
    checkCompileErrors(compute, "PROGRAM");
     
    while(!glfwWindowShouldClose(window))
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}