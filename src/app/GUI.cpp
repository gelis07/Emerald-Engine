#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <core/Utils.h>
#include <nlohmann/json.hpp>
#include <fmt/base.h>
#include <misc/cpp/imgui_stdlib.h>
using json = nlohmann::json;

void GUI::SceneModifier(float dt, const std::vector<ImTextureID>& imgs, CameraControl& camControl, LoadSceneInfo info)
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

    ImGui::SeparatorText("Model Settings");
    for (int i = 0; i < scene.models.size(); i++)
    {
        Model& model = scene.models[i];
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Model: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("Position", glm::value_ptr(model.pos), 0.01f);
        ImGui::DragFloat3("Rotation", glm::value_ptr(model.rotation), 0.1f);
        ImGui::DragFloat3("Scale", glm::value_ptr(model.scale), 0.1f);
        ImGui::DragInt("matIndex", &model.mMeshes[0].matIndex);

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
        ImGui::PushID(i);
        ImGui::SeparatorText(std::string("Material: " + std::to_string(i)).c_str());
        ImGui::DragFloat3("albedo", glm::value_ptr(scene.materials[i].albedo), 0.01f);
        ImGui::DragFloat3("emmColor", glm::value_ptr(scene.materials[i].emmColor), 0.01f);
        ImGui::DragFloat("roughness", &scene.materials[i].roughness, 0.01f, 0.0f, 1.0f);
        if(scene.materials[i].albedoTexture != -1)
            ImGui::Text("I have a texture!");
        ImGui::PopID();
    }
    ImGui::SeparatorText("Environment Settings");
    ImGui::Checkbox("EnvLight", &settings.EnvLight);
    if(ImGui::Button("add cube"))
    {
        Model cube;
        ModelConstructData data;
        Mesh mesh;
        mesh.vertices = CubeVertices;
        mesh.indices = CubeIndices;
        data.meshes.push_back(mesh);
        #ifdef OPENGL
            mesh.texture.id = -1;
        #endif
        cube.Load(data);
        scene.AddModel(std::move(cube));
    }
    if(ImGui::Button("add mat"))
    {
        Material mat;
        mat.albedo = glm::vec3(1.0f);
        mat.albedo = glm::vec3(0.0f);
        scene.materials.push_back(mat);
    }

    ImGui::InputText("input model", &loadModelTextbox);
    if(ImGui::Button("Load Model"))
    {
        Model model;
        ModelConstructData data = loader->LoadModel(loadModelTextbox, info);
        LoadExternalScene(data, model);
        model.pos = glm::vec3(0.0);
        model.rotation = glm::vec3(0.0);
        model.scale = glm::vec3(1.0);

        scene.AddModel(std::move(model));
    }

    ImGui::Text("delta time is %fms", dt * 1000);
    if(ImGui::Button("Save"))
    {
        SaveSettings(camControl);
    }
    if(ImGui::Button("Load"))
    {
        LoadSettings("scene.json", camControl, info);
    }


    if(ImGui::Button("Render"))
    {
        settings.Render = true;
    }
    ImGui::Text("%i", settings.frameIdx);
    ImGui::End();

    for (int i = 0; i < imgs.size(); i++)
    {
        std::string title = "Viewport #" + std::to_string(i);
        Windows(title.c_str(), imgs[i]);
    }
}


void GUI::Windows(const char* name, ImTextureID image)
{
    ImGui::Begin(name);
    ImVec2 size = ImGui::GetWindowSize();
    settings.ImgWidth = RTXimgWidth;
    settings.ImgHeight = RTXimgHeight;

    float ar = (float)settings.ImgWidth / (float)settings.ImgHeight;
    ImVec2 avail_size = ImGui::GetContentRegionAvail();

    ImVec2 display_size = avail_size;
    if (avail_size.x / avail_size.y > ar) {
        display_size.x = avail_size.y * ar;
    } else {
        display_size.y = avail_size.x / ar;
    }
    ImGui::Image(image, display_size, ImVec2(0,1), ImVec2(1,0));

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
        modelJson["meshCount"] = model.mMeshes.size();
        for(int j = 0; j < model.mMeshes.size(); j++)
        {
            json meshJson;
            meshJson["matIdx"] = model.mMeshes[j].matIndex;
            modelJson["meshes"][std::to_string(j)] = meshJson;
        }
        data["models"][std::to_string(i)] = modelJson;
    }
    data["materials"]["count"] = settings.scene.materials.size();
    for(int i = 0; i < settings.scene.materials.size(); i++)
    {
        Material mat = settings.scene.materials[i];
        json materialsJson;
        materialsJson["albedo"]["r"] = mat.albedo.r;
        materialsJson["albedo"]["g"] = mat.albedo.g;
        materialsJson["albedo"]["b"] = mat.albedo.b;
        materialsJson["emColor"]["r"] = mat.emmColor.r;
        materialsJson["emColor"]["g"] = mat.emmColor.g;
        materialsJson["emColor"]["b"] = mat.emmColor.b;
        materialsJson["albedoMap"] = mat.albedoTexture;

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

void GUI::LoadSettings(const std::string& source, CameraControl& camControl, LoadSceneInfo info)
{
    settings.scene.models.clear();
    settings.scene.materials.clear();


    std::ifstream f(source);
    json data = json::parse(f);
    
    int matCount = data["materials"]["count"].get<int>();
    for(int i = 0; i < matCount; i++)
    {
        Material mat;
        mat.albedo.r = data["materials"][std::to_string(i)]["albedo"]["r"].get<float>();
        mat.albedo.g = data["materials"][std::to_string(i)]["albedo"]["g"].get<float>();
        mat.albedo.b = data["materials"][std::to_string(i)]["albedo"]["b"].get<float>();
        mat.emmColor.r = data["materials"][std::to_string(i)]["emColor"]["r"].get<float>();
        mat.emmColor.g = data["materials"][std::to_string(i)]["emColor"]["g"].get<float>();
        mat.emmColor.b = data["materials"][std::to_string(i)]["emColor"]["b"].get<float>();
        mat.albedoTexture = data["materials"][std::to_string(i)]["albedoMap"].get<uint32_t>();

        settings.scene.materials.push_back(mat);
    }
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
                if(loader != nullptr)
                {
                    ModelConstructData data = loader->LoadModel(modelJson["source"].get<std::string>(), info);
                    LoadExternalScene(data, model);
                }else{
                    fmt::println("Loader on GUI Class is not defined!");
                }
                break;
            }
            case(CUBE):
            {
                ModelConstructData data;
                Mesh mesh;
                mesh.vertices = CubeVertices;
                mesh.indices = CubeIndices;
                data.meshes.push_back(mesh);
                model.Load(data);
                break;
            }
        }
        for(int j = 0; j < model.mMeshes.size(); j++)
        {
            model.mMeshes[j].matIndex = modelJson["meshes"][std::to_string(j)]["matIdx"];
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
        settings.scene.models.push_back(model);
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



void GUI::LoadExternalScene(const ModelConstructData& data, Model& model)
{
    model.Load(data);
    settings.scene.textures.insert(settings.scene.textures.end(), data.textureData.begin(), data.textureData.end());
    settings.scene.materials.insert(settings.scene.materials.end(), data.materials.begin(), data.materials.end());

    for(int i = 0; i < model.mMeshes.size(); i++)
    {
        model.mMeshes[i].matIndex += settings.scene.materials.size() - 1;
    }
}