#pragma once
#include <core/RenderSettings.h>
#include <app/CameraControl.h>
#include "AssimpLoader.h"
#include <imgui.h>
#include <ImGuizmo/src/ImGuizmo.h>
#include "GLFW/glfw3.h"
#include <nlohmann/json.hpp>
#include <core/Animator.h>
using json = nlohmann::json;


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

        void Callbacks();
        GLFWwindow* window;
        Animator animator;
    private:

        void Timeline();

        void Windows(const char* name, ImTextureID image);
        void SaveSettings(const std::string& path, CameraControl& camControl);
        void LoadSettings(const std::string& source, CameraControl& camControl, LoadSceneInfo info);
        void Gizmo(bool focus);

        void TexturesWindow();
        void MaterialDataWindow();
        void MeshDataWindow(uint32_t objId, uint32_t meshId);
        void ModelDataWindow(uint32_t obj);
        void NodeDataWindow(uint32_t objId, uint32_t nodeId);
        void RenderSettingsWindow(LoadSceneInfo info, CameraControl& camControl, float dt);
        void TextureDropdown(uint32_t& id, const std::string& name, const std::vector<const char*> options);
        void MainMenuBar(CameraControl& camControl, LoadSceneInfo info);
        void SaveVec3(json& json, const std::string& name, glm::vec3& vec);
        void LoadVec3(json& json, const std::string& name, glm::vec3& vec);
        ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
        
        uint32_t SelectedModelId;
        uint32_t SelectedNodeId;
        uint32_t SelectedMatId;
        uint32_t SelectedMeshId = -1;


        float timelineTime = 0.0f;
        float maxTime = 10.0f;

        ImVec2 RendWindowSize;
        ImVec2 RendWindowPos;
};