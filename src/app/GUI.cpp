#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>


void GUI::SceneModifier(float dt)
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
        ImGui::DragInt("mat Index", &hitObj->matIndex);
        if(scene.hitObjects[i]->type == SPHERE)
        {
            ImGui::DragFloat3("position", glm::value_ptr(static_cast<HitSphere*>(hitObj)->point), 0.01f);
            ImGui::DragFloat("radius", &static_cast<HitSphere*>(hitObj)->radius, 0.01f);
        }
        if(scene.hitObjects[i]->type == TRIANGLE)
        {
            HitTriangle* tri = static_cast<HitTriangle*>(hitObj);
            ImGui::DragFloat3("a", glm::value_ptr(tri->a), 0.01f);
            ImGui::DragFloat3("b", glm::value_ptr(tri->b), 0.01f);
            ImGui::DragFloat3("c", glm::value_ptr(tri->c), 0.01f);
        }
        ImGui::PopID();
    }
    ImGui::SeparatorText("Material Settings");
    for(int i = 0; i < scene.materials.size(); i++)
    {
        Material* mat = scene.materials[i];
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Material: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("albedo", glm::value_ptr(mat->albedo), 0.01f);
        ImGui::DragInt("material Index", &mat->scatter, 0.01f);
        ImGui::DragFloat("material fuzz", &mat->fuzz, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("material refraction index", &mat->refractionIndex, 0.01f, 0.0f);
        ImGui::PopID();
    }
    ImGui::SeparatorText("Environment Settings");
    ImGui::Checkbox("accumulate", &settings.accumulate);
    ImGui::Checkbox("EnvLight", &settings.EnvLight);
    if(ImGui::Button("add sphere"))
    {
        HitSphere* newSphere = new HitSphere();
        newSphere->radius = 1.0f;
        newSphere->point = glm::vec3(0, 0, 0);
        newSphere->matIndex = 0;
        scene.hitObjects.push_back(newSphere);
    }
    // if(ImGui::Button("add triangle"))
    // {
    //     HitTriangle* newTriangle = new HitTriangle();
    //     newTriangle->a = glm::vec3(1.0f);
    //     newTriangle->b = glm::vec3(0.0f);
    //     newTriangle->c = glm::vec3(2.0f);
    //     newTriangle->matIndex = 0;
    //     scene.hitObjects.push_back(newTriangle);
    // }
    if(ImGui::Button("add mat"))
    {
        Material* mat = new Material;
        mat->albedo = glm::vec3(1.0f);
        mat->fuzz = 0.0f;
        mat->refractionIndex = 0.0f;
        mat->scatter = 1;
        scene.materials.push_back(mat);
    }

    float fps = 1 / dt;
    ImGui::Text("dt: %.3f", fps);
    ImGui::End();
}