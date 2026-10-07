#pragma once
#include <nlohmann/json_fwd.hpp>
#include <resources/scene.hpp>

namespace loader
{
    enum MODEL_TYPE
    {
        PLANE = 2,
        CUSTOM = 1
    };

    class sceneLoader
    {
        public:
        static engine::Scene loadSceneFile(core::context context, const std::string& path);
        private:
        static glm::vec3 jsonToVec3(nlohmann::json json);
        static glm::vec3 jsonColorToVec3(nlohmann::json json);
        static glm::mat4 transfromFromJson(nlohmann::json json);
        static std::vector<engine::Mesh> loadJsonMeshes(core::context context, nlohmann::json json, uint32_t meshCount, MODEL_TYPE type);
    };
}