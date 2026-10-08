#version 460
#extension GL_EXT_ray_tracing : require


#include "payload.glsl"
layout(location = 0) rayPayloadInEXT Payload payload;
#include "integrator.glsl"


void main()
{
    float a = 0.5*(payload.rayDir.y + 1);  
    vec3 sky = (1-a)*vec3(1) + a * vec3(0.5, 0.7, 1.0);
    // BRDFIntegrator(sky);

    payload.rayDir = vec3(0.0);
}