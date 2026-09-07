#pragma once
#include <core/RenderSettings.h>
#include <app/CameraControl.h>
#include "AssimpLoader.h"
#include <imgui.h>
#include <ImGuizmo/src/ImGuizmo.h>



enum class RenderMode
{
    Rasterizer,
    PathTracing
};
inline const char* modes[] = {
    "Rasterization",
    "Path Tracing",
};

class GUI
{
    public:
        void SceneModifier(float dt, const std::vector<ImTextureID>& imgs, CameraControl& camControl, LoadSceneInfo info);
        RenderSettings settings;
        AssimpLoader* loader  = nullptr;

        void LoadExternalScene(const ModelConstructData& data, Model& model);
        RenderMode renderMode = RenderMode::PathTracing;

        GLFWwindow* window;
    private:
        void Windows(const char* name, ImTextureID image);
        void SaveSettings(CameraControl& camControl);
        void LoadSettings(const std::string& source, CameraControl& camControl, LoadSceneInfo info);
        void Gizmo(bool focus);

        void TexturesWindow();
        void MaterialDataWindow();
        void MeshDataWindow(uint32_t objId, uint32_t meshId);
        void ModelDataWindow(uint32_t obj);
        void NodeDataWindow(uint32_t objId, uint32_t nodeId);

        ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
        
        uint32_t SelectedModelId;
        uint32_t SelectedNodeId;
        uint32_t SelectedMeshId;

        ImVec2 RendWindowSize;
        ImVec2 RendWindowPos;




        std::string loadModelTextbox;
        std::string loadTextureTextbox;
};