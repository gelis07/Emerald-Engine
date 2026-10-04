#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <functional> 
#include <glm.hpp>
#include <cstdlib>

#ifdef FMT

#include <fmt/format.h>

#define CORE_PRINT(...) fmt::println(__VA_ARGS__)

#else

#include <iostream>
#include <string>
#include <utility>

namespace core {

    // Base case: no formatting arguments left.
    inline void print_args(std::ostream& os, const std::string& format)
    {
        os << format;
    }

    template <typename T, typename... Args>
    void print_args(std::ostream& os,
                    const std::string& format,
                    T&& value,
                    Args&&... args)
    {
        const size_t pos = format.find("{}");

        if (pos == std::string::npos) {
            // No more placeholders.
            os << format;
            return;
        }

        // Print everything before "{}".
        os << format.substr(0, pos);

        // Print the current argument.
        os << std::forward<T>(value);

        // Process the rest of the format string.
        print_args(
            os,
            format.substr(pos + 2),
            std::forward<Args>(args)...
        );
    }

    inline void print(const char* format)
    {
        std::cout << format << std::endl;
    }

    template <typename... Args>
    void print(const char* format, Args&&... args)
    {
        print_args(
            std::cout,
            std::string(format),
            std::forward<Args>(args)...
        );

        std::cout << std::endl;
    }

}

#define CORE_PRINT(...) core::print(__VA_ARGS__)

#endif

namespace core
{

    inline void ExecuteSingleTimeCb(const vk::Device& device, const vk::CommandPool& commandPool, const vk::Queue& queue,const std::function<void(const vk::CommandBuffer &singleTimeCb)>& c)
    {
        vk::CommandBufferAllocateInfo cbAllocInfo{};
        cbAllocInfo.setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

        vk::CommandBuffer singleBuffer = device.allocateCommandBuffers(cbAllocInfo).front();
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        singleBuffer.begin(beginInfo);
        c(singleBuffer);
        singleBuffer.end();

        vk::SubmitInfo subInfo{};
        subInfo.setWaitSemaphoreCount(0)
        .setPWaitSemaphores(nullptr)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&singleBuffer);
        vk::Fence fence;
        fence = device.createFence({});

        queue.submit(subInfo, fence);

        vk::Result result = device.waitForFences(1, &fence, vk::True, UINT64_MAX);

        device.destroyFence(fence);
        device.freeCommandBuffers(commandPool, singleBuffer);
    }

    inline std::vector<char> ReadFileBinary(const std::string& path)
    {
        // Open the file in binary mode and seek to the end to get the size
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            CORE_PRINT("failed to open shader file with path {}!", path);
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        // Seek back to the beginning and read the file
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    } 
    template<typename T> T checkVulkanNull(T t)
    {
        if(t == VK_NULL_HANDLE)
        {
            CORE_PRINT("This {} doesn't exist", typeid(T).name());
            std::exit(EXIT_FAILURE);
        }
        return t;
    }
    inline VkTransformMatrixKHR GlmToVk(const glm::mat4& matrix)
    {
        VkTransformMatrixKHR vkMatrix;
        vkMatrix.matrix[0][0] = matrix[0][0];
        vkMatrix.matrix[0][1] = matrix[1][0];
        vkMatrix.matrix[0][2] = matrix[2][0];
        vkMatrix.matrix[0][3] = matrix[3][0];

        vkMatrix.matrix[1][0] = matrix[0][1];
        vkMatrix.matrix[1][1] = matrix[1][1];
        vkMatrix.matrix[1][2] = matrix[2][1];
        vkMatrix.matrix[1][3] = matrix[3][1];

        vkMatrix.matrix[2][0] = matrix[0][2];
        vkMatrix.matrix[2][1] = matrix[1][2];
        vkMatrix.matrix[2][2] = matrix[2][2];
        vkMatrix.matrix[2][3] = matrix[3][2];

        return vkMatrix;
    }
};
