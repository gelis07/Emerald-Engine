#pragma once
#include <core/RenderSettings.h>
#include <app/CameraControl.h>
#include "AssimpLoader.h"
#include <imgui.h>


class GUI
{
    public:
        void SceneModifier(float dt, const std::vector<ImTextureID>& imgs, CameraControl& camControl, LoadSceneInfo info);
        RenderSettings settings;
        AssimpLoader* loader  = nullptr;

        void LoadExternalScene(const ModelConstructData& data, Model& model);

    private:
        void Windows(const char* name, ImTextureID image);
        void SaveSettings(CameraControl& camControl);
        void LoadSettings(const std::string& source, CameraControl& camControl, LoadSceneInfo info);

        std::string loadModelTextbox;
};