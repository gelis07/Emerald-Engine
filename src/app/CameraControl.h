#pragma once
#include <core/Camera.h>
#include <GLFW/glfw3.h>


class CameraControl
{
    public:
        void Init(float verticalFov, float nearClip, float farClip);
        void OnUpdate(GLFWwindow* window, float ts, int width, int height);
        void SetSettings(const CameraSettings& settings);
        const Camera& GetCamera() {return mCamera;}
    private:
        int prevWidth, prevHeight;
        glm::mat4 CalculateView(const glm::vec3& pos, const glm::vec3& dir);
        glm::mat4 CalculateProj(int width, int height);
        Camera mCamera;
        glm::vec2 mLastMousePos{0.0f, 0.0f};
        uint32_t mViewportWidth = 0, mViewportHeight = 0;
};