#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>


void GUI::SceneModifier()
{

    Scene& scene = settings.scene;
    Camera& camera = scene.camera;
    ImGui::Begin("Scene modifier");
    ImGui::SeparatorText("Camera Settings");
    CameraSettings stg;
    stg.pos = camera.GetPos();
    ImGui::DragFloat3("Position", glm::value_ptr(stg.pos));
    camera.Set(stg);
    ImGui::SeparatorText("Object Settings");
    for(int i = 0; i < scene.hitObjects.size(); i++)
    {
        Hittable* hitObj = scene.hitObjects[i];
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Sphere: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("position", glm::value_ptr(hitObj->point), 0.01f);
        ImGui::DragFloat3("albedo", glm::value_ptr(hitObj->mat.albedo), 0.01f);
        ImGui::DragInt("material Index", &hitObj->mat.scatter, 0.01f);
        ImGui::DragFloat("material fuzz", &hitObj->mat.fuzz, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("material refraction index", &hitObj->mat.refractionIndex, 0.01f, 0.0f);
        if(scene.hitObjects[i]->type == SPHERE)
        {
            ImGui::DragFloat("radius", &static_cast<HitSphere*>(hitObj)->radius, 0.01f);
        }
        ImGui::PopID();
    }
    ImGui::SeparatorText("Environment Settings");
    ImGui::Checkbox("accumulate", &settings.accumulate);
    ImGui::Checkbox("EnvLight", &settings.EnvLight);
    if(ImGui::Button("add"))
    {
        HitSphere* newSphere = new HitSphere();
        newSphere->radius = 1.0f;
        newSphere->point = glm::vec3(0, 0, 0);
        newSphere->mat.albedo = glm::vec3(0, 1, 0);
        newSphere->mat.scatter = 1;
        newSphere->mat.fuzz = 0.0f;
        scene.hitObjects.push_back(newSphere);
    }
    ImGui::End();
}