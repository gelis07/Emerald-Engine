cmake_minimum_required(VERSION 4.0)
project(Editor)

#set your own path.
set(assimp_DIR "C:/sdk/assimp/INSTALL/lib/cmake/assimp-5.4")
set(nlohmann_json_DIR "C:/sdk/nlhjson/install/share/cmake/nlohmann_json")

find_package(assimp CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)



if(assimp_FOUND)
message(STATUS "assimp found.")
else()
message(FATAL_ERROR "couldn't find cmake abort. Make sure to call -Dassimp_DIR with the specified 
path to the cmake installition of assimp or change the assimp_DIR directly from the cmake file.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/vendor)

add_executable(${PROJECT_NAME} 
main.cpp
vendor/fpng.cpp
app/extern/loadModelFiles.cpp
app/extern/loadSceneFiles.cpp
)
target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Engine assimp::assimp nlohmann_json::nlohmann_json)
add_subdirectory(pathtracer)
add_subdirectory(core)