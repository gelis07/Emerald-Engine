struct Payload
{
    uint seed;
    vec3 rayDir;
    vec3 rayPos;

    float prevBrdfPdf;
    vec3 throughput;
    vec3 color;
};
const float EPSILON = 0.001;
