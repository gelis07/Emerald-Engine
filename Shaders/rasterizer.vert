#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require // Optional: Enables pointer arithmetic
#extension GL_ARB_gpu_shader_int64 : enable

layout (location = 0) in vec3 iPos;
layout (location = 1) in vec2 iTexCoords;
layout (location = 2) in vec3 iNormals;

// layout(std430,buffer_reference, buffer_reference_align = 16) readonly buffer UniformBO
// {
//     mat4 mvp;
// };
layout(push_constant) uniform PushConstants
{
    mat4 mvp; 
} push;

void main()
{
    // UniformBO ubo = UniformBO(push.uboAddress);

    gl_Position = push.mvp * vec4(iPos, 1.0);
}