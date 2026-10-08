uint PCGHash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed)
{
    seed = PCGHash(seed);
    return (float(seed) + 0.5) / 4294967296.0;
}

vec3 RandomVec3(inout uint seed)
{
    return vec3(
    RandomFloat(seed) * 2.0 - 1.0,
    RandomFloat(seed) * 2.0 - 1.0,
    RandomFloat(seed) * 2.0 - 1.0);
}

vec3 RandomOnHemisphere(inout uint seed, vec3 normal)
{
    vec3 vector = RandomVec3(seed);
    if(dot(vector, normal) > 0)
    {
        return vector;
    }else
    {
        return -vector;
    }
}

vec3 SampleSquare(inout uint seed)
{
    return vec3(RandomFloat(seed) - 0.5,RandomFloat(seed) - 0.5,0.0); 
}