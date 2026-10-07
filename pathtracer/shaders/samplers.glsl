#include "random.glsl"

vec3 VNDFSampling(inout uint seed, vec3 wo, vec3 N, float a)
{
    vec3 z = vec3(0,0,1);

    vec3 an = (abs(N.x) > 0.9) ? vec3(0.0, 1.0, 0.0) : vec3(1, 0, 0);
    vec3 B = normalize(cross(N, an));
    vec3 T = cross(N, B);

    mat3 localToWorld = mat3(T,B,N);
    mat3 worldToLocal = transpose(localToWorld);

    vec3 view = worldToLocal * wo;

    float u1 = RandomFloat(seed);
    float u2 = RandomFloat(seed);

    mat3 A = mat3(0.0);
    A[0][0] = a;
    A[1][1] = a;
    A[2][2] = 1;

    vec3 Vh = A * view;
    Vh = normalize(Vh);

    
    vec3 T1 = (Vh.z < 0.99999) ? normalize(cross(z, Vh))
                                    : z;
    vec3 T2 = cross(Vh, T1);

    float r = sqrt(u1);
    float phi = 2.0 * pi * u2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = (1.0 + Vh.z) * 0.5;
    t2 = (1.0 - s) * sqrt(1.0 - t1*t1) + s * t2;

    float uh = sqrt(max(0.0,1 - t1*t1 - t2*t2));

    vec3 Nh = t1 * T1 + t2 * T2 + uh*Vh;

    vec3 wm = normalize(A * Nh);
    return localToWorld * wm;
}

vec3 CosineSampling(inout uint seed, vec3 n)
{
    float u = RandomFloat(seed);
    float v = RandomFloat(seed);

    float phi = 2.0 * pi * u;
    float sinTheta = sqrt(v);
    float cosTheta = sqrt(1 - v);

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;

    vec3 axis[3];
    axis[2] = normalize(n);
    vec3 an = (abs(axis[2].x) > 0.9) ? vec3(0.0, 1.0, 0.0) : vec3(1, 0, 0);
    axis[1] = normalize(cross(axis[2], an));
    axis[0] = cross(axis[2], axis[1]);

    return axis[0] * x + axis[1] * y + axis[2] * z; 
}