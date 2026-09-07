#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_query : require


const uint UINT_MAX = 0xFFFFFFFFu;

struct Ray
{
    vec3 pos;
    vec3 dir;
};
struct Vertex {
    vec3 pos;
    vec2 uv;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;

    uint boneOffset;
    uint boneCount;
};
struct Payload
{
    Ray newRay;
    vec3 sampleInfo;
    vec3 colorInfo;
    uint bounce;
    uint sampleIdx;
    float prevBrdfPdf;
};


layout(location = 0) rayPayloadInEXT Payload payload; // Must be rayPayloadInEXT
hitAttributeEXT vec2 attribs;


layout(binding = 1) uniform accelerationStructureEXT accStruct; 

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
    float metalness;
    float idr;
    float transmittance;
    float subsurface;


    uint albedoMap;
    uint roughnessMap;
    uint metalicnessMap;
    uint normalMap;
};


layout(binding = 2, std140) uniform Camera {
    vec3 pos;
    uint lightCount;
    int frameIdx;
    mat4 invProj;
    mat4 invView;

    float skyboxProb;
    float totalSkyboxPower;
    bool skybox;
} cam;



struct ModelData
{
    mat4 modelMatrix;
};
layout(scalar, binding = 12, set = 0) buffer ModelDataBuffer{
    ModelData data[];
} modelData;


struct MeshData
{
    uint globalIdx;
    uint modelIdx;
    uint matIdx;
    mat4 localTransform;
};
layout(scalar, binding = 13, set = 0) buffer MeshDataBuffer{
    MeshData data[];
} meshData;

layout(scalar, binding = 14, set = 0) buffer MaterialDataBuffer
{
    Material data[];
} materialData;

struct walkersAlias
{
    float prob;
    uint startIdx;
    uint aliasIdx;
};

//! UNLESS MODEL IDX IS TAKEN FROM THE MESH DATA, ITS REFERING TO THE MESH IDX GLOBABLLY.

struct triPickingData
{
    float prob;
};
struct MeshLight
{
    float prob;
    uint triCount;
    uint modelIdx;
};
layout(binding = 8, set = 0) buffer WalkersAliasLights
{
    walkersAlias bucket[];
} walkersAliasMeshLights;

layout(binding = 9, set = 0) buffer WalkersAliasMeshTriangles
{
    walkersAlias bucket[];
} walkersAliasMeshTriangles[];

layout(binding = 10, set = 0) buffer Lights
{
    MeshLight meshLights[];
} lights;

layout(binding = 11, set = 0) buffer TriangleProbs
{
    triPickingData triPickerData[];
} triangleProbs[];

layout(binding = 7, set = 0) uniform sampler2D skybox;

layout(binding = 15, set = 0) buffer WalkersAliasSkybox
{
    walkersAlias bucket[];
} walkersAliasSkybox;


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
    float costheta = clamp(dot(w, n), 0.0001, 1.0);
    float costheta2 = costheta * costheta;
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
    float cosThetaM = max(dot(n, wm), 0.0001);
    float cosWoWm = max(dot(wo, wm), 0.0001);

    return GGXNDF(n, wm, a) * (cosThetaM) / (4.0 * cosWoWm);
}

float areaPdfToSAng(float areaProb, vec3 lightDir, vec3 lightNormal, float dist)
{
    return areaProb * dist * dist / max(abs(dot(lightNormal, lightDir)), 0.0001);
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



struct triPick
{
    uint modelIdx;
    uint triIdx;
    
    float triProb;
    float modelProb;
};



triPick pickLightSamplingTri(inout uint seed)
{
    triPick tri;

    float u1 = RandomFloat(seed);
    float x1 = u1 * cam.lightCount;
    uint i1 = uint(x1);
    float v1 = x1 - i1;

    uint lightIdx;
    if(walkersAliasMeshLights.bucket[nonuniformEXT(i1)].aliasIdx != UINT_MAX)
    {
        lightIdx = (v1 < walkersAliasMeshLights.bucket[nonuniformEXT(i1)].prob) 
        ? walkersAliasMeshLights.bucket[nonuniformEXT(i1)].startIdx 
        : walkersAliasMeshLights.bucket[nonuniformEXT(i1)].aliasIdx;
    }else
    {
        lightIdx = walkersAliasMeshLights.bucket[nonuniformEXT(i1)].startIdx;
    }
    tri.modelIdx = lights.meshLights[lightIdx].modelIdx;
    float u2 = RandomFloat(seed);
    float x2 = u2 * lights.meshLights[lightIdx].triCount;
    uint i2 = uint(x2);
    float v2 = x2 - i2;
    if(walkersAliasMeshTriangles[nonuniformEXT(lightIdx)].bucket[nonuniformEXT(i2)].aliasIdx != UINT_MAX)
    {
        tri.triIdx = (v2 < walkersAliasMeshTriangles[nonuniformEXT(lightIdx)].bucket[nonuniformEXT(i2)].prob) 
        ? walkersAliasMeshTriangles[nonuniformEXT(lightIdx)].bucket[nonuniformEXT(i2)].startIdx 
        : walkersAliasMeshTriangles[nonuniformEXT(lightIdx)].bucket[nonuniformEXT(i2)].aliasIdx;
    }
    else{
        tri.triIdx = walkersAliasMeshTriangles[nonuniformEXT(lightIdx)].bucket[nonuniformEXT(i2)].startIdx;
    }

    tri.triProb = triangleProbs[nonuniformEXT(lightIdx)].triPickerData[nonuniformEXT(tri.triIdx)].prob;
    tri.modelProb = lights.meshLights[nonuniformEXT(lightIdx)].prob;
    return tri;
}


struct triInfo
{
    vec3 normal;
    vec3 pos;
    float area;
};

struct trianglePos
{
    vec3 a;
    vec3 b;
    vec3 c;
};

trianglePos getTriangleFromIdx(uint modelIdx, uint triIdx)
{
    trianglePos tri;
    uint lightIndexOffset = triIdx * 3;
    uint i0 = indexBuffers[nonuniformEXT(modelIdx)].i[lightIndexOffset + 0];
    uint i1 = indexBuffers[nonuniformEXT(modelIdx)].i[lightIndexOffset + 1];
    uint i2 = indexBuffers[nonuniformEXT(modelIdx)].i[lightIndexOffset + 2];

    vec3 v0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].pos.xyz;
    vec3 v1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].pos.xyz;
    vec3 v2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].pos.xyz;

    tri.a = vec3(modelData.data[meshData.data[nonuniformEXT(modelIdx)].modelIdx].modelMatrix * meshData.data[nonuniformEXT(modelIdx)].localTransform * vec4(v0, 1.0));
    tri.b = vec3(modelData.data[meshData.data[nonuniformEXT(modelIdx)].modelIdx].modelMatrix * meshData.data[nonuniformEXT(modelIdx)].localTransform * vec4(v1, 1.0));
    tri.c = vec3(modelData.data[meshData.data[nonuniformEXT(modelIdx)].modelIdx].modelMatrix * meshData.data[nonuniformEXT(modelIdx)].localTransform * vec4(v2, 1.0));

    return tri;
}

float calcTriArea(trianglePos tri)
{
    return 0.5 * length(cross(tri.b - tri.a, tri.c - tri.a));
}


triInfo pickRandomPointOnTri(triPick tri, inout uint seed)
{
    triInfo info;

    trianglePos triPos = getTriangleFromIdx(tri.modelIdx, tri.triIdx);
    uint lightIndexOffset = tri.triIdx * 3;
    uint i0 = indexBuffers[nonuniformEXT(tri.modelIdx)].i[lightIndexOffset + 0];
    uint i1 = indexBuffers[nonuniformEXT(tri.modelIdx)].i[lightIndexOffset + 1];
    uint i2 = indexBuffers[nonuniformEXT(tri.modelIdx)].i[lightIndexOffset + 2];

    vec3 n0 = vertexBuffers[nonuniformEXT(tri.modelIdx)].v[i0].normal.xyz;
    vec3 n1 = vertexBuffers[nonuniformEXT(tri.modelIdx)].v[i1].normal.xyz;
    vec3 n2 = vertexBuffers[nonuniformEXT(tri.modelIdx)].v[i2].normal.xyz;

    float u1 = sqrt(RandomFloat(seed));
    float u2 = RandomFloat(seed);

    float a = 1 - u1;
    float b = u2*u1;
    float c = u1*(1-u2);

    info.pos = a * triPos.a + b * triPos.b + c*triPos.c;
    vec3 objectNormal = a * n0 + b * n1 + c * n2;

    mat4 lightMat = modelData.data[meshData.data[nonuniformEXT(tri.modelIdx)].modelIdx].modelMatrix * meshData.data[nonuniformEXT(tri.modelIdx)].localTransform;
    vec3 worldNormal = transpose(inverse(mat3(lightMat))) * objectNormal;

    info.normal = normalize(worldNormal);
    info.area = calcTriArea(triPos);

    return info;
}





vec3 bsdfEvaluation(vec3 f0, vec3 wm, vec3 wo, vec3 wi
, vec3 n, float a, float metalness, vec3 albedo, float subsurface)
{
    float nDotWm = max(dot(wm,n), 0.0001);
    float nDotWo = max(dot(wo,n), 0.0001);
    float nDotWi = max(dot(wi, n), 0.0001);

    vec3 fBase = (albedo/ pi);
    vec3 fDiffuse = fBase;

    vec3 F = Fresnel(wi, wm, f0);
    vec3 specularBrdf = GGXNDF(n, wm, a) * F * G(wo, wi, n, a) / (4.0 * nDotWi*nDotWo);

    vec3 bsdf = (1.0 - metalness) * fDiffuse + metalness * specularBrdf;
    return bsdf;
}



struct samplingSkyboxInfo 
{
    vec3 dir;
    ivec2 uvs;
    float phi;
};

//Importance sampling the skybox.
samplingSkyboxInfo ImpSampleSkybox(inout uint seed)
{
    ivec2 size = textureSize(skybox, 0);
    float R = RandomFloat(seed);
    float x = R * size.x * size.y;
    uint i = uint(x); 
    float rem = x - i;

    uint pixelIdx;
    if(walkersAliasSkybox.bucket[i].aliasIdx != UINT_MAX)
    {
        pixelIdx = (rem < walkersAliasSkybox.bucket[nonuniformEXT(i)].prob) 
        ? walkersAliasSkybox.bucket[nonuniformEXT(i)].startIdx 
        : walkersAliasSkybox.bucket[nonuniformEXT(i)].aliasIdx;
    }else
    {
        pixelIdx = walkersAliasSkybox.bucket[nonuniformEXT(i)].startIdx;
    }


    uint u = uint(floor(pixelIdx / size.x));
    uint v = pixelIdx - u * size.x;

    float theta = 2.0 * pi * u - pi;
    float phi = pi*v - pi*0.5;

    samplingSkyboxInfo info;
    info.dir.x = cos(phi) * cos(theta);
    info.dir.y = sin(phi);
    info.dir.z = cos(phi) * sin(theta);
    info.dir = normalize(info.dir);
    info.uvs = ivec2(u,v);
    info.phi = phi;
    return info;
}




vec3 NEEFromSkybox(inout uint seed, vec3 rayPos, float metalness, vec3 f0
, vec3 n, float a, vec3 throughput, vec3 wo, vec3 albedo, float subsurface)
{
    rayQueryEXT shadowRayQuery;
    samplingSkyboxInfo skyboxInfo = ImpSampleSkybox(seed);
    vec3 wi = skyboxInfo.dir;
    vec3 wm = normalize(wi+wo);
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    rayQueryInitializeEXT(shadowRayQuery, accStruct, flags, 0xFF, rayPos, 0.001, wi, 1e24);
    while(rayQueryProceedEXT(shadowRayQuery)) {}
    if(rayQueryGetIntersectionTypeEXT(shadowRayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
    {
        ivec2 size = textureSize(skybox, 0);
        vec3 skyColor = texture(skybox, vec2(skyboxInfo.uvs.x, skyboxInfo.uvs.y)).rgb;
        float pixelProb = skyColor.x / cam.totalSkyboxPower;
        float probToWi = cam.skyboxProb * pixelProb * size.x * size.y / (2.0 * pi * pi * cos(skyboxInfo.phi));
        float brdfPdf = TorSpPdf(wo, wm, n, a);
        float nDotWi = max(dot(wi, n), 0.0001);
        vec3 bsdf = bsdfEvaluation(f0, wm, wo, wi, n, a, metalness, albedo, subsurface);

        vec3 directLight = skyColor * bsdf * nDotWi * throughput / (probToWi + brdfPdf);

        return directLight;
    }

    return vec3(0.0);
}


vec3 NEEFromSceneLight(inout uint seed, vec3 rayPos, float metalness, vec3 f0
, vec3 n, float a, vec3 throughput, vec3 wo, vec3 albedo, float subsurface)
{
    rayQueryEXT shadowRayQuery;

    //picking a direction based on picked triangle of a light source.
    triPick lightTri = pickLightSamplingTri(seed);
    triInfo lightTriInfo = pickRandomPointOnTri(lightTri, seed);
    vec3 randomLightPos = lightTriInfo.pos;
    vec3 lightDir = normalize(randomLightPos - rayPos);
    float lightDist = length(randomLightPos - rayPos);
    vec3 wi = lightDir;
    vec3 wm = normalize(wi+wo);

    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    rayQueryInitializeEXT(shadowRayQuery, accStruct, flags, 0xFF, rayPos, 0.001, lightDir, lightDist - 0.001);

    while(rayQueryProceedEXT(shadowRayQuery)) {}

    float dist = 0;
    Material shadowHitMat;
    if(rayQueryGetIntersectionTypeEXT(shadowRayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
    {
        uint instanceID = rayQueryGetIntersectionInstanceIdEXT(shadowRayQuery, true);
        uint primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(shadowRayQuery, true);

        shadowHitMat = materialData.data[meshData.data[nonuniformEXT(lightTri.modelIdx)].matIdx];
        //It hit a light.
        if(dot(wi, n) > 0.0 && dot(lightTriInfo.normal, lightDir) > 0.0)
        {
            float lightPdf = areaPdfToSAng((1.0 - cam.skyboxProb) * lightTri.modelProb * lightTri.triProb * 1.0/lightTriInfo.area, lightDir, lightTriInfo.normal, lightDist);
            float brdfPdf = TorSpPdf(wo, wm, n, a);
            float nDotWi = max(dot(wi, n), 0.0001);
            vec3 bsdf = bsdfEvaluation(f0, wm, wo, wi, n, a, metalness, albedo, subsurface);
            vec3 directLight = shadowHitMat.emmColor * bsdf * nDotWi * throughput / (lightPdf + brdfPdf);

            return directLight;
        }
    }
    return vec3(0.0);
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

    vec3 t0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].tangent.xyz;
    vec3 t1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].tangent.xyz;
    vec3 t2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].tangent.xyz;

    vec3 b0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].bitangent.xyz;
    vec3 b1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].bitangent.xyz;
    vec3 b2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].bitangent.xyz;

    vec2 texCoord0 = vertexBuffers[nonuniformEXT(modelIdx)].v[i0].uv.xy;
    vec2 texCoord1 = vertexBuffers[nonuniformEXT(modelIdx)].v[i1].uv.xy;
    vec2 texCoord2 = vertexBuffers[nonuniformEXT(modelIdx)].v[i2].uv.xy;

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 objectNormal = barycentrics.x * n0 + barycentrics.y * n1 + barycentrics.z * n2;
    vec3 objectTangent = barycentrics.x * t0 + barycentrics.y * t1 + barycentrics.z * t2;
    vec3 objectBitangent = barycentrics.x * b0 + barycentrics.y * b1 + barycentrics.z * b2;
    
    vec3 N = normalize(objectNormal);

    vec3 T = normalize(
        objectTangent - N * dot(N, objectTangent)
    );

    vec3 B = normalize(cross(N, T));

    vec3 finalNormal = objectNormal;

    MeshData mesh = meshData.data[nonuniformEXT(modelIdx)];
    Material mat = materialData.data[mesh.matIdx];
    if(mat.normalMap != -1)
    {
        vec2 uvHit = barycentrics.x * texCoord0 + barycentrics.y * texCoord1 + barycentrics.z * texCoord2;
        vec3 tangentNormal = textureLod(texImage[nonuniformEXT(mat.normalMap)], uvHit, 0.0).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0;

        finalNormal = normalize(T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z);
    }


    vec3 worldNormal =
        normalize(
            transpose(mat3(gl_WorldToObjectEXT)) * finalNormal
        );
    
    float a = mat.roughness * mat.roughness;

    if(mat.roughnessMap != -1)
    {
        vec2 uvHit = barycentrics.x * texCoord0 + barycentrics.y * texCoord1 + barycentrics.z * texCoord2;
        a = textureLod(texImage[nonuniformEXT(mat.roughnessMap)], uvHit, 0.0).r;
        a *= a;
    }

    bool frontFace = dot(direction, worldNormal) < 0.0;
    vec3 n = frontFace ? worldNormal : -worldNormal;


    
    float t = gl_HitTEXT;
    payload.newRay.pos = origin + direction * t + n * 0.001;
    vec3 wo = normalize(-direction);
    vec3 BRDFWm = RandomWm(payload.sampleIdx, n, a);
    vec3 BRDFWi = -wo + 2.0 * dot(wo, BRDFWm) * BRDFWm;


    if(any(notEqual(mat.emmColor, vec3(0.0))))
    {
        //BRDF sampler hit the light
        if(payload.bounce == 0)
        {
            payload.sampleInfo += payload.colorInfo * mat.emmColor;
        }
        else
        {
            uint lightIdx = -1;
            for(int i = 0; i < cam.lightCount; i++)
            {
                if(modelIdx == lights.meshLights[i].modelIdx)
                {
                    lightIdx = i;
                    break;
                }
            }
            if(lightIdx == -1)
                return;
                
            float modelProb = lights.meshLights[nonuniformEXT(lightIdx)].prob;

            trianglePos triPos = getTriangleFromIdx(modelIdx, gl_PrimitiveID);
            float area = calcTriArea(triPos);
            triPickingData triProb = triangleProbs[nonuniformEXT(lightIdx)].triPickerData[nonuniformEXT(gl_PrimitiveID)];

            float lightPdf = areaPdfToSAng((1-cam.skyboxProb)*modelProb * triProb.prob * 1.0/area, wo, worldNormal, t);
            float brdfPdf = payload.prevBrdfPdf;

            float misWeight = brdfPdf / (brdfPdf + lightPdf);
            payload.sampleInfo += payload.colorInfo * mat.emmColor * misWeight;
        }
        payload.newRay.dir = vec3(0.0);
        return;
    }


    if (dot(n, BRDFWi) <= 0.0 || dot(wo, BRDFWm) <= 0.0) {
        payload.newRay.dir = vec3(0.0);
        return;
    }

    vec3 albedo = mat.albedo;
    if(mat.albedoMap != -1)
    {
        vec2 uvHit = barycentrics.x * texCoord0 + barycentrics.y * texCoord1 + barycentrics.z * texCoord2;
        albedo = textureLod(texImage[nonuniformEXT(mat.albedoMap)], uvHit, 0.0).rgb;
    }
    float metalness = mat.metalness;
    if(mat.metalicnessMap != -1)
    {
        vec2 uvHit = barycentrics.x * texCoord0 + barycentrics.y * texCoord1 + barycentrics.z * texCoord2;
        metalness = textureLod(texImage[nonuniformEXT(mat.metalicnessMap)], uvHit, 0.0).r;
    }


    vec3 f0 = mix(vec3(0.04), albedo, metalness);
    
    float chooseSampler = 1;
    if(false)
        chooseSampler = RandomFloat(payload.sampleIdx);

    if(chooseSampler < cam.skyboxProb)
    {
        payload.sampleInfo += NEEFromSkybox(payload.sampleIdx, payload.newRay.pos, metalness, f0, n, a, payload.colorInfo, wo, albedo, mat.subsurface);
    }else
    {   
        if(cam.lightCount != 0)
            payload.sampleInfo += NEEFromSceneLight(payload.sampleIdx, payload.newRay.pos, metalness, f0, n, a, payload.colorInfo, wo, albedo, mat.subsurface);
    }

    payload.newRay.dir = BRDFWi;
    payload.prevBrdfPdf = TorSpPdf(wo, BRDFWm, n, a);
    float nDotWi = max(dot(BRDFWi, n), 0.0001);

    vec3 bsdf = bsdfEvaluation(f0, BRDFWm, wo, BRDFWi, n, a, metalness, albedo, mat.subsurface);

    payload.colorInfo *= bsdf * nDotWi / payload.prevBrdfPdf;
}