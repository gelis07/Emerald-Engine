#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
struct Ray
{
    vec3 pos;
    vec3 dir;
};
struct Vertex {
    vec3 pos;
    vec2 uv;
    vec3 normal;
};
struct Payload
{
    Ray newRay;
    vec3 sampleInfo;
    vec3 colorInfo;
    uint sampleIdx;
};

layout(location = 0) rayPayloadInEXT Payload payload; // Must be rayPayloadInEXT
hitAttributeEXT vec2 attribs;

layout(scalar, binding = 4, set = 0) buffer VertexBuffers {
    Vertex v[];
} vertexBuffers[];

// Unbounded array of index buffers (one SSBO per model)
layout(scalar, binding = 3, set = 0) buffer IndexBuffers {
    uint i[];
} indexBuffers[];

layout(binding = 6, set = 0) uniform sampler2D texImage[];

struct Material
{
    vec3 albedo;
    vec3 emmColor;
    float roughness;

    uint albedoMap;
    uint roughnessMap;
};


layout(binding = 2) uniform Camera {
    vec3 pos;
    int frameIdx;
    mat4 invProj;
    mat4 invView;
    Material mat[512];
} cam;
const float pi = 3.14159265359;
const float INF = 3.402823466e+38;

uint PCGHash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed)
{
    seed = PCGHash(seed);
    return float(seed) / 4294967295.0;
}

vec3 RandomVec3(inout uint seed)
{
    return vec3(
    RandomFloat(seed) * 2.0 - 1.0,
    RandomFloat(seed) * 2.0 - 1.0,
    RandomFloat(seed) * 2.0 - 1.0);
}

vec3 RandomOnHemisphere(inout uint seed, vec3 normal)
{
    vec3 vector = RandomVec3(seed);
    if(dot(vector, normal) > 0)
    {
        return vector;
    }else
    {
        return -vector;
    }
}

float GGXNDF(vec3 n, vec3 wm, float a)
{
    float costheta2 = dot(n,wm);
    costheta2 *= costheta2;
    float tantheta2 = -1.0 + 1.0 / costheta2;
    float denom = pi * costheta2 * costheta2 * pow((a*a + tantheta2), 2.0);
    return a*a / denom; 
}
vec3 Fresnel(vec3 wi, vec3 wm, vec3 f0)
{
    return f0 + (1-f0) * pow(1.0 -  dot(wi, wm), 5.0);
}
float lambda(vec3 w, vec3 n, float a)
{
    float costheta2 = dot(w, n);
    costheta2 *= costheta2;
    float tantheta2 = -1.0 + 1.0 / costheta2;
    return 0.5 * (sqrt(1.0 + a*a*tantheta2) - 1.0);
}
vec3 RandomWm(inout uint seed, vec3 w, float a)
{
    float r1 = RandomFloat(seed);
    float r2 = RandomFloat(seed);

    float thetam = atan(a * sqrt(r1 / (1 - r1)));
    float phim = 2 * pi * r2;
    float x = sin(thetam) * cos(phim);
    float y = sin(thetam) * sin(phim);
    float z = cos(thetam);


    vec3 axis[3];
    axis[2] = normalize(w);
    vec3 an = (abs(axis[2].x) > 0.9) ? vec3(0.0, 1.0, 0.0) : vec3(1, 0, 0);
    axis[1] = normalize(cross(axis[2], an));
    axis[0] = cross(axis[2], axis[1]);

    return axis[0] * x + axis[1] * y + axis[2] * z; 

}

float TorSpPdf(vec3 wo, vec3 wm, vec3 n, float a)
{
    float cosThetaM = max(dot(n, wm), 0.0);
    float cosWoWm = max(dot(wo, wm), 0.0);

    return GGXNDF(n, wm, a) * (cosThetaM) / (4.0 * cosWoWm);
}

float G1(vec3 w, vec3 n, float a)
{
    return ( 1.0 / (1.0 + lambda(w, n, a)));
}


float G(vec3 wo, vec3 wi, vec3 n, float a)
{
    return (1.0 / (1.0 + lambda(wo,n, a) + lambda(wi,n, a)));
}

vec3 BRDFSampling(vec3 n, vec3 f0, float a, vec3 wm, vec3 wi, vec3 wo,
inout vec3 rayTraversalInfo, vec3 matEmission)
{
    vec3 Sample = vec3(0.0);
    Sample = Fresnel(wo, wm, f0) * G1(wi, n, a);
    return rayTraversalInfo *Sample * matEmission;
}



void main()
{
    vec3 origin    = payload.newRay.pos;
    vec3 direction = payload.newRay.dir;

    uint modelIdx = gl_InstanceID; 
    uint indexOffset = gl_PrimitiveID * 3;

    uint i0 = indexBuffers[nonuniformEXT(modelIdx)].i[indexOffset + 0];
    uint i1 = indexBuffers[nonuniformEXT(modelIdx)].i[indexOffset + 1];
    uint i2 = indexBuffers[nonuniformEXT(modelIdx)].i[indexOffset + 2];

    vec3 n0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].normal.xyz;
    vec3 n1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].normal.xyz;
    vec3 n2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].normal.xyz;

    vec2 texCoord0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].uv.xy;
    vec2 texCoord1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].uv.xy;
    vec2 texCoord2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].uv.xy;

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 objectNormal = barycentrics.x * n0 + barycentrics.y * n1 + barycentrics.z * n2;

    vec3 worldNormal = normalize(objectNormal * mat3(gl_WorldToObjectEXT));
    //vec3 worldNormal =
    //normalize(mat3(gl_ObjectToWorldEXT) * objectNormal);

    
    Material mat = cam.mat[nonuniformEXT(modelIdx)];
    float a = mat.roughness;
    bool frontFace = dot(direction, worldNormal) < 0.0;
    vec3 n = frontFace ? worldNormal : -worldNormal;

    


    float t = gl_HitTEXT;
    payload.newRay.pos = origin + direction * t + n * 0.001;
    vec3 wo = normalize(-direction);
    vec3 wm = RandomWm(payload.sampleIdx, n, a);
    vec3 wi = -wo + 2.0 * dot(wo, wm) * wm;

    vec3 f0 = mat.albedo;

    if(mat.albedoMap != -1)
    {
        vec2 uvHit = barycentrics.x * texCoord0 + barycentrics.y * texCoord1 + barycentrics.z * texCoord2;
        f0 = textureLod(texImage[nonuniformEXT(mat.albedoMap)], uvHit, 0.0).rgb;
    }

    vec3 brdfSample = BRDFSampling(n, f0, a, wm, wi, wo, payload.colorInfo, mat.emmColor);
    payload.sampleInfo += brdfSample;

    payload.colorInfo *= Fresnel(wi, wm, f0) * G1(wi, n, a);
    payload.newRay.dir = wi;
}