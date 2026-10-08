const float pi = 3.14159265359;


struct Material
{
    vec3 albedo;
    float metalness;
    float roughness;
    vec3 emission;
};

float GGXNDF(vec3 n, vec3 h, float alpha)
{
    float NoH = max(dot(n, h), 0.0);

    float alpha2 = alpha * alpha;
    float denom = NoH * NoH * (alpha2 - 1.0) + 1.0;

    return alpha2 / (pi * denom * denom);
}
vec3 Fresnel(vec3 wi, vec3 wm, vec3 f0)
{
    return f0 + (1-f0) * pow(1.0 -  dot(wi, wm), 5.0);
}
float lambda(vec3 w, vec3 n, float a)
{
    float costheta = clamp(dot(w, n), 0.0001, 1.0);
    float costheta2 = costheta * costheta;
    float tantheta2 = -1.0 + 1.0 / costheta2;
    return 0.5 * (sqrt(1.0 + a*a*tantheta2) - 1.0);
}

float G1(vec3 w, vec3 n, float a)
{
    return ( 1.0 / (1.0 + lambda(w, n, a)));
}
float G(vec3 wo, vec3 wi, vec3 n, float a)
{
    return (1.0 / (1.0 + lambda(wo,n, a) + lambda(wi,n, a)));
}
vec3 bsdfEvaluation(vec3 wm, vec3 wo, vec3 wi
, vec3 n, float a, float metalness, vec3 albedo)
{
    float nDotWm = max(dot(wm,n), 0.0001);
    float nDotWo = max(dot(wo,n), 0.0001);
    float nDotWi = max(dot(wi, n), 0.0001);

    vec3 fBase = (albedo/ pi);
    vec3 fDiffuse = fBase;

    vec3 f0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = Fresnel(wi, wm, f0);
    vec3 specularBrdf = vec3(0.0);
    specularBrdf = GGXNDF(n, wm, a) * F * G(wo, wi, n, a) / (4.0 * nDotWi*nDotWo);
    
    vec3 bsdf = (1.0 - metalness) * fDiffuse + specularBrdf;

    return bsdf;
}
