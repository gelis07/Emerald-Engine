cmake_minimum_required(VERSION 3.10)
project(Raytracer)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  message(STATUS "No build type specified. Defaulting to Release.")
endif()

# Optional: make choices visible in tools like cmake-gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

include_directories(${CMAKE_SOURCE_DIR}/vendor/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/glfw/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/imgui-new/backends/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/imgui-new/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/assimp/include/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/glm)
include_directories(${CMAKE_SOURCE_DIR}/vendor)
include_directories(${CMAKE_SOURCE_DIR}/vendor/fmt/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/json/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories("C:/VulkanSDK/1.4.350.0/Include")


link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(Raytracer
src/main.cpp
src/app/AssimpLoader.cpp
src/vkEngine/vkRasterizer.cpp
src/vkEngine/VkSceneManager.cpp
src/vkEngine/vkRaytracer/vkRaytracer.cpp
src/vkEngine/vkRaytracer/LightSampling.cpp
src/vkEngine/vkRaytracer/ASManager.cpp
src/vkEngine/vkRaytracer/RtPipeline.cpp
src/vkEngine/vkBackend.cpp
src/vkEngine/VkEngineApiBinding.cpp
src/vkEngine/Animator.cpp
src/app/GUI.cpp
src/app/CameraControl.cpp
src/core/Model.cpp
vendor/imgui-new/imgui.cpp
vendor/imgui-new/imgui_draw.cpp
vendor/imgui-new/imgui_demo.cpp
vendor/imgui-new/backends/imgui_impl_glfw.cpp
vendor/imgui-new/backends/imgui_impl_vulkan.cpp
vendor/imgui-new/imgui_tables.cpp
vendor/imgui-new/misc/cpp/imgui_stdlib.cpp
vendor/imgui-new/imgui_widgets.cpp
vendor/ImGuizmo/src/ImGuizmo.cpp
src/app/vkApp.cpp
)

target_link_libraries(Raytracer PRIVATE libglfw3.a)
target_link_libraries(Raytracer PRIVATE vulkan-1.lib)
target_link_libraries(Raytracer PRIVATE libassimp-6.dll)