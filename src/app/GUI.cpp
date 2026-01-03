#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>


void GUI::SceneModifier()
{

    Scene& scene = settings.scene;

    ImGui::Begin("test");
    for(int i = 0; i < scene.hitObjects.size(); i++)
    {
        Hittable* hitObj = scene.hitObjects[i];
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Sphere: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("position", glm::value_ptr(hitObj->point), 0.01f);
        ImGui::DragFloat3("color", glm::value_ptr(hitObj->mat.Color), 0.01f);
        ImGui::DragFloat("mult", &hitObj->mat.mult, 0.01f);
        ImGui::DragFloat("emmision power", &hitObj->mat.EmmisionPower, 0.01f);
        if(scene.hitObjects[i]->type == SPHERE)
        {
            ImGui::DragFloat("radius", &static_cast<HitSphere*>(hitObj)->radius, 0.01f);
        }
        ImGui::PopID();
    }
    ImGui::Checkbox("accumulate", &settings.accumulate);
    ImGui::Checkbox("EnvLight", &settings.EnvLight);
    if(ImGui::Button("add"))
    {
        HitSphere* newSphere = new HitSphere();
        newSphere->radius = 1.0f;
        newSphere->point = glm::vec3(0, 0, 0);
        newSphere->mat.Color = glm::vec3(0, 1, 0);
        newSphere->mat.EmmisionPower = 0.0f;
        scene.hitObjects.push_back(newSphere);
    }
    ImGui::End();
}