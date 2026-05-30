#include "Model.h"
#include <algorithm>
#include <string>

enum LoadingType
{
    TRIANGLE,
    VERTEX,
    VERTEX_COORD
};

int Model::ChooseSliceAxis(const AABB& aabb)
{
    glm::vec3 size(aabb.max.x - aabb.min.x,
    aabb.max.y - aabb.min.y, 
    aabb.max.z - aabb.min.z);
    
    if(size.x >= size.y && size.x >= size.z)
    {
        return 0;
    }
    else if(size.y >= size.x && size.y >= size.z)
    {
        return 1;
    }
    else if(size.z >= size.y && size.z >= size.x)
    {
        return 2;
    }

    return -1;
}


void Model::Load(const ModelConstructData& data)
{
    mMeshes = data.meshes;
    GetAABBTriangles();
    ConstructAABBBounds(ModelAabb);
    aabbs.push_back(ModelAabb);
    if(ModelAabb.mTriangleList.size() > 12)
    {
        SliceAABB(0, ChooseSliceAxis(ModelAabb));
    }else
    {
        aabbs[0].leaf = true;
    }
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


void Model::ConstructAABBBounds(AABB& aabb)
{
    for (int i = 0; i < aabb.mTriangleList.size(); i++)
    {
        aabb.min.x = glm::min(aabb.mTriangleList[i].min.x, aabb.min.x);
        aabb.min.y = glm::min(aabb.mTriangleList[i].min.y, aabb.min.y);
        aabb.min.z = glm::min(aabb.mTriangleList[i].min.z, aabb.min.z);

        aabb.max.x = glm::max(aabb.mTriangleList[i].max.x, aabb.max.x);
        aabb.max.y = glm::max(aabb.mTriangleList[i].max.y, aabb.max.y);
        aabb.max.z = glm::max(aabb.mTriangleList[i].max.z, aabb.max.z);
    }
}

void Model::GetAABBTriangles()
{
    for (int m = 0; m < mMeshes.size(); m++)
    {
        const Mesh& mesh = mMeshes[m];
        for (int i = 0; i < mesh.indices.size(); i+=3) 
        {
            Triangle tri;

            tri.a = mesh.vertices[mesh.indices[i]].position;
            tri.b = mesh.vertices[mesh.indices[i+1]].position;
            tri.c = mesh.vertices[mesh.indices[i+2]].position;
            tri.texA = mesh.vertices[mesh.indices[i]].texCoords;
            tri.texB = mesh.vertices[mesh.indices[i+1]].texCoords;
            tri.texC = mesh.vertices[mesh.indices[i+2]].texCoords;
            tri.meshIdx = m;
            float tempMinX = glm::min(tri.a.x, tri.b.x);
            tri.min.x = glm::min(tempMinX, tri.c.x); 

            float tempMinY = glm::min(tri.a.y, tri.b.y);
            tri.min.y = glm::min(tempMinY, tri.c.y); 

            float tempMinZ = glm::min(tri.a.z, tri.b.z);
            tri.min.z = glm::min(tempMinZ, tri.c.z); 

            float tempMaxX = glm::max(tri.a.x, tri.b.x);
            tri.max.x = glm::max(tempMaxX, tri.c.x); 

            float tempMaxY = glm::max(tri.a.y, tri.b.y);
            tri.max.y = glm::max(tempMaxY, tri.c.y); 

            float tempMaxZ = glm::max(tri.a.z, tri.b.z);
            tri.max.z = glm::max(tempMaxZ, tri.c.z);
            
            ModelAabb.mTriangleList.push_back(tri);
        }
    }
}


glm::mat4 Model::GetModelInverse()
{
    return glm::inverse(model);
}

void Model::SliceAABB(int idx, int axis)
{
    AABB& aabb = aabbs[idx];
    std::sort(aabb.mTriangleList.begin(), aabb.mTriangleList.end(),
    [axis](Triangle a, Triangle b)
    {
        return a.min[axis] < b.min[axis];
    });

    size_t mid = aabb.mTriangleList.size() / 2;
    AABB aabbA;
    aabbA.mTriangleList.assign(
        aabb.mTriangleList.begin(),
        aabb.mTriangleList.begin() + mid
    );

    AABB aabbB;
    aabbB.mTriangleList.assign(
        aabb.mTriangleList.begin() + mid,
        aabb.mTriangleList.end()
    );
    ConstructAABBBounds(aabbA);
    ConstructAABBBounds(aabbB);
    aabbA.nodeNum = aabb.nodeNum+1;
    aabbB.nodeNum = aabb.nodeNum+1;

    aabbs.push_back(aabbA);
    int nodeA = aabbs.size() - 1;

    aabbs.push_back(aabbB);
    int nodeB = aabbs.size() - 1;

    aabbs[idx].nodeA = nodeA;
    aabbs[idx].nodeB = nodeB;

    if(aabbA.mTriangleList.size() > 12)
    {
        SliceAABB(nodeA, ChooseSliceAxis(aabbA));
        SliceAABB(nodeB, ChooseSliceAxis(aabbB));
    }else{
        aabbs[nodeA].leaf = true;
        aabbs[nodeB].leaf = true;
    }
}
