#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable


layout (location = 0) in vec3 iPos;
layout (location = 1) in vec2 iTexCoords;
layout (location = 2) in vec3 iNormals;
layout (location = 3) in vec3 iTangent;
layout (location = 4) in vec3 iBitangent;
layout (location = 5) in uint iBoneOffset;
layout (location = 6) in uint iBoneCount;

layout (location = 0) out vec2 uv;


layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    vec3 albedo;
    uint textId;
} push;


void main()
{
    uv = iTexCoords;
    gl_Position = push.mvp * vec4(iPos, 1.0);
}