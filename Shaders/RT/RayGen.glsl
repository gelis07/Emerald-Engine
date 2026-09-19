#version 460
#extension GL_EXT_ray_tracing : require

const int MAX_BOUNCES = 12;
const int MAX_SAMPLES = 1;

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

layout(binding = 0, rgba32f) uniform image2D renderTarget;
layout(binding = 1) uniform accelerationStructureEXT accStruct; 
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
layout(binding = 5, rgba8) uniform image2D sumImage;



layout(location = 0) rayPayloadEXT Payload payload;
const float MAX_RAY_COLLISION_DISTANCE = 100000.0f;


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

vec3 SampleSquare(inout uint seed)
{
    return vec3(RandomFloat(seed) - 0.5,RandomFloat(seed) - 0.5,0.0); 
}

void main()
{
    // Pixel coordinate (e.g., 0 to 1279, 0 to 719)
    ivec2 pixelCoord = ivec2(gl_LaunchIDEXT.xy);

    uint seed =
    uint(pixelCoord.x) * 1973u +
    uint(pixelCoord.y) * 9277u +
    cam.frameIdx  * 26699u + 1u;

    // Normalized Device Coordinates (NDC) in range [-1.0, 1.0]
    vec3 offset = SampleSquare(seed);
    vec2 pixelCenter = vec2(pixelCoord + offset.xy) + vec2(0.5);
    vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    // Unproject to world space ray
    vec4 target = cam.invProj * vec4(d.x, d.y, 1.0, 1.0);
    vec4 rayDir = cam.invView * vec4(normalize(target.xyz / target.w), 0.0);\

    vec3 finalColor = vec3(0.0);
    vec4 prev = vec4(0.0);
    if(cam.frameIdx != 0)
    {
        prev = imageLoad(renderTarget, pixelCoord);
    }else
    {
        imageStore(renderTarget, pixelCoord, vec4(0.0));
    }
    for(int s = 0; s < MAX_SAMPLES; s++)
    {
        uint idx = (cam.frameIdx * MAX_SAMPLES) + s;

        uint seed =
        uint(pixelCoord.x) * 1973u +
        uint(pixelCoord.y) * 9277u +
        idx * 26699u + 1u;

        payload.sampleIdx = seed;


        payload.colorInfo = vec3(1.0, 1.0, 1.0);
        Ray ray;
        ray.pos = cam.pos;
        ray.dir = rayDir.xyz;
        payload.newRay = ray;
        payload.sampleInfo = vec3(0.0, 0.0, 0.0);
        bool dontEvaluate = false;
        for(int bounce = 0; bounce < MAX_BOUNCES; bounce++)
        {
            payload.bounce = bounce;
            bool rr = false;
            if(bounce >= 3)
            {
                float p = max(payload.colorInfo.r, max(payload.colorInfo.g, payload.colorInfo.b));
                p = clamp(p, 0.05, 0.95);
                rr = RandomFloat(payload.sampleIdx) > p;
                if(rr)
                    payload.colorInfo /= p;
            } 

            traceRayEXT(accStruct, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, payload.newRay.pos, 0.001f, payload.newRay.dir, MAX_RAY_COLLISION_DISTANCE, 0);
            if (all(lessThan(abs(payload.newRay.dir), vec3(0.0001))) || rr)
            {
                break;
            }
        }
        finalColor += payload.sampleInfo;
    }
    vec3 colorToStore = prev.xyz + finalColor;
    imageStore(renderTarget, pixelCoord, vec4(colorToStore, 1.0));


    vec3 outputColor = colorToStore / ((cam.frameIdx + 1)* MAX_SAMPLES);
    imageStore(sumImage, pixelCoord, vec4(pow(outputColor, vec3(1.0/2.2)), 1.0));
}