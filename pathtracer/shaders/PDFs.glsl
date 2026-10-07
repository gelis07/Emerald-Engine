float CosineSamplingPdf(vec3 n, vec3 w)
{
    return abs(dot(n, w)) / pi;
}

float BRDFSamplingPdf(vec3 wo, vec3 wm, vec3 n, float a)
{
    float nDotWm = max(dot(wm, n), 0.001);
    float cosWoWm = max(dot(wo, wm), 0.001);
    float vndf = (G1(wo, n ,a) / nDotWm) * GGXNDF(n, wm, a);

    return max(vndf / (4.0 * cosWoWm), 0.001);
}