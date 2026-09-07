#include "Animator.h"


void Animator::run(uint32_t frame)
{
    animationFrame = frame * MAX_SAMPLES_SHADER >= renderFramesPerAnimFrame ? animationFrame + 1 : animationFrame;
    for(int i = 0; i < floatAnims.size(); i++)
    {
        *floatAnims[i].value  = floatAnims[i].f((float)animationFrame / timeStep);
    }
}

void Animator::createLinearFloatAnim(float* value, float vMax,float tMax)
{
    floatAnimation anim;
    anim.f = [tMax, vMax](float time) { return (time * vMax)/tMax; };
    anim.value = value;
    floatAnims.push_back(anim);
}