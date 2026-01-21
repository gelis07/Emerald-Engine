#pragma once
#include <core/Camera.h>


class CameraControl
{
    public:
        void Init(float verticalFov, float nearClip, float farClip, int width, int height);
        void OnUpdate(float ts);
        // void OnResize(uint32_t width, uint32_t height);

        const Camera& GetCamera() {return mCamera;}
    private:
        int mWidth, mHeight;
        glm::mat4 CalculateView(const glm::vec3& pos);
        glm::mat4 CalculateProj();
        Camera mCamera;
        glm::vec2 mLastMousePos{0.0f, 0.0f};
        uint32_t mViewportWidth = 0, mViewportHeight = 0;
};