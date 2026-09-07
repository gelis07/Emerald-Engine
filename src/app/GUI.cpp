#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <core/Utils.h>
#include <nlohmann/json.hpp>
#include <fmt/base.h>
#include <misc/cpp/imgui_stdlib.h>
#include "GLFW/glfw3.h"
#include <vkEngine/VkEngineApiBinding.h>

using json = nlohmann::json;

void GUI::SceneModifier(float dt, const std::vector<ImTextureID>& imgs, CameraControl& camControl, LoadSceneInfo info)
{
    ImGui::DockSpaceOverViewport();
    Scene& scene = settings.scene;
    Camera& camera = scene.camera;

    for (int i = 0; i < imgs.size(); i++)
    {
        std::string title = "Viewport #" + std::to_string(i);
        Windows(title.c_str(), imgs[i]);
    }


    ImGui::Begin("Scene modifier");
    ImGui::SeparatorText("Camera Settings");
    CameraSettings stg;
    stg.pos = camera.GetPos();
    ImGui::DragFloat3("Position", glm::value_ptr(stg.pos));
    glm::vec3 dir = camControl.GetCamera().GetDirection();
    ImGui::DragFloat3("Direction", glm::value_ptr(dir));
    camera.Set(stg);

    ImGui::SeparatorText("Models");
    ImGuiTreeNodeFlags treeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags leafFlags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_Leaf;
    for (int i = 0; i < scene.models.size(); i++)
    {
        ImGuiTreeNodeFlags flags = treeFlags;
        if (SelectedModelId == i)
            flags |= ImGuiTreeNodeFlags_Selected;
        bool open = ImGui::TreeNodeEx(
            std::string("Model: " + std::to_string(i)).c_str(),
            flags
        );
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SelectedModelId = i;
            SelectedNodeId = -1;
            SelectedMeshId = -1;
        }
        if (open)
        {
            for (int j = 0; j < scene.models[i].meshes.size(); j++)
            {
                ImGuiTreeNodeFlags nLeafFlags = leafFlags;
                if(j == SelectedNodeId)
                    nLeafFlags |= ImGuiTreeNodeFlags_Selected;
                ImGui::TreeNodeEx(
                    scene.models[i].nodeData[j].name.c_str(),
                    nLeafFlags
                );
                if (ImGui::IsItemClicked())
                {
                    SelectedMeshId = j;
                    SelectedNodeId = scene.models[i].meshes[j].nodeId;
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
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
        mesh.matIndex = 0;
        mesh.nodeId = 0;
        data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
        data.meshes.push_back(mesh);
        cube.Load(data);
        cube.type = CUBE;
        scene.AddModel(std::move(cube));
    }
    if(ImGui::Button("add plane"))
    {
        Model plane;
        ModelConstructData data;
        Mesh mesh;
        mesh.vertices = PlaneVertices;
        mesh.indices = PlaneIndices;
        mesh.matIndex = 0;
        mesh.nodeId = 0;
        data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
        data.meshes.push_back(mesh);
        plane.Load(data);
        plane.type = PLANE;
        scene.AddModel(std::move(plane));
    }
    if(ImGui::Button("add sphere"))
    {
        Model plane;
        ModelConstructData data;
        Mesh mesh;
        mesh.vertices = SphereVertices;
        mesh.indices = SphereIndices;
        mesh.matIndex = 0;
        mesh.nodeId = 0;
        data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
        data.meshes.push_back(mesh);
        plane.Load(data);
        plane.type = SPHERE;
        scene.AddModel(std::move(plane));
    }
    if(ImGui::Button("add mat"))
    {
        Material mat;
        mat.albedo = glm::vec3(1.0f);
        mat.albedo = glm::vec3(0.0f);
        scene.materials.push_back(mat);
    }
    ImGui::InputText("input texture", &loadTextureTextbox);
    if(ImGui::Button("add texture"))
    {
        TextureVk newTexture;
        int width, height, channels;
        unsigned char* texData = stbi_load(loadTextureTextbox.c_str(), &width, &height, &channels, 4);
        VkImageCreateData imgCreateData;
        imgCreateData.allocator = info.alloc;
        imgCreateData.commandPool = info.cPool;
        imgCreateData.device = info.device;
        imgCreateData.queue = info.queue;
        imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
        newTexture = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
        newTexture.path = loadTextureTextbox;
        settings.scene.textures.push_back(newTexture);
        stbi_image_free(texData);
    }

    ImGui::InputText("input model", &loadModelTextbox);
    if(ImGui::Button("Load Model"))
    {
        Model model;
        ModelConstructData data = loader->LoadModel(loadModelTextbox, info);
        LoadExternalScene(data, model);
        model.type = CUSTOM;
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
        SelectedModelId = 0;
        SelectedMeshId = -1;
        SelectedNodeId = -1;
    }
    int selectedMode = static_cast<int>(renderMode);

    if (ImGui::Combo(
            "Render Mode",
            &selectedMode,
            modes,
            IM_ARRAYSIZE(modes)))
    {
        renderMode = static_cast<RenderMode>(selectedMode);
    }

    ImGui::Checkbox("Play animation", &settings.playAnimation);
    if(ImGui::Button("Reload Scene"))
    {
        settings.ReloadScene = true;
    }
    if(ImGui::Button("Render"))
    {
        settings.Render = true;
    }
    ImGui::Text("%i", settings.frameIdx);
    ImGui::End();
    
    TexturesWindow();
    MeshDataWindow(SelectedModelId, SelectedMeshId);
    MaterialDataWindow();
    ModelDataWindow(SelectedModelId);
    NodeDataWindow(SelectedModelId, SelectedNodeId);
}


void GUI::MeshDataWindow(uint32_t objId, uint32_t meshId)
{
    if(meshId == -1)
        return;

    ImGui::Begin("Mesh Window");
    ImGui::DragInt("Mat id", reinterpret_cast<int*>(&settings.scene.models[objId].meshes[meshId].matIndex));
    ImGui::End();
}

void GUI::MaterialDataWindow()
{   
    ImGui::Begin("Materials");
    for(int i = 0; i < settings.scene.materials.size(); i++)
    {
        ImGui::PushID(i);
        ImGui::SeparatorText("Material");
        ImGui::DragFloat3("albedo", glm::value_ptr(settings.scene.materials[i].albedo), 0.01f);
        ImGui::DragFloat3("emmColor", glm::value_ptr(settings.scene.materials[i].emmColor), 0.01f);
        ImGui::DragFloat("roughness", &settings.scene.materials[i].roughness, 0.01f, 0.001f, 1.0f);
        ImGui::DragFloat("metalness", &settings.scene.materials[i].metalness, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("index of refraction", &settings.scene.materials[i].idr, 0.01f, 0.0f, FLT_MAX);
        ImGui::DragFloat("transmittance", &settings.scene.materials[i].transmittance, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("subsurface", &settings.scene.materials[i].subsurface, 0.01f, 0.0f, 1.0f);
        ImGui::DragInt("albedoMap", reinterpret_cast<int*>(&settings.scene.materials[i].albedoTexture));
        ImGui::DragInt("roughnessMap", reinterpret_cast<int*>(&settings.scene.materials[i].roughnessTexture));
        ImGui::DragInt("metallicnesMap", reinterpret_cast<int*>(&settings.scene.materials[i].metallicnesTexture));
        ImGui::DragInt("normalMap", reinterpret_cast<int*>(&settings.scene.materials[i].normalTexture));
        if(settings.scene.materials[i].albedoTexture != -1)
            ImGui::Text("I have a texture!");

        ImGui::PopID();
    }
    ImGui::End();
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

    RendWindowSize = display_size;
    RendWindowPos = ImGui::GetWindowPos();
    ImGuizmo::SetDrawlist();
    Gizmo(ImGui::IsWindowFocused());

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
        
        glm::vec3 transform, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model.model)
    ,glm::value_ptr(transform), glm::value_ptr(rotation), glm::value_ptr(scale));


        modelJson["position"]["x"] = transform.x; 
        modelJson["position"]["y"] = transform.y; 
        modelJson["position"]["z"] = transform.z; 

        modelJson["rotation"]["x"] = rotation.x; 
        modelJson["rotation"]["y"] = rotation.y; 
        modelJson["rotation"]["z"] = rotation.z;

        modelJson["scale"]["x"] = scale.x; 
        modelJson["scale"]["y"] = scale.y; 
        modelJson["scale"]["z"] = scale.z;
        modelJson["meshCount"] = model.meshes.size();
        for(int j = 0; j < model.meshes.size(); j++)
        {
            json meshJson;
            meshJson["matIdx"] = model.meshes[j].matIndex;
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
                mesh.matIndex = 0;
                mesh.nodeId = 0;
                data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
                data.meshes.push_back(mesh);
                model.Load(data);
                break;
            }
            case(PLANE):
            {
                ModelConstructData data;
                Mesh mesh;
                mesh.vertices = PlaneVertices;
                mesh.indices = PlaneIndices;
                mesh.matIndex = 0;
                mesh.nodeId = 0;
                data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
                data.meshes.push_back(mesh);
                model.Load(data);
                break;
            }
            case(SPHERE):
            {
                ModelConstructData data;
                Mesh mesh;
                mesh.vertices = SphereVertices;
                mesh.indices = SphereIndices;
                mesh.matIndex = 0;
                mesh.nodeId = 0;
                data.nodeData.push_back({glm::mat4(1.0f),UINT32_MAX, "root", "root"});
                data.meshes.push_back(mesh);
                model.Load(data);
                break;
            }
        }
        model.type = modelJson["type"];
        for(int j = 0; j < model.meshes.size(); j++)
        {
            model.meshes[j].matIndex = modelJson["meshes"][std::to_string(j)]["matIdx"];
        }

        glm::vec3 transform, rotation, scale;

        transform.x = modelJson["position"]["x"].get<float>();
        transform.y = modelJson["position"]["y"].get<float>();
        transform.z = modelJson["position"]["z"].get<float>();

        rotation.x = modelJson["rotation"]["x"].get<float>();
        rotation.y = modelJson["rotation"]["y"].get<float>();
        rotation.z = modelJson["rotation"]["z"].get<float>();

        scale.x = modelJson["scale"]["x"].get<float>();
        scale.y = modelJson["scale"]["y"].get<float>();
        scale.z = modelJson["scale"]["z"].get<float>();

        ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation), glm::value_ptr(scale)
        ,glm::value_ptr(model.model));
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
}

void GUI::Gizmo(bool focus)
{
    if(focus)
    {
        if(glfwGetKey(window, GLFW_KEY_T))
                gizmoOp = ImGuizmo::TRANSLATE;
        else if(glfwGetKey(window, GLFW_KEY_R))
            gizmoOp = ImGuizmo::ROTATE;
        else if(glfwGetKey(window, GLFW_KEY_S))
            gizmoOp = ImGuizmo::SCALE;

    }

    if(SelectedModelId < 0 || SelectedModelId >= settings.scene.models.size())
        SelectedModelId = 0;

    Model& model = settings.scene.models[SelectedModelId];
    glm::mat4 mat = model.model;
    bool isChild = false;
    glm::mat4 transformNodeMat = glm::mat4(1.0f);
    if(SelectedNodeId != -1)
    {
        if(SelectedNodeId < 0 || SelectedNodeId >= settings.scene.models[SelectedModelId].nodeData.size())
        {
            SelectedNodeId = -1;
        }else
        {
            transformNodeMat = vkUtils::NodeHierarchyTransform(SelectedNodeId, settings.scene.models[SelectedModelId].nodeData);
            mat = model.model * transformNodeMat;
            isChild = true;
        }
    }
    
    ImGuizmo::SetRect(RendWindowPos.x, RendWindowPos.y, RendWindowSize.x, RendWindowSize.y);
    ImGuizmo::Manipulate(glm::value_ptr(settings.scene.camera.GetView()), glm::value_ptr(settings.scene.camera.GetProjection())
    ,gizmoOp,
    ImGuizmo::LOCAL, glm::value_ptr(mat), NULL, NULL);

    if(!isChild)
    {
        model.model = mat;
    }else
    {
        glm::mat4 parentSpace = transformNodeMat * glm::inverse(settings.scene.models[SelectedModelId].nodeData[SelectedNodeId].transform);

        settings.scene.models[SelectedModelId].nodeData[SelectedNodeId].transform
         = glm::inverse(parentSpace) * glm::inverse(model.model) * mat;
    }
}

void GUI::LoadExternalScene(const ModelConstructData& data, Model& model)
{
    model.Load(data);
    settings.scene.textures.insert(settings.scene.textures.end(), data.textureData.begin(), data.textureData.end());
    for(int i = 0; i < model.meshes.size(); i++)
    {
        model.meshes[i].matIndex += settings.scene.materials.size();
    }
    settings.scene.materials.insert(settings.scene.materials.end(), data.materials.begin(), data.materials.end());
}

void GUI::TexturesWindow()
{
    ImGui::Begin("Textures");

    for(int i = 0; i < settings.scene.textures.size(); i++)
    {
        ImGui::PushID(i);

        ImGui::Text("%s", settings.scene.textures[i].path.c_str());

        ImGui::PopID();
    }

    ImGui::End();
}
void GUI::ModelDataWindow(uint32_t obj)
{

    ImGui::Begin("Model Editor");

    Model& model = settings.scene.models[obj];

    glm::vec3 transform(1.0f), rotation(1.0f), scale(1.0f);
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model.model),
    glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale));

    ImGui::SeparatorText(std::string("Model: " + std::to_string(obj)).c_str());
    ImGui::DragFloat3("Position", glm::value_ptr(transform), 0.01f);
    ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f);
    ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);

    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale)
    ,glm::value_ptr(model.model));
    if(ImGui::Button("Delete"))
    {
        settings.scene.models.erase(settings.scene.models.begin() + obj);
    }
    ImGui::End();
}

void GUI::NodeDataWindow(uint32_t objId, uint32_t nodeId)
{
    if(nodeId == -1)
        return;

    ImGui::Begin("Mesh Editor");

    Model& model = settings.scene.models[objId];
    NodeData& node = model.nodeData[nodeId];

    glm::vec3 transform(1.0f), rotation(1.0f), scale(1.0f);
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node.transform),
    glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale));

    ImGui::SeparatorText(node.name.c_str());
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Remember, that is the local transform!");
    }
    ImGui::DragFloat3("Position", glm::value_ptr(transform), 0.01f);
    ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f);
    ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);
    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale)
    ,glm::value_ptr(model.nodeData[nodeId].transform));
    ImGui::End();
}
