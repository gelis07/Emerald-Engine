void BRDFIntegrator(vec3 emission)
{
    //BRDF sampler hit the light
    payload.color += payload.throughput * emission;
    payload.rayDir = vec3(0.0);
}