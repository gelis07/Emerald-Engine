#version 460
#extension GL_EXT_ray_tracing : require
const float pi = 3.14159265359;


struct Ray
{
    vec3 pos;
    vec3 dir;
};


struct Material
{
    vec3 albedo;
    vec3 emmColor;
    float roughness;

    uint albedoMap;
    uint roughnessMap;
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
layout(binding = 7, set = 0) uniform sampler2D skybox;
layout(binding = 2, std140) uniform Camera {
    vec3 pos;
    uint lightCount;
    int frameIdx;
    mat4 invProj;
    mat4 invView;

    float skyboxProb;
    float totalSkyboxPower;
    bool skybox;

    float intervalLength;
} cam;

void main()
{
    vec3 dir = payload.newRay.dir;

    float theta = acos(dir.y);
    float phi = atan(dir.z, dir.x);

    float u = phi / (2.0 * pi) + 0.5;
    float v = theta / pi;

    vec3 sky = texture(skybox, vec2(u,v)).rgb;

    payload.newRay.dir = vec3(0.0,0.0,0.0);

    if(!cam.skybox)
        return;

    if(payload.bounce == 0)
    {
        payload.sampleInfo = payload.colorInfo * sky;
    }
    else
    {
        ivec2 size = textureSize(skybox, 0);
        float pixelProb = sky.x / cam.totalSkyboxPower;
        float brdfPdf = payload.prevBrdfPdf;
        float probToWi = cam.skyboxProb * pixelProb * size.x * size.y / (2.0 * pi * pi * max(sin(theta), 0.001));
        // payload.sampleInfo += payload.colorInfo * sky * 0.5;
        if(abs(probToWi + brdfPdf) > 0.001)
            payload.sampleInfo += payload.colorInfo * sky * brdfPdf / (probToWi + brdfPdf);
    }

}