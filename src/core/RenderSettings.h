#pragma once 
#include <core/Scene.h>

struct RenderSettings
{
    Scene scene;
    bool EnvLight = true;
    int ImgWidth, ImgHeight;
    bool ReloadScene = false;
    bool Render = false;
    int spp;
    int imageOut;
    int frameIdx;

    uint32_t animFrame = 0;
    bool playAnimation = false;
};