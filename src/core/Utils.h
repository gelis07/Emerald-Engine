#pragma once
#include <fstream>
#include <cstring>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>


namespace Utils
{
    inline char* ReadFile(const std::string& path)
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
    inline glm::vec3 RandomVec3()
    {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        return glm::vec3(r,g,b);
    }
    inline void checkCompileErrors(GLuint shader, std::string type)
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
    
}

inline std::vector<unsigned int> CubeIndices {
    //Top
    2, 6, 7,
    2, 3, 7,

    //Bottom
    0, 4, 5,
    0, 1, 5,

    //Left
    0, 2, 6,
    0, 4, 6,

    //Right
    1, 3, 7,
    1, 5, 7,

    //Front
    0, 2, 3,
    0, 1, 3,

    //Back
    4, 6, 7,
    4, 5, 7
};


inline std::vector<float> CubeVertices {
    -1, -1,  1, //0
        1, -1,  1, //1
    -1,  1,  1, //2
        1,  1,  1, //3
    -1, -1, -1, //4
        1, -1, -1, //5
    -1,  1, -1, //6
        1,  1, -1  //7
};
inline std::vector<glm::vec3> CubeVerticesVec3 {
    glm::vec3(-1, -1,  1), //0
        glm::vec3(1, -1,  1), //1
    glm::vec3(-1,  1,  1), //2
        glm::vec3(1,  1,  1), //3
    glm::vec3(-1, -1, -1), //4
        glm::vec3(1, -1, -1), //5
    glm::vec3(-1,  1, -1), //6
        glm::vec3(1,  1, -1)  //7
};