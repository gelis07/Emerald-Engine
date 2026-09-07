#pragma once
#include <core/Utils.h>

const uint32_t MAX_SAMPLES_SHADER = 10;


struct floatAnimation
{
    float* value;
    std::function<float(float)> f;
};


class Animator
{
    public:
        void run(uint32_t frame);
        uint32_t animationFrame = 0;
        uint32_t renderFramesPerAnimFrame = 1000;
        void createLinearFloatAnim(float* value, float vMax, float tMax);
    private:
        std::vector<floatAnimation> floatAnims;
        float timeStep = 24;
};