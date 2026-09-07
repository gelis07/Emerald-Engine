#version 430 core

layout(location = 0) out vec4 oColor;


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

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint objId;
} push;

void main()
{
    uint seed = push.objId * 1973u;

    vec3 color = vec3(
        RandomFloat(seed),
        RandomFloat(seed),
        RandomFloat(seed)
    );
    oColor = vec4(color, 1.0);
}     