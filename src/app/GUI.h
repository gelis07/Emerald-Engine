#pragma once
#include <core/RenderSettings.h>
#include <app/CameraControl.h>



class GUI
{
    public:
        void SceneModifier(float dt, const std::vector<unsigned int>& imgs, CameraControl& camControl);
        RenderSettings settings;
    private:
        void Windows(const char* name, unsigned int image);

        void SaveSettings(CameraControl& camControl);
        void LoadSettings(const std::string& source, CameraControl& camControl);
};