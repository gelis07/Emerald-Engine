#version 460
#extension GL_EXT_ray_tracing : require

#include "payload.glsl"

const uint MAX_BOUNCES = 4;

layout(binding = 0, rgba8) uniform image2D renderTarget;
layout(binding = 1) uniform accelerationStructureEXT accStruct;

layout(binding = 2, std140) uniform Camera {
    vec3 pos;
    mat4 invProj;
    mat4 invView;
} cam;

layout(location = 0) rayPayloadEXT Payload payload;
const float MAX_RAY_COLLISION_DISTANCE = 10000.0f;
void main()
{
    ivec2 pixelCoord = ivec2(gl_LaunchIDEXT.xy);
    vec2 pixelCenter = vec2(pixelCoord) + vec2(0.5);
    vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 target = cam.invProj * vec4(d.x, d.y, 1.0, 1.0);
    vec4 rayDir = cam.invView * vec4(normalize(target.xyz / target.w), 0.0);

    payload.throughput = vec3(1.0);
    payload.color = vec3(0.0);

    payload.seed =
    uint(pixelCoord.x) * 1973u +
    uint(pixelCoord.y) * 9277u;

    payload.rayPos = cam.pos;
    payload.rayDir = rayDir.xyz;
    for(int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        //Having rayDir = vec3(0.0) as a termination value.
        if(payload.rayDir == vec3(0.0))
            break;

        traceRayEXT(accStruct, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, payload.rayPos, 0.001f, payload.rayDir, MAX_RAY_COLLISION_DISTANCE, 0);
    }


    imageStore(renderTarget, pixelCoord, vec4(payload.color, 1.0));
}