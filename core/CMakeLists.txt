cmake_minimum_required(VERSION 4.0)
project(core)

#set your own path.
set(fmt_DIR "C:/sdk/fmt/install/lib/cmake/fmt")

find_package(Vulkan REQUIRED)
find_package(fmt CONFIG QUIET)

add_library(${PROJECT_NAME}
STATIC
device.cpp
instance.cpp
allocator.cpp
instance.cpp
shaderModules.cpp
)

if(fmt_FOUND)
    message(STATUS "fmt found")
    target_compile_definitions(${PROJECT_NAME} PRIVATE FMT)
    target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
else()
    message(STATUS "fmt not found, falling back to std")
    message(STATUS "if fmt is installed, please link the cmake installation of fmt using -Dfmt_DIR with the path
    or change the CMakeLists.txt fmt_DIR variable directly with the path")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
message(STATUS "Release build")
message(STATUS "Valdation layers OFF")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
message(STATUS "Debug build")
message(STATUS "Validation layers ON")
target_compile_definitions(${PROJECT_NAME} PRIVATE VALIDATION_LAYERS)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#CONFIGURE TO YOUR GLM PATH
set(GLM_PATH "C:/sdk/GLM")

if(NOT EXISTS "${GLM_PATH}/glm.hpp")
    message(FATAL_ERROR "GLM is not found, aborting. 
    Please install GLM and set the path in the cmake file in the core's CMakeLists.txt.")
endif()

message(STATUS "GLM found.")
target_include_directories(${PROJECT_NAME} PUBLIC "C:/sdk/GLM")
