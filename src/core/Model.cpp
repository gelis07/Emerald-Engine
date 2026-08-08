#include "Model.h"
#include <string>

enum LoadingType
{
    TRIANGLE,
    VERTEX,
    VERTEX_COORD
};


void Model::Load(const ModelConstructData& data)
{
    mMeshes = data.meshes;
    if(data.path == "")
        type = CUBE;
    else{
        type = CUSTOM;
        fileSource = data.path;
    }
}

void Model::Transform()
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, rotation.y,   glm::vec3(0, 1, 0));
    model = glm::rotate(model, rotation.z,  glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
}

glm::mat4 Model::GetModelInverse()
{
    return glm::inverse(model);
}