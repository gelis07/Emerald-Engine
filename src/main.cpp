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





void InitImGui(GLFWwindow* window)
{

}

int main()
{

    
    InitImGui(window);


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