#include <pathtracer.hpp>
#include "app/defaultMeshes.hpp"
#include "app/export.hpp"
#include <chrono>
int main()
{
    core::instance instance({});
    core::device device(instance.getInstance());
    core::allocator alloc(instance.getInstance(), device.getPhysicalDevice(), device.getDevice());
    core::context context(device, alloc, instance);

    engine::Scene scene;
    engine::Model model;
    model.InitBuffer(context, CubeVertices, CubeIndices);
    model.transform = glm::mat4(1.0f);
    model.transform = glm::translate(model.transform, glm::vec3(0, -2, 0));
    scene.models.push_back(model);

    
    engine::RenderSettings settings;
    settings.ImageHeight = 1080;
    settings.ImageWidth = 1920;

    glm::mat4 projection = glm::perspectiveFov(glm::radians(45.0f), 
    (float)settings.ImageWidth, 
    (float)settings.ImageHeight,
    0.1f,
    1000.0f);

    
    scene.camera.invProj = glm::inverse(projection);
    scene.camera.pos = glm::vec3(0.0f, 0.0f, -5.0f);
    
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(scene.camera.pos, scene.camera.pos + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));

    scene.camera.invView = glm::inverse(view);
    engine::Pathtracer pathtracer;
    pathtracer.Init(context, scene, settings);

    auto start = std::chrono::high_resolution_clock::now();
    pathtracer.Run(context);
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>((end - start)).count();
    CORE_PRINT("took: {}s", duration);


    ExportToPng(context, 
    pathtracer.getRenderTarget(),
    pathtracer.getCommandPool(),
    "render.png",
    settings.ImageWidth,
    settings.ImageHeight);
}