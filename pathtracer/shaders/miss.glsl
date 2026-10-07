#version 460
#extension GL_EXT_ray_tracing : require


#include "payload.glsl"
layout(location = 0) rayPayloadInEXT Payload payload;


void main()
{
    payload.color = payload.throughput * vec3(0.45, 0.61, 1.0);
}