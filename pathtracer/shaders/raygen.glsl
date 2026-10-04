#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 0, rgba8) uniform image2D renderTarget;
layout(binding = 1) uniform accelerationStructureEXT accStruct;

layout(binding = 2, std140) uniform Camera {
    vec3 pos;
    mat4 invProj;
    mat4 invView;
} cam;

layout(location = 0) rayPayloadEXT vec3 color;
const float MAX_RAY_COLLISION_DISTANCE = 10000.0f;
void main()
{
    ivec2 pixelCoord = ivec2(gl_LaunchIDEXT.xy);
    vec2 pixelCenter = vec2(pixelCoord) + vec2(0.5);
    vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 target = cam.invProj * vec4(d.x, d.y, 1.0, 1.0);
    vec4 rayDir = cam.invView * vec4(normalize(target.xyz / target.w), 0.0);
    traceRayEXT(accStruct, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, cam.pos, 0.001f, rayDir.xyz, MAX_RAY_COLLISION_DISTANCE, 0);

    imageStore(renderTarget, pixelCoord, vec4(color, 1.0));
}