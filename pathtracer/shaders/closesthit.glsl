#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable


#include "payload.glsl"
#include "materials.glsl"
#include "samplers.glsl"
#include "PDFs.glsl"

const float EPSILON = 0.001;

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

layout(scalar, binding = 3, set = 0) buffer IndexBuffer
{
    uint i[];
} indexBuffer[];
layout(scalar, binding = 4, set = 0) buffer VertexBuffer
{
    Vertex v[];
} vertexBuffer[];
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
    uint startIdx = gl_PrimitiveID * 3;
    uint i0 = indexBuffer[nonuniformEXT(meshId)].i[startIdx + 0];
    uint i1 = indexBuffer[nonuniformEXT(meshId)].i[startIdx + 1];
    uint i2 = indexBuffer[nonuniformEXT(meshId)].i[startIdx + 2];

    float t = gl_HitTEXT;
    
    vec3 barrycentric = vec3(
        1.0 - attrib.x - attrib.y,
        attrib.x,
        attrib.y
    );
    //Normal in vertex coordinates.
    vec3 VNormal = vertexBuffer[nonuniformEXT(meshId)].v[i0].normals * barrycentric.x
    + vertexBuffer[nonuniformEXT(meshId)].v[i1].normals * barrycentric.y
    + vertexBuffer[nonuniformEXT(meshId)].v[i2].normals * barrycentric.z;

    vec3 worldNormal = normalize(
        inverse(transpose(mat3(gl_WorldToObjectEXT))) * VNormal
    );

    bool frontFace = dot(payload.rayDir, worldNormal) < 0.0;
    vec3 n = frontFace ? worldNormal : -worldNormal;
    Mesh mesh = meshBuffer.meshes[nonuniformEXT(meshId)];
    // Model model = modelBuffer.models[mesh.modelId];
    Material mat = materialBuffer.materials[nonuniformEXT(mesh.matId)];

    //Roughness like in the Disney model.
    float a = max(mat.roughness * mat.roughness, 0.001);

    vec3 wo = normalize(-payload.rayDir);
    vec3 wm = VNDFSampling(payload.seed, wo, n, a);
    vec3 wi = reflect(-wo, wm);
    vec3 f0 = mat.albedo;

    vec3 bsdf = bsdfEvaluation(f0, wm, wo, wi, n, a, mat.metalness, mat.albedo);
    float NdotWi = max(dot(n, wi), 0.0);
    float pdf = max(BRDFSamplingPdf(wo, wm, n, a), EPSILON);

    payload.throughput *= bsdf * NdotWi / pdf;
    payload.rayPos = payload.rayPos + payload.rayDir * t + n * EPSILON; 
    payload.rayDir = wi;
}