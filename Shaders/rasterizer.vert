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

const uint UINT_MAX = 0xFFFFFFFFu;

struct BoneInfluence
{
    uint boneId;
    float weight;
};

struct BoneData
{
    mat4 transform;
};

layout(binding = 0, set = 0) readonly buffer BoneInfluenceBuffer
{
    BoneInfluence boneInf[];
} boneInfluenceBuffer;

layout(binding = 1, set = 0) readonly buffer BoneBuffer
{
    BoneData bones[];
} boneBuffer;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint objId;
    uint modelId;
} push;


void main()
{
    vec3 boneVertPos = vec3(iPos);
    // if(iBoneCount == 0)
    // {
    //     boneVertPos = iPos;
    // }else{
    //     for(uint i = iBoneOffset; i < iBoneOffset + iBoneCount; i++)
    //     {
    //         boneVertPos += vec3(boneInfluenceBuffer.boneInf[i].weight * boneBuffer.bones[boneInfluenceBuffer.boneInf[i].boneId].transform * vec4(iPos, 1.0));
    //     }
    // }

    gl_Position = push.mvp * vec4(boneVertPos, 1.0);
}