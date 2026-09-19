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
        bool foundDot = false;
        for(int i = data.path.size() - 1; i >= 0; i--)
        {
            if(data.path[i] == '.')
            {
                foundDot = true;
                continue;
            }
            
            if(data.path[i] == '/' || data.path[i] == '\\')
                break;

            if(foundDot)
                name = data.path[i] + name;
        }
    }
}


glm::mat4 Model::GetModelInverse()
{
    return glm::inverse(model);
}