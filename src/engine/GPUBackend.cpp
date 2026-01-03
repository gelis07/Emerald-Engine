#include "GPUBackend.h"
#include <glm/gtc/type_ptr.hpp>
#include <core/Utils.h>

void Shader::Init()
{
    mProgramID = glCreateProgram();
}

void Shader::LinkShader(const std::string& path, int type)
{
    mShaderID = glCreateShader(type);
    const char* shaderCode = Utils::ReadFile(path.c_str());
    glShaderSource(mShaderID, 1, &shaderCode, NULL);
    glCompileShader(mShaderID);
    Utils::checkCompileErrors(mShaderID, "COMPUTE");
    delete[] shaderCode;

    glAttachShader(mProgramID, mShaderID);
    glLinkProgram(mProgramID);
}


void Shader::Bind()
{
    glUseProgram(mProgramID);
}

void Shader::Unbind()
{
    glUseProgram(0);
}

void Shader::RunCompute(unsigned int x,unsigned int y,unsigned int z)
{
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void Shader::Uniform1f(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(mProgramID,name.c_str()), value);
}
void Shader::Uniform1i(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(mProgramID,name.c_str()), value);
}
void Shader::Uniform3f(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(mProgramID,name.c_str()), 1, glm::value_ptr(value));
}