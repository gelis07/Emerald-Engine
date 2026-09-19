#version 430 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 oColor;
layout(location = 0) in vec2 uv;
const uint UINT_MAX = 0xFFFFFFFFu;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    vec3 albedo;
    uint textId;
} push;

layout(binding = 0, set = 0) uniform sampler2D texImage[];


void main()
{

    vec3 color = push.albedo;
    if(push.textId != UINT_MAX)
    {
        color = textureLod(texImage[push.textId], uv, 0.0).rgb;
    }

    oColor = vec4(color, 1.0);
}     