#pragma once
#include <string>
#include <glad/glad.h>
#include <glm.hpp>
class Shader
{
    public:
        Shader(const std::string& path);
        void Bind();
        void RunCompute(unsigned int x,unsigned int y,unsigned int z);
        void Unbind();


        void Uniform1f(const std::string& name, float value);
        void Uniform1i(const std::string& name, int value);
        void Uniform3f(const std::string& name, const glm::vec3& value);
    private:
        GLuint mShaderID;
        GLuint mProgramID;
};  