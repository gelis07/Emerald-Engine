#version 460
#extension GL_EXT_ray_tracing : require
const float pi = 3.14159265359;


struct Ray
{
    vec3 pos;
    vec3 dir;
};


struct Payload
{
    Ray newRay;
    vec3 sampleInfo;
    vec3 colorInfo;
    int sampleIdx;
};


layout(location = 0) rayPayloadInEXT Payload payload; // Must be rayPayloadInEXT
layout(binding = 7, set = 0) uniform sampler2D skybox;

void main()
{
    vec3 dir = payload.newRay.dir;

    //float a = 0.5*(dir.y + 1);
    //vec3 sky = vec3(0.87, 0.97, 1.0);

    float theta = atan(dir.z, dir.x);
    float phi =  asin(dir.y);

    float u = (theta + pi) / (2.0 * pi);
    float v = (phi + pi * 0.5) / pi;

    vec3 sky = texture(skybox, vec2(u,v)).rgb;
    sky = min(sky, vec3(10.0));
    payload.sampleInfo = payload.colorInfo * sky;

    payload.newRay.dir = vec3(0.0,0.0,0.0);
}