cmake_minimum_required(VERSION 3.10)
project(Raytracer)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  message(STATUS "No build type specified. Defaulting to Release.")
endif()

# Optional: make choices visible in tools like cmake-gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

include_directories(D:/sdk/glad/include)
include_directories(D:/sdk/GLFW/include)
include_directories(D:/sdk/ImGuiDocking/backends/)
include_directories(D:/sdk/ImGuiDocking/)
include_directories(D:/sdk/GLM)
include_directories(D:/sdk/fmt/include)
include_directories(D:/sdk/nlhjson/single_include/nlohmann)
include_directories(D:/sdk/Lua)


link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(Raytracer
src/main.cpp
D:/sdk/glad/src/glad.c
)

target_link_libraries(Raytracer PRIVATE libglfw3.a)