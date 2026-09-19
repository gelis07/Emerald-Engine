#pragma once
#include <fstream>
#include <cstring>
#include <iostream>
#include <glm/glm.hpp>


constexpr int RTXimgWidth = 1920; 
constexpr int RTXimgHeight = 1080; 

namespace Utils
{

    template <typename T>
    struct linkedList
    {
        T* parent = nullptr;
        T* child = nullptr;
    };

    inline std::string checkExtensionOfFile(const std::string& path)
    {
        std::string extension = "";
        for(int i = path.size() - 1; i >= 0; i--)
        {
            if(path[i] == '.')
                break;

            extension = path[i] + extension;
        }
        return extension;
    }
    inline bool ExistsFile(const std::string& path)
    {
        return std::ifstream(path.c_str()).good();
    }

    inline char* ReadFile(const std::string& path)
    {
        std::ifstream file(path);
        if(!file)
            return nullptr;

        std::string text((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

        char* buffer = new char[text.size() + 1];
        memcpy(buffer, text.c_str(), text.size() + 1); // includes '\0'
        return buffer;
    }

    inline std::vector<char> ReadFileBinary(const std::string& path)
    {
        // Open the file in binary mode and seek to the end to get the size
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open shader file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        // Seek back to the beginning and read the file
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    } 
    inline glm::vec3 RandomVec3()
    {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        return glm::vec3(r,g,b);
    }
    
}
