#include <pathtracer.hpp>
#include "app/export.hpp"
#include <gtc/matrix_transform.hpp>
#include "app/externInput.hpp"
#include <chrono>



int main()
{
    core::instance instance({});
    core::device device(instance.getInstance());
    core::allocator alloc(instance.getInstance(), device.getPhysicalDevice(), device.getDevice());
    core::context context(device, alloc, instance);

    engine::Scene scene;
    loader::loaderOutput bunny = loader::assimpLoader::loadModel("bunny.obj");
    
    engine::Material material;
    material.albedo = glm::vec3(0.0, 1.0, 0.0);
    material.metalness = 0.0f;
    material.roughness = 1.0f;
    
    engine::Model model;
    engine::Mesh mesh;
    mesh.InitBuffer(context, bunny.vertices, bunny.indices);
    mesh.transform = glm::mat4(1.0f);
    mesh.matId = 0;
    model.meshes.push_back(mesh);
    model.transform = glm::mat4(1.0f);
    model.transform = glm::translate(model.transform, glm::vec3(0, 0, 0));
    model.transform = glm::scale(model.transform, glm::vec3(10.0f));
    scene.models.push_back(model);
    scene.materials.push_back(material);

    
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
    double duration = std::chrono::duration<double, std::milli>((end - start)).count();
    CORE_PRINT("took: {}ms", duration);


    ExportToPng(context, 
    pathtracer.getRenderTarget(),
    pathtracer.getCommandPool(),
    "render.png",
    settings.ImageWidth,
    settings.ImageHeight);
}