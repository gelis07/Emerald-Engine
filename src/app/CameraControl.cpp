#include "CameraControl.h"
#include <fmt/base.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


void CameraControl::Init(float verticalFov, float nearClip, float farClip)
{
    CameraSettings stg;
    stg.dir = glm::vec3(0, 0, 1);
    stg.pos = glm::vec3(0, 0, 0);
    mCamera.Set(stg);
}

void CameraControl::OnUpdate(GLFWwindow* window, float ts, int width, int height)
{
    CameraSettings stg;
    stg.pos = mCamera.GetPos();
    stg.dir = mCamera.GetDirection();
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 mousePos = glm::vec2(x,y);
    glm::vec2 delta = (mousePos - mLastMousePos) * 0.002f;
    mLastMousePos = mousePos;
    bool moved = false;
    if(!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
    {
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        constexpr glm::vec3 upDir = glm::vec3(0,1,0);
        glm::vec3 rightDir = glm::cross(mCamera.GetDirection(), upDir);
        float speed = 5.0f;


        if(glfwGetKey(window, GLFW_KEY_W))
        {
            stg.pos += mCamera.GetDirection() * ts * speed;
            moved = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_S))
        {
            stg.pos -= mCamera.GetDirection() * ts * speed;
            moved = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_D))
        {
            stg.pos += rightDir * ts * speed;
            moved = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_A))
        {
            stg.pos -= rightDir * ts * speed;
            moved = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_Q))
        {
            stg.pos -= upDir * ts * speed;
            moved = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_E))
        {
            stg.pos += upDir * ts * speed;
            moved = true;
        }

        constexpr float rotSpeed = 2.0f; 
        if(delta.x != 0.0f || delta.y != 0.0f)
        {
            float pitchDelta = delta.y * rotSpeed;
            float yawDelta = delta.x * rotSpeed;
            
            glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta,rightDir),
            glm::angleAxis(-yawDelta, glm::vec3(0, 1, 0))));

            stg.dir = glm::rotate(q, mCamera.GetDirection());
            moved =true;
        }
    }



    if(moved)
    {
        stg.view = CalculateView(stg.pos, mCamera.GetDirection());
    }
    if(prevHeight != height || prevWidth != width)
    {
        stg.projection = CalculateProj(width, height);
        prevWidth = width;
        prevHeight = height;
    }

    mCamera.Set(stg);
    mCamera.moved = moved;
}

glm::mat4 CameraControl::CalculateView(const glm::vec3& pos, const glm::vec3& dir)
{
    glm::mat4 view = glm::lookAt(pos, pos + dir, glm::vec3(0,1,0));
    return view;
}

glm::mat4 CameraControl::CalculateProj(int width, int height)
{
    glm::mat4 proj = glm::perspectiveFov(glm::radians(mCamera.GetFov()), (float)width, 
    (float)height, mCamera.GetNearClip(), mCamera.GetFarClip());
    return proj;
}

void CameraControl::SetSettings(const CameraSettings& settings)
{
    CameraSettings stg = settings;
    stg.view = CalculateView(settings.pos, settings.dir);
    mCamera.Set(stg);
}
