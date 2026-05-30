#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
class Shader
{
    public:
        void Init();
        void LinkShader(const std::string& path, int type);
        void Bind();
        void RunCompute(unsigned int x,unsigned int y,unsigned int z);
        void Unbind();


        void Uniform1f(const std::string& name, float value);
        void Uniform1i(const std::string& name, int value);
        void Uniform3f(const std::string& name, const glm::vec3& value);
        void Uniform4f(const std::string& name, const glm::vec4& value);
        void UniformMat4(const std::string& name, const glm::mat4& mat);
        GLuint mProgramID;
    private:
        GLuint mShaderID;
};  