cmake_minimum_required(VERSION 4.0)
project(Engine)

set(SHADER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/raygen.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/miss.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/closestHit.glsl
)

set(SHADER_OUTPUT
    ${CMAKE_BINARY_DIR}/shaders/shader.rgen.spv
    ${CMAKE_BINARY_DIR}/shaders/shader.rmiss.spv
    ${CMAKE_BINARY_DIR}/shaders/shader.rchit.spv
)
find_package(Vulkan REQUIRED)
message(STATUS "GLSLC = ${Vulkan_GLSLC_EXECUTABLE}")

add_custom_command(
    OUTPUT ${SHADER_OUTPUT}

    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/shaders

    COMMAND ${Vulkan_GLSLC_EXECUTABLE}
            -fshader-stage=rgen
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/raygen.glsl
            -o ${CMAKE_BINARY_DIR}/shaders/shader.rgen.spv
            --target-env=vulkan1.4

    COMMAND ${Vulkan_GLSLC_EXECUTABLE}
            -fshader-stage=rmiss
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/miss.glsl
            -o ${CMAKE_BINARY_DIR}/shaders/shader.rmiss.spv
            --target-env=vulkan1.4

    COMMAND ${Vulkan_GLSLC_EXECUTABLE}
            -fshader-stage=rchit
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/closesthit.glsl
            -o ${CMAKE_BINARY_DIR}/shaders/shader.rchit.spv
            --target-env=vulkan1.4

    DEPENDS ${SHADER_SRC}

    COMMENT "Compiling Vulkan shaders"
)

add_custom_target(shaders ALL
    DEPENDS ${SHADER_OUTPUT}
)

add_library(
    ${PROJECT_NAME}
    STATIC
    rtpipeline/pipeline.cpp
    rtpipeline/bindingTable.cpp
    rtpipeline/pipelineLayout.cpp
    bvh/blas.cpp
    bvh/tlas.cpp
    resources/model.cpp
    resources/resources.cpp
    pathtracer.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC core)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
