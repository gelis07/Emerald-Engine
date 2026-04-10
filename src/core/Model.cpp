#include "Model.h"
#include "fmt/base.h"
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

    type = CUSTOM;
    fileSource = path;
}


void Model::Load(const std::vector<float>& iVertices, const std::vector<unsigned int>& iIndices)
{
    mVertices = iVertices;
    mTriangles = iIndices;
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