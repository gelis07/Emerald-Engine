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
                            int vertex = std::stoi(word);
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
}