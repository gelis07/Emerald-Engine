#include "GUI.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <core/Utils.h>
#include <fmt/base.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vkEngine/VkEngineApiBinding.h>
#include <tinyfiledialogs.h>


std::vector<std::string> dropCallBackFiles;

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

    MainMenuBar(camControl, info);
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
            scene.models[i].name.c_str(),
            flags
        );
        if (ImGui::IsItemClicked())
        {
            SelectedModelId = i;
            SelectedNodeId = -1;
            SelectedMeshId = -1;
        }
        if (open)
        {
            for (int j = 0; j < scene.models[i].meshes.size(); j++)
            {
                ImGuiTreeNodeFlags nMeshFlags = treeFlags;

                
                bool openMesh = false;
                if(!scene.models[i].meshes[j].bones.empty())
                {
                    if(j == SelectedNodeId)
                        nMeshFlags |= ImGuiTreeNodeFlags_Selected;
                    openMesh = ImGui::TreeNodeEx(
                        scene.models[i].nodeData[j].name.c_str(),
                        nMeshFlags
                    );
                }else
                {
                    nMeshFlags = leafFlags;
                    if(j == SelectedMeshId)
                        nMeshFlags |= ImGuiTreeNodeFlags_Selected;
                    ImGui::TreeNodeEx(
                        scene.models[i].nodeData[j].name.c_str(),
                        nMeshFlags
                    );
                }
                if(openMesh)
                {
                    for(int b = 0; b < scene.models[i].meshes[j].bones.size(); b++)
                    {
                        ImGuiTreeNodeFlags boneFlags = leafFlags;
                        if(scene.models[i].meshes[j].bones[b].nodeId == SelectedNodeId)
                            boneFlags |= ImGuiTreeNodeFlags_Selected;
                        ImGui::TreeNodeEx(
                            scene.models[i].meshes[j].bones[b].name.c_str(),
                            boneFlags
                        );
                        if (ImGui::IsItemClicked())
                        {
                            SelectedModelId = i;
                            SelectedMeshId = j;
                            SelectedNodeId = scene.models[i].meshes[j].bones[b].nodeId;
                        } 
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                if(scene.models[i].meshes[j].bones.empty())
                    ImGui::TreePop();
                if (ImGui::IsItemClicked())
                {
                    SelectedModelId = i;
                    SelectedMeshId = j;
                    SelectedNodeId = -1;
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::SeparatorText("Materials");
    for(int i = 0; i < scene.materials.size(); i++)
    {
        ImGuiTreeNodeFlags flags = leafFlags;
        if(i == SelectedMatId)
            flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx(scene.materials[i].name.c_str(), flags);
        if (ImGui::IsItemClicked())
        {
            SelectedMatId = i;
        }
        ImGui::TreePop();
    }


    ImGui::End();
    

    for(int i = 0; i < dropCallBackFiles.size(); i++)
    {
        std::string fileExtension = Utils::checkExtensionOfFile(dropCallBackFiles[i]);
        if(fileExtension == "fbx"
        ||  fileExtension == "gltf"
        || fileExtension == "obj")
        {
            Model model;
            ModelConstructData data = loader->LoadModel(dropCallBackFiles[i], info);
            LoadExternalScene(data, model);
            model.type = CUSTOM;
            settings.scene.AddModel(std::move(model));
        }
        else if(fileExtension == "png")
        {
            TextureVk newTexture;
            int width, height, channels;
            unsigned char* texData = stbi_load(dropCallBackFiles[i].c_str(), &width, &height, &channels, 4);
            VkImageCreateData imgCreateData;
            imgCreateData.allocator = info.alloc;
            imgCreateData.commandPool = info.cPool;
            imgCreateData.device = info.device;
            imgCreateData.queue = info.queue;
            imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
            newTexture = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
            newTexture.path = dropCallBackFiles[i];
            settings.scene.textures.push_back(newTexture);
            stbi_image_free(texData);
        }else if(fileExtension == "json")
        {
            LoadSettings(dropCallBackFiles[i], camControl, info);
        }else
        {
            fmt::println("I don't know what to do with this file.");
        }
    }
    if(!dropCallBackFiles.empty())
        dropCallBackFiles.clear();


    Timeline();
    RenderSettingsWindow(info, camControl, dt);
    TexturesWindow();
    MeshDataWindow(SelectedModelId, SelectedMeshId);
    MaterialDataWindow();
    ModelDataWindow(SelectedModelId);

    NodeDataWindow(SelectedModelId, SelectedNodeId);
}

void GUI::RenderSettingsWindow(LoadSceneInfo info, CameraControl& camControl, float dt)
{
    Scene& scene = settings.scene;
    Camera& camera = scene.camera;
    ImGui::Begin("Render settings");
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
        cube.name = "Model " + std::to_string(scene.models.size());
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
        plane.name = "Model " + std::to_string(scene.models.size());
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
        plane.name = "Model " + std::to_string(scene.models.size());
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
        mat.name = "Material " + std::to_string(scene.materials.size());
        scene.materials.push_back(mat);
        settings.ReloadScene = true;
    }

    ImGui::Text("delta time is %fms", dt * 1000);
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
    ImGui::Text("%f", (float)animator.animationFrame / animator.timeStep);

    ImGui::End();
}
void GUI::MeshDataWindow(uint32_t objId, uint32_t meshId)
{
    if(meshId == -1)
        return;

    ImGui::Begin("Mesh Window");
    Mesh& mesh = settings.scene.models[objId].meshes[meshId];
    ImGui::InputText("Name: ", &settings.scene.models[objId].nodeData[mesh.nodeId].name);
    std::vector<const char*> materials;
    for(int i = 0; i < settings.scene.materials.size(); i++)
    {
        materials.push_back(settings.scene.materials[i].name.c_str());
    }
    ImGui::Combo(
            "Material",
            reinterpret_cast<int*>(&mesh.matIndex),
            materials.data(),
            materials.size());
    Model& model = settings.scene.models[objId];
    for(int i = 0; i < mesh.bones.size(); i++)
    {
        ImGui::PushID(i);
        uint32_t nodeId = mesh.bones[i].nodeId;
        NodeData& node = model.nodeData[nodeId];
        glm::vec3 transform(1.0f), rotation(1.0f), scale(1.0f);
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node.transform),
        glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale));
        
        ImGui::SeparatorText(node.name.c_str());
        if(ImGui::Button("Select this"))
        {
            SelectedNodeId = nodeId;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Remember, that is the local transform!");
        }
        ImGui::DragFloat3("Position", glm::value_ptr(transform), 0.01f);
        ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f);
        ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);
        ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale)
        ,glm::value_ptr(model.nodeData[nodeId].transform));

        ImGui::PopID();
    }

    ImGui::End();
}
void GUI::TextureDropdown(uint32_t& id, const std::string& name, const std::vector<const char*> options)
{
    int comboIndex = id + 1;
    if (ImGui::Combo(
            name.c_str(),
            reinterpret_cast<int*>(&comboIndex),
            options.data(),
            options.size()))
    {
        id = comboIndex - 1;
    }
}

void GUI::MaterialDataWindow()
{   

    if(SelectedMatId == -1)
        return;


    std::vector<const char*> textures;
    textures.push_back("None");
    for(int i = 0; i < settings.scene.textures.size(); i++)
    {
        textures.push_back(settings.scene.textures[i].path.c_str());
    }
    ImGui::Begin("Materials");
    uint32_t i = SelectedMatId;
    ImGui::InputText("Name: ", &settings.scene.materials[i].name);
    ImGui::DragFloat3("albedo", glm::value_ptr(settings.scene.materials[i].albedo), 0.01f);
    ImGui::DragFloat3("emmColor", glm::value_ptr(settings.scene.materials[i].emmColor), 0.01f);
    ImGui::DragFloat("roughness", &settings.scene.materials[i].roughness, 0.01f, 0.001f, 1.0f);
    ImGui::DragFloat("metalness", &settings.scene.materials[i].metalness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("index of refraction", &settings.scene.materials[i].idr, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragFloat("transmittance", &settings.scene.materials[i].transmittance, 0.01f, 0.0f, 1.0f);
    TextureDropdown(settings.scene.materials[i].albedoTexture, "Albedo map", textures);
    TextureDropdown(settings.scene.materials[i].roughnessTexture, "Roughness map", textures);
    TextureDropdown(settings.scene.materials[i].metallicnesTexture, "Metallicness map", textures);
    TextureDropdown(settings.scene.materials[i].normalTexture, "Normal map", textures);

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
    RendWindowPos = ImGui::GetItemRectMin();
    ImGuizmo::SetDrawlist();
    Gizmo(ImGui::IsWindowFocused());

    ImGui::End();
}

void GUI::SaveSettings(const std::string& path, CameraControl& camControl)
{
    json data;
    data["Sign"] = "Emerald Engine";
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
        modelJson["Name"] = model.name;
        for(int j = 0; j < model.meshes.size(); j++)
        {
            json meshJson;
            meshJson["matIdx"] = model.meshes[j].matIndex;
            glm::vec3 transform(1.0f), rotation(1.0f), scale(1.0f);
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model.nodeData[model.meshes[j].nodeId].transform),
            glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale));
            SaveVec3(meshJson, "position", transform);
            SaveVec3(meshJson, "rotation", rotation);
            SaveVec3(meshJson, "scale", scale);
            meshJson["boneCount"] = model.meshes[j].bones.size();
            for(int b = 0; b < model.meshes[j].bones.size(); b++)
            {
                json boneJson;
                glm::vec3 btransform(1.0f), brotation(1.0f), bscale(1.0f);
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model.nodeData[model.meshes[j].bones[b].nodeId].transform),
                glm::value_ptr(btransform), glm::value_ptr(brotation),glm::value_ptr(bscale));
                SaveVec3(boneJson, "position", btransform);
                SaveVec3(boneJson, "rotation", brotation);
                SaveVec3(boneJson, "scale", bscale);
                meshJson["bone"][std::to_string(b)] = boneJson;
            }
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
        materialsJson["Roughness"] = mat.roughness;
        materialsJson["Metalicness"] = mat.metalness;
        materialsJson["Name"] = mat.name;

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
    std::ofstream file(path);
    file << data.dump(4);
    file.close();
}

void GUI::LoadSettings(const std::string& source, CameraControl& camControl, LoadSceneInfo info)
{



    std::ifstream f(source);
    json data;
    try
    {
        data = json::parse(f);
    }
    catch (const nlohmann::json::parse_error&)
    {
        fmt::println("This isn't a compatable scene file");
        return;
    }

    if(data["Sign"] != "Emerald Engine")
    {
        fmt::println("This isn't a compatable scene file");
        return;
    }    
    if(data.is_discarded())
    {
        fmt::println("This isn't a compatable scene file");
        return;
    }

    settings.scene.models.clear();
    settings.scene.materials.clear();
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
        mat.roughness = data["materials"][std::to_string(i)]["Roughness"].get<float>();
        mat.metalness = data["materials"][std::to_string(i)]["Metalicness"].get<float>();
        mat.name = data["materials"][std::to_string(i)]["Name"].get<std::string>();

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
                    ModelConstructData data = loader->LoadModel(modelJson["source"].get<std::string>(), info, true);
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
        model.name = modelJson["Name"];

        for(int j = 0; j < model.meshes.size(); j++)
        {
            json meshJson = modelJson["meshes"][std::to_string(j)];
            model.meshes[j].matIndex = meshJson["matIdx"];

            NodeData& node = model.nodeData[model.meshes[j].nodeId];
            glm::vec3 transform(1.0f), rotation(1.0f), scale(1.0f);
            LoadVec3(meshJson, "position", transform);
            LoadVec3(meshJson, "rotation", rotation);
            LoadVec3(meshJson, "scale", scale);
            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale)
            ,glm::value_ptr(node.transform));
            uint32_t boneCount = meshJson["boneCount"];

            for(int b = 0; b < model.meshes[j].bones.size(); b++)
            {
                json boneJson = meshJson["bone"][std::to_string(b)];
                glm::vec3 btransform(1.0f), brotation(1.0f), bscale(1.0f);
                LoadVec3(boneJson, "position", btransform);
                LoadVec3(boneJson, "rotation", brotation);
                LoadVec3(boneJson, "scale", bscale);

                ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(btransform), glm::value_ptr(brotation),glm::value_ptr(bscale)
                ,glm::value_ptr(model.nodeData[model.meshes[j].bones[b].nodeId].transform));
            }
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

    ImGui::InputText("Name: ", &model.name);
    ImGui::DragFloat3("Position", glm::value_ptr(transform), 0.01f);
    ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f);
    ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);

    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform), glm::value_ptr(rotation),glm::value_ptr(scale)
    ,glm::value_ptr(model.model));
    if(ImGui::Button("Delete"))
    {
        if(settings.scene.models.size() != 1)
            settings.scene.models.erase(settings.scene.models.begin() + obj);
    }
    ImGui::End();
}

glm::vec3 animPos = glm::vec3(0.0f);
glm::vec3 animRot = glm::vec3(0.0f);
glm::vec3 animScale = glm::vec3(1.0f);
float keyTime = 0.0f;


inline const char* AnimVarMode[] = {
    "Position",
    "Rotation",
    "Scale"
};
inline const char* AnimVarIdxMode[] = {
    "X",
    "Y",
    "Z"
};
int currAnimVar = 0;
int currAnimVarIdx = 0;
void GUI::NodeDataWindow(uint32_t objId, uint32_t nodeId)
{
    if(nodeId == -1)
        return;

    ImGui::Begin("Node Editor");

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

    ImGui::DragFloat3("Animation Position", glm::value_ptr(animPos), 0.01f);
    ImGui::DragFloat3("Animation Rotation", glm::value_ptr(animRot), 0.01f);
    ImGui::DragFloat3("Animation Scale", glm::value_ptr(animScale), 0.01f);

    ImGui::Combo("Anim key var", &currAnimVar, AnimVarMode, IM_ARRAYSIZE(AnimVarMode));
    ImGui::Combo("Anim key Idx var", &currAnimVarIdx, AnimVarIdxMode, IM_ARRAYSIZE(AnimVarIdxMode));

    ImGui::DragFloat("key time", &keyTime);

    if(ImGui::Button("Add keyframe"))
    {
        float value;
        if(currAnimVar == 0)
            value = animPos[currAnimVarIdx];
        else if(currAnimVar == 1)
            value = animRot[currAnimVarIdx];
        else if(currAnimVar == 2)
            value = animScale[currAnimVarIdx];

        animator.addKeyframe({keyTime, value}, {objId, nodeId, static_cast<Variable>(currAnimVar), static_cast<uint32_t>(currAnimVarIdx)});
    }

    ImGui::End();
}


void dropCallback(GLFWwindow* window, int count, const char** paths)
{
    for(int i = 0; i < count; i++)
        dropCallBackFiles.push_back(paths[i]);
}

void GUI::Callbacks()
{
    glfwSetDropCallback(window, dropCallback);
}


void GUI::MainMenuBar(CameraControl& camControl, LoadSceneInfo info)
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Save Scene"))
            {
                const char* path = tinyfd_saveFileDialog(
                    "Save Scene",
                    "scene.json",
                    1,
                    (const char*[]){"*.json"},
                    "JSON Scene"
                );
                if (path)
                {
                    SaveSettings(path, camControl);
                }
            }
            if(ImGui::MenuItem("Load Scene"))
            {
                const char* file = tinyfd_openFileDialog(
                    "Open File",
                    "",
                    0,
                    nullptr,
                    nullptr,
                    0
                );
                if (file)
                {
                    LoadSettings(file, camControl, info);
                    SelectedModelId = 0;
                    SelectedMeshId = -1;
                    SelectedNodeId = -1;
                }
            }

            if(ImGui::MenuItem("Load Model"))
            {
                const char* file = tinyfd_openFileDialog(
                    "Open File",
                    "",
                    0,
                    nullptr,
                    nullptr,
                    0
                );
                if(file)
                {
                    Model model;
                    ModelConstructData data = loader->LoadModel(file, info);
                    LoadExternalScene(data, model);
                    model.type = CUSTOM;
                    settings.scene.AddModel(std::move(model));
                }
            }

            if(ImGui::MenuItem("Load Texture"))
            {
                const char* file = tinyfd_openFileDialog(
                    "Open File",
                    "",
                    0,
                    nullptr,
                    nullptr,
                    0
                );
                if(file)
                {
                    TextureVk newTexture;
                    int width, height, channels;
                    unsigned char* texData = stbi_load(file, &width, &height, &channels, 4);
                    VkImageCreateData imgCreateData;
                    imgCreateData.allocator = info.alloc;
                    imgCreateData.commandPool = info.cPool;
                    imgCreateData.device = info.device;
                    imgCreateData.queue = info.queue;
                    imgCreateData.format = vk::Format::eR8G8B8A8Unorm;
                    newTexture = vkUtils::LoadTexture(width, height, channels, texData, imgCreateData);
                    newTexture.path = file;
                    settings.scene.textures.push_back(newTexture);
                    stbi_image_free(texData);
                }
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View"))
        {
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Settings"))
        {
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void GUI::SaveVec3(json& json, const std::string& name, glm::vec3& vec)
{
    json[name]["x"] = vec.x;
    json[name]["y"] = vec.y;
    json[name]["z"] = vec.z;
}
void GUI::LoadVec3(json& json, const std::string& name, glm::vec3& vec)
{
    vec.x = json[name]["x"];
    vec.y = json[name]["y"];
    vec.z = json[name]["z"];
}



void GUI::Timeline()
{
    ImGui::Begin("Timeline");

    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 pos = ImGui::GetWindowPos();
    double xpos;
    double ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float offset = timelineTime * (size.x / maxTime);


    drawList->AddLine(
        ImVec2(pos.x + offset, pos.y),
        ImVec2(pos.x + offset, pos.y + size.y),
        IM_COL32(255, 0, 0, 255),
        3.0f
    );
    timelineTime = animator.animationFrame / animator.timeStep;

    if(xpos > pos.x && xpos < pos.x + size.x 
    && ypos > pos.y && ypos < pos.y + size.y
    && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
    {
        timelineTime = (maxTime / size.x) * (xpos - pos.x);
    }
    
    animator.animationFrame = timelineTime * animator.timeStep;

    Scene& scene = settings.scene;

    float animCountMax = 10.0f;
    float parts = (float)size.y / animCountMax;

    ImVec2 squareSize = ImVec2(10.0f, 35.0f);
    for(int i = 0; i < animator.keyframes.size(); i++)
    {
        ImVec2 min = pos;
        ImVec2 max = pos;
        min.x += animator.keyframes[i].time * (size.x / maxTime) - squareSize.x * 0.5f;
        min.y += (size.y - squareSize.y) * 0.5f;
        max.x += animator.keyframes[i].time * (size.x / maxTime) + squareSize.x * 0.5f;
        max.y += (size.y + squareSize.y) * 0.5f;

        drawList->AddRectFilled(min, max, IM_COL32(0,255,0,255), 5.0f);
    }


    ImGui::End();
}