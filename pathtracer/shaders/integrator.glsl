#extension GL_EXT_ray_query : require

vec3 BRDFIntegrator(vec3 wo,vec3 n, float distSqr, vec3 emission)
{
    float pdfLight = LightSamplingPdf(n, wo, 0.2116, distSqr);
    float pdfBrdf = payload.prevBrdfPdf;
    float MISWeight = pdfBrdf / (pdfBrdf + pdfLight);
    return payload.throughput * vec3(20.0) * MISWeight;
}

//Next event estimation
vec3 LightIntegrator(vec3 wo ,vec3 n,float a, float metalness, vec3 albedo)
{
    rayQueryEXT shadowRayQuery;
    lightSample lightsample = lightSampler(payload.seed, payload.rayPos);
    vec3 wi = lightsample.dir;
    float distanceSqr = lightsample.distanceSqr;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    rayQueryInitializeEXT(shadowRayQuery, accStruct, flags, 0xFF, payload.rayPos, 0.001, wi, sqrt(distanceSqr) - EPSILON);
    while(rayQueryProceedEXT(shadowRayQuery)) {}

    //hit the light.
    if(rayQueryGetIntersectionTypeEXT(shadowRayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
    {
        vec3 wm = normalize(wo + wi);

        float pdfLight = LightSamplingPdf(vec3(0,1,0), wi, 0.2116, distanceSqr);
        float pdfBrdf = BRDFSamplingPdf(wo, wm, n,a);

        vec3 bsdf = bsdfEvaluation(wm, wo, wi, n, a, metalness, albedo);
        vec3 emission = vec3(20.0);
        float nDotWi = max(dot(wi, n), 0.0);
        return payload.throughput * bsdf * nDotWi * emission / (pdfLight + pdfBrdf);
    }

    return vec3(0.0);
}
