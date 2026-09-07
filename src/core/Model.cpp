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
    meshes = data.meshes;
    nodeData = data.nodeData;
    if(data.path != "")
    {
        type = CUSTOM;
        fileSource = data.path;
    }
}


glm::mat4 Model::GetModelInverse()
{
    return glm::inverse(model);
}