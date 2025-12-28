#include "Renderer.h"
#include "Utils.h"
#include <glm.hpp>
void Renderer::OnUpdate()
{
    if(accumulate)
    {
        frames++;
    }else{
        frames = 1;
    }
    ImGui::Begin("test");
    for(int i = 0; i < scene.hitObjects.size(); i++)
    {
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Sphere: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("position", glm::value_ptr(scene.hitObjects[i].point), 0.01f);
        ImGui::DragFloat3("color", glm::value_ptr(scene.hitObjects[i].mat.Color), 0.01f);
        ImGui::DragFloat("mult", &scene.hitObjects[i].mat.mult, 0.01f);
        ImGui::DragFloat("emmision power", &scene.hitObjects[i].mat.EmmisionPower, 0.01f);
        if(scene.hitObjects[i].type == SPHERE)
        {
            ImGui::DragFloat("radius", &static_cast<HitSphere*>(&scene.hitObjects[i])->radius, 0.01f);
        }
        ImGui::PopID();
    }
    ImGui::Checkbox("accumalate", &accumulate);
    if(ImGui::Button("add"))
    {
        HitSphere newSphere;
        newSphere.radius = 1.0f;
        newSphere.point = glm::vec3(0, 0, 0);
        newSphere.mat.Color = glm::vec3(0, 1, 0);
        newSphere.mat.EmmisionPower = 0.0f;
    }
    ImGui::End();
    glUseProgram(ComputeShaderID);
    glUniform1f(glGetUniformLocation(ComputeShaderID,"AR"), AR);
    glUniform1f(glGetUniformLocation(ComputeShaderID,"tfov"), tfov);
    glUniform3fv(glGetUniformLocation(ComputeShaderID,"CamPos"), 1, glm::value_ptr(camPos));
    glUniform1i(glGetUniformLocation(ComputeShaderID, "FrameIndex"), frames);
    glUniform1i(glGetUniformLocation(ComputeShaderID, "accumalate"), accumulate);

    
    glUniform1i(glGetUniformLocation(ComputeShaderID, "SphereCount"), scene.hitObjects.size() + 1);
    glUniform1i(glGetUniformLocation(ComputeShaderID, "enableEnvironment"), environmentLight);
    for(int i = 0; i < scene.hitObjects.size(); i++)
    {
        std::string indexString = std::to_string(i);
        glUniform3f(glGetUniformLocation(ComputeShaderID,std::string("SPoint[" + indexString + "]").c_str()), scene.hitObjects[i].point.x, scene.hitObjects[i].point.y, scene.hitObjects[i].point.z);
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

}

void Renderer::Init(int width, int heigth)
{
    std::vector<float> QuadVertices = 
    {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

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
    const char* shaderCode = Utils::ReadFile("shader.comp");
    glShaderSource(compute, 1, &shaderCode, NULL);
    glCompileShader(compute);
    Utils::checkCompileErrors(compute, "COMPUTE");
    delete[] shaderCode;

    GLuint VShader, FShader;
    VShader = glCreateShader(GL_VERTEX_SHADER);
    FShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* VShaderCode = Utils::ReadFile("basic.vs");
    const char* FShaderCode = Utils::ReadFile("basic.fs");
    glShaderSource(VShader, 1, &VShaderCode, NULL);
    glShaderSource(FShader, 1, &FShaderCode, NULL);
    glCompileShader(VShader);
    glCompileShader(FShader);
    Utils::checkCompileErrors(VShader, "VERTEX");
    Utils::checkCompileErrors(FShader, "FRAGMENT");
    delete[] VShaderCode;
    delete[] FShaderCode;

    //Raytracing shader
    ComputeShaderID = glCreateProgram();
    glAttachShader(ComputeShaderID, compute);
    glLinkProgram(ComputeShaderID);
    Utils::checkCompileErrors(ComputeShaderID, "PROGRAM");


    //Render quad on screen shader.
    BasicProgram = glCreateProgram();
    glAttachShader(BasicProgram, VShader);
    glAttachShader(BasicProgram, FShader);
    glLinkProgram(BasicProgram);
    Utils::checkCompileErrors(BasicProgram, "PROGRAM");
    tfov = glm::tan(3.14159 / 8);
    AR = (double)width / (double)heigth;

}