#include "loadSceneFiles.hpp"
#include "LoadModelFiles.hpp"
#include <utils.hpp>
#include <nlohmann/json.hpp>
#include <app/defaultMeshes.hpp>

namespace loader
{
    engine::Scene sceneLoader::loadSceneFile(core::context context,const std::string& path)
    {
        if(!core::fileExists(path))
        {
            CORE_PRINT("{} doesn't exist", path);
            return {};
        }
        engine::Scene scene;
        std::ifstream file(path);
        nlohmann::json jsonScene;
        jsonScene = nlohmann::json::parse(file);
        if(jsonScene["sign"].get<std::string>() != "Emerald Engine")
        {
            CORE_PRINT("{} is not a scene file", path);
            return {};
        }

        scene.camera.pos = jsonToVec3(jsonScene["camera"]["position"]);
        scene.camera.invView = engine::CalcInvViewFromDirection(
            scene.camera.pos,
            jsonToVec3(jsonScene["camera"]["direction"])
        );

        uint32_t materialCount = jsonScene["materials"]["count"];
        for(uint32_t i = 0; i < materialCount; i++)
        {
            engine::Material material;
            nlohmann::json matJson = jsonScene["materials"][std::to_string(i)];
            material.albedo = jsonColorToVec3(matJson["albedo"]);
            material.roughness = matJson["Roughness"];
            material.metalness = matJson["Metalicness"];

            scene.materials.push_back(material);
        }

        uint32_t modelCount = jsonScene["models"]["count"];
        for(uint32_t i = 0; i < modelCount; i++)
        {
            engine::Model model;
            nlohmann::json modelJson = jsonScene["models"][std::to_string(i)];
            MODEL_TYPE type = modelJson["type"];

            model.transform = transfromFromJson(modelJson);
            model.meshes = loadJsonMeshes(context, modelJson["meshes"], modelJson["meshCount"], type);

            scene.models.push_back(model);
        }

        return scene;
    }


    std::vector<engine::Mesh> sceneLoader::loadJsonMeshes(core::context context,nlohmann::json json, uint32_t meshCount,MODEL_TYPE type)
    {
        std::vector<engine::Mesh> meshes;
        std::vector<engine::GPUVertex> vertices;
        std::vector<uint32_t> indices;

        switch(type)
        {
            case PLANE:
            {
                vertices = PlaneVertices;
                indices = PlaneIndices;
                break;
            }
            case CUSTOM:
            {
                loaderOutput output;
                output = assimpLoader::loadModel(json["source"]);
                vertices = output.vertices;
                indices = output.indices;
                break;
            }
        }
        
        for(uint32_t i = 0; i < meshCount; i++)
        {
            nlohmann::json meshJson = json[std::to_string(i)];
            engine::Mesh mesh;
            mesh.InitBuffer(context, vertices, indices);
            mesh.matId = meshJson["matIdx"];
            mesh.transform = transfromFromJson(meshJson);
            meshes.push_back(mesh);
        }

        return meshes;
    }
    glm::mat4 sceneLoader::transfromFromJson(nlohmann::json json)
    {
        glm::vec3 pos = jsonToVec3(json["position"]);
        glm::vec3 rot = glm::radians(jsonToVec3(json["rotation"]));
        glm::vec3 scale = jsonToVec3(json["scale"]);
        glm::mat4 transform(1.0f);
        transform = glm::translate(transform, pos);
        transform = glm::scale(transform, scale);
        transform = core::rotateWithEulerAngles(transform, rot);
        return transform;
    }

    glm::vec3 sceneLoader::jsonToVec3(nlohmann::json json)
    {
        glm::vec3 vec;
        vec.x = json["x"];
        vec.y = json["y"];
        vec.z = json["z"];
        return vec;
    }
    glm::vec3 sceneLoader::jsonColorToVec3(nlohmann::json json)
    {
        glm::vec3 vec;
        vec.x = json["r"];
        vec.y = json["g"];
        vec.z = json["b"];
        return vec;
    }
}