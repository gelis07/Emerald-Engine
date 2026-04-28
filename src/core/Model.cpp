#include "Model.h"
#include "fmt/base.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>

enum LoadingType
{
    TRIANGLE,
    VERTEX
};
void Model::Load(const std::string& path)
{
    std::ifstream file(path);
    std::string line;
    if (file.is_open()) {
        while (getline(file, line)) 
        {
            std::stringstream ss(line);
            std::string word;
            LoadingType type;
            int i = 0;
            bool skip = false;
            while(ss >> word && !skip)
            {
                if(i == 0)
                {
                    if(word == "v")
                    {
                        type = VERTEX;
                    }else if(word == "f")
                    {
                        type = TRIANGLE;
                    }else
                    {
                        skip = true;
                    }
                }else
                {
                    switch (type)
                    {
                        case VERTEX:
                        {
                            float coord = std::stof(word);
                            mVertices.push_back(coord);
                            break;
                        }
                        case TRIANGLE:
                        {
                            int vertex = std::stoi(word) - 1;
                            mTriangles.push_back(vertex);
                            break;
                        }
                    }
                }

                i++;
            }
        }

        file.close();
    }
    else {
        fmt::println("cannot open file");
    }
    fmt::println("Dragon time!");

    GetAABBTriangles();
    ConstructAABBBounds(ModelAabb);
    aabbs.push_back(ModelAabb);
    SliceAABB(0, ChooseSliceAxis(ModelAabb));

    aabbs.reserve(100);
    type = CUSTOM;
    fileSource = path;
}

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
void Model::Load(const std::vector<float>& iVertices, const std::vector<unsigned int>& iIndices)
{
    fmt::println("Ill be talking about a cube!");
    mVertices = iVertices;
    mTriangles = iIndices;
    GetAABBTriangles();
    ConstructAABBBounds(ModelAabb);
    aabbs.push_back(ModelAabb);
    SliceAABB(0, ChooseSliceAxis(ModelAabb));
    type = CUBE;
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
    for (int i = 0; i < mTriangles.size(); i+=3) 
    {
        Triangle tri;

        tri.a = glm::vec3(
            mVertices[mTriangles[i] * 3],
            mVertices[mTriangles[i] * 3 + 1],
            mVertices[mTriangles[i] * 3 + 2]);

        tri.b = glm::vec3(
            mVertices[mTriangles[i+1] * 3],
            mVertices[mTriangles[i+1] * 3 + 1],
            mVertices[mTriangles[i+1] * 3 + 2]);

        tri.c = glm::vec3(
            mVertices[mTriangles[i+2] * 3],
            mVertices[mTriangles[i+2] * 3 + 1],
            mVertices[mTriangles[i+2] * 3 + 2]);
        
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

    // fmt::println("Model triangle count: {}", ModelAabb.mTriangleList.size());
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
        fmt::println("Node depth: {}", aabbA.nodeNum);
        aabbs[nodeA].leaf = true;
        aabbs[nodeB].leaf = true;
    }

}
