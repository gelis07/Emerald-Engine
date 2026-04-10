#pragma once
#include <core/Camera.h>


class CameraControl
{
    public:
        void Init(float verticalFov, float nearClip, float farClip);
        void OnUpdate(float ts, int width, int height);
        // void OnResize(uint32_t width, uint32_t height);
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