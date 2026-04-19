#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>
#include <core/Utils.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void GUI::SceneModifier(float dt, const std::vector<unsigned int>& imgs, CameraControl& camControl)
{
    ImGui::DockSpaceOverViewport();
    Scene& scene = settings.scene;
    Camera& camera = scene.camera;
    ImGui::Begin("Scene modifier");
    ImGui::SeparatorText("Camera Settings");
    CameraSettings stg;
    stg.pos = camera.GetPos();
    ImGui::DragFloat3("Position", glm::value_ptr(stg.pos));
    glm::vec3 dir = camControl.GetCamera().GetDirection();
    ImGui::DragFloat3("Direction", glm::value_ptr(dir));
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

            ImGui::PopID();
        }
    }
    ImGui::SeparatorText("Model Settings");
    for (int i = 0; i < scene.models.size(); i++)
    {
        Model& model = scene.models[i];
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Model: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("Position", glm::value_ptr(model.pos), 0.1f);
        ImGui::DragFloat3("Rotation", glm::value_ptr(model.rotation), 0.1f);
        ImGui::DragFloat3("Scale", glm::value_ptr(model.scale), 0.1f);
        ImGui::DragInt("matIndex", &model.matIndex);

        model.Transform();
        if(ImGui::Button("Delete"))
        {
            scene.models.erase(scene.models.begin() + i);
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
        ImGui::DragFloat3("emmColor", glm::value_ptr(mat->emmColor), 0.01f);
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
    if(ImGui::Button("add cube"))
    {
        Model cube;
        cube.Load(CubeVertices, CubeIndices);
        scene.AddModel(std::move(cube));
    }
    if(ImGui::Button("add mat"))
    {
        Material* mat = new Material;
        mat->albedo = glm::vec3(1.0f);
        mat->fuzz = 0.0f;
        mat->refractionIndex = 0.0f;
        mat->scatter = 1;
        scene.materials.push_back(mat);
    }
    ImGui::Text("delta time is %fms", dt * 1000);
    if(ImGui::Button("Save"))
    {
        SaveSettings(camControl);
    }
    if(ImGui::Button("Load"))
    {
        LoadSettings("scene.json", camControl);
    }


    if(ImGui::Button("Render"))
    {
        settings.Render = true;
    }

    ImGui::End();

    for (int i = 0; i < imgs.size(); i++)
    {
        std::string title = "Viewport #" + std::to_string(i);
        Windows(title.c_str(), imgs[i]);
    }
}


void GUI::Windows(const char* name, unsigned int image)
{
    ImGui::Begin(name);
    ImVec2 size = ImGui::GetWindowSize();
    settings.ImgWidth = size.x;
    settings.ImgHeight = size.y;
    ImGui::Image(image, size, ImVec2(0,1), ImVec2(1,0));

    ImGui::End();
}

void GUI::SaveSettings(CameraControl& camControl)
{
    json data;
    data["models"]["count"] = settings.scene.models.size();
    for (int i = 0; i < settings.scene.models.size(); i++)
    {
        Model& model = settings.scene.models[i];
        json modelJson;

        modelJson["type"] = model.type;
        if(model.type == CUSTOM)
        {
            modelJson["source"] = model.fileSource;
        }

        modelJson["position"]["x"] = model.pos.x; 
        modelJson["position"]["y"] = model.pos.y; 
        modelJson["position"]["z"] = model.pos.z; 

        modelJson["rotation"]["x"] = model.rotation.x; 
        modelJson["rotation"]["y"] = model.rotation.y; 
        modelJson["rotation"]["z"] = model.rotation.z;

        modelJson["scale"]["x"] = model.scale.x; 
        modelJson["scale"]["y"] = model.scale.y; 
        modelJson["scale"]["z"] = model.scale.z;
        
        modelJson["matIndex"] = model.matIndex;
        data["models"][std::to_string(i)] = modelJson;
    }
    data["materials"]["count"] = settings.scene.materials.size();
    for(int i = 0; i < settings.scene.materials.size(); i++)
    {
        Material* mat = settings.scene.materials[i];
        json materialsJson;
        materialsJson["albedo"]["r"] = mat->albedo.r;
        materialsJson["albedo"]["g"] = mat->albedo.g;
        materialsJson["albedo"]["b"] = mat->albedo.b;
        materialsJson["emColor"]["r"] = mat->emmColor.r;
        materialsJson["emColor"]["g"] = mat->emmColor.g;
        materialsJson["emColor"]["b"] = mat->emmColor.b;

        data["materials"][std::to_string(i)] = materialsJson;
    }

    json cameraJson;
    cameraJson["position"]["x"] = camControl.GetCamera().GetPos().x;
    cameraJson["position"]["y"] = camControl.GetCamera().GetPos().y;
    cameraJson["position"]["z"] = camControl.GetCamera().GetPos().z;
    
    cameraJson["direction"]["x"] = camControl.GetCamera().GetDirection().x;
    cameraJson["direction"]["y"] = camControl.GetCamera().GetDirection().y;
    cameraJson["direction"]["z"] = camControl.GetCamera().GetDirection().z;
    
    data["camera"] = cameraJson;
    std::ofstream file("scene.json");
    file << data.dump(4);
    file.close();
}

void GUI::LoadSettings(const std::string& source, CameraControl& camControl)
{
    settings.scene.models.clear();
    for(int i = 0; i < settings.scene.materials.size(); i++)
    {
        delete settings.scene.materials[i];
    }
    settings.scene.materials.clear();


    std::ifstream f(source);
    json data = json::parse(f);
    int modelCount = data["models"]["count"].get<int>();
    for (int i = 0; i < modelCount; i++)
    {
        Model model;
        json modelJson = data["models"][std::to_string(i)];
        model.type = modelJson["type"];
        switch (model.type)
        {
            case(CUSTOM):
            {
                model.Load(modelJson["source"].get<std::string>());
                break;
            }
            case(CUBE):
            {
                model.Load(CubeVertices, CubeIndices);
                break;
            }
        }
        model.pos.x = modelJson["position"]["x"].get<float>();
        model.pos.y = modelJson["position"]["y"].get<float>();
        model.pos.z = modelJson["position"]["z"].get<float>();
        model.rotation.x = modelJson["rotation"]["x"].get<float>();
        model.rotation.y = modelJson["rotation"]["y"].get<float>();
        model.rotation.z = modelJson["rotation"]["z"].get<float>();
        model.scale.x = modelJson["scale"]["x"].get<float>();
        model.scale.y = modelJson["scale"]["y"].get<float>();
        model.scale.z = modelJson["scale"]["z"].get<float>();
        model.Transform();
        model.matIndex = modelJson["matIndex"].get<int>();
        settings.scene.models.push_back(model);
    }

    int matCount = data["materials"]["count"].get<int>();
    for(int i = 0; i < matCount; i++)
    {
        Material* mat = new Material;
        mat->albedo.r = data["materials"][std::to_string(i)]["albedo"]["r"].get<float>();
        mat->albedo.g = data["materials"][std::to_string(i)]["albedo"]["g"].get<float>();
        mat->albedo.b = data["materials"][std::to_string(i)]["albedo"]["b"].get<float>();
        mat->emmColor.r = data["materials"][std::to_string(i)]["emColor"]["r"].get<float>();
        mat->emmColor.g = data["materials"][std::to_string(i)]["emColor"]["g"].get<float>();
        mat->emmColor.b = data["materials"][std::to_string(i)]["emColor"]["b"].get<float>();
        mat->fuzz = 0.0f;
        mat->refractionIndex = 0.0f;
        mat->scatter = 1;

        settings.scene.materials.push_back(mat);
    }

    json cameraJson = data["camera"];
    CameraSettings camSettings;
    camSettings.pos.x = data["camera"]["position"]["x"].get<float>();
    camSettings.pos.y = data["camera"]["position"]["y"].get<float>();
    camSettings.pos.z = data["camera"]["position"]["z"].get<float>();

    camSettings.dir.x = data["camera"]["direction"]["x"].get<float>();
    camSettings.dir.y = data["camera"]["direction"]["y"].get<float>();
    camSettings.dir.z = data["camera"]["direction"]["z"].get<float>();
    camControl.SetSettings(camSettings);
    settings.ReloadScene = true;

}