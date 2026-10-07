#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#include "payload.glsl"
#include "materials.glsl"
#include "samplers.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

struct Vertex
{
    vec3 position;
    vec2 texCoords;
    vec3 normals;
};

struct Model
{
    mat4 transform;
};

struct Mesh
{
    mat4 transform;
    uint modelId;
    uint matId;
};

layout(binding = 1) uniform accelerationStructureEXT accStruct; 

// layout(scalar, binding = 3, set = 0) buffer IndexBuffer
// {
//     uint i[];
// } indexBuffer[];
// layout(scalar, binding = 4, set = 0) buffer VertexBuffer
// {
//     Vertex v[];
// } vertexBuffer[];
layout(scalar, binding = 5, set = 0) buffer ModelBuffer
{
    Model models[];
} modelBuffer;
layout(scalar, binding = 6, set = 0) buffer MaterialBuffer
{
    Material materials[];
} materialBuffer;
layout(scalar, binding = 7, set = 0) buffer MeshBuffer
{
    Mesh meshes[];
} meshBuffer;


hitAttributeEXT vec3 attrib;

void main()
{
    uint meshId = gl_InstanceCustomIndexEXT;
    // uint startIdx = gl_PrimitiveID * 3;
    // uint i0 = indexBuffer[meshId].i[startIdx + 0];
    // uint i1 = indexBuffer[meshId].i[startIdx + 1];
    // uint i2 = indexBuffer[meshId].i[startIdx + 2];

    // vec3 barrycentric = vec3(
    //     1.0 - attrib.x - attrib.y,
    //     attrib.x,
    //     attrib.y
    // );
    // //Normal in vertex coordinates.
    // vec3 VNormal = vertexBuffer[meshId].v[i0].normals * barrycentric.x
    // + vertexBuffer[meshId].v[i1].normals * barrycentric.y
    // + vertexBuffer[meshId].v[i2].normals * barrycentric.z;

    // vec3 worldNormal = normalize(
    //     inverse(transpose(mat3(gl_WorldToObjectEXT))) * VNormal
    // );
    Mesh mesh = meshBuffer.meshes[meshId];
    // Model model = modelBuffer.models[mesh.modelId];
    Material mat = materialBuffer.materials[mesh.matId];

    // float a = max(mat.roughness * mat.roughness, 0.001);

    payload.color = mat.albedo;
}