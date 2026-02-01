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
            std::vector<int> vertices;
            std::vector<float> pos;
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
                            pos.push_back(coord);
                            break;
                        }
                        case TRIANGLE:
                        {
                            int vertex = std::stoi(word);
                            vertices.push_back(vertex);
                            break;
                        }
                    }
                }

                i++;
            }
            if(!skip)
            {
                if(vertices.size() != 0)
                {
                    mTriangles.push_back({vertices[0], vertices[1], vertices[2]});
                }else if(pos.size() != 0)
                {
                    mVertices.push_back({pos[0], pos[1], pos[2]});
                }
            }
        }

        file.close();
    }
    else {
        fmt::println("cannot open file");
    }
}