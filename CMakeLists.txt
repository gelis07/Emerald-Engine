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
include_directories(${CMAKE_SOURCE_DIR}/vendor/imgui/backends/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/imgui/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/assimp/include/)
include_directories(${CMAKE_SOURCE_DIR}/vendor/glm)
include_directories(${CMAKE_SOURCE_DIR}/vendor)
include_directories(${CMAKE_SOURCE_DIR}/vendor/fmt/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor/json/include)
include_directories(${CMAKE_SOURCE_DIR}/src)


link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(Raytracer
src/main.cpp
src/app/Application.cpp
src/app/AssimpLoader.cpp
src/engine/Renderer.cpp
src/engine/Rasterizer.cpp
src/engine/GPUBackend.cpp
src/app/GUI.cpp
src/app/CameraControl.cpp
src/core/Model.cpp
vendor/glad/src/glad.c
vendor/imgui/imgui.cpp
vendor/imgui/imgui_draw.cpp
vendor/imgui/backends/imgui_impl_glfw.cpp
vendor/imgui/backends/imgui_impl_opengl3.cpp
vendor/imgui/imgui_tables.cpp
vendor/imgui/imgui_widgets.cpp
)

target_link_libraries(Raytracer PRIVATE libglfw3.a)
target_link_libraries(Raytracer PRIVATE libassimp-6.dll)