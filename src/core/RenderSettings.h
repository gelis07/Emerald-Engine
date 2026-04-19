#pragma once 
#include <core/Scene.h>

struct RenderSettings
{
    Scene scene;
    bool accumulate = false;
    bool EnvLight = true;
    int ImgWidth, ImgHeight;
    bool ReloadScene = false;
    bool Render = false;
};