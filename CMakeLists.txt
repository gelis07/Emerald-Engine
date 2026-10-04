cmake_minimum_required(VERSION 4.0)
project(Editor)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/vendor)

add_executable(${PROJECT_NAME} 
main.cpp
vendor/fpng.cpp
)
target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)
add_subdirectory(pathtracer)
add_subdirectory(core)