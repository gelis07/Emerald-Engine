#include <app/Application.h>


int main()
{

    Application app;

    app.Init();
    app.OnUpdate();

    // std::vector<glm::vec3> SPoints = {glm::vec3(0,0,5), glm::vec3(3,0,5)};
    // std::vector<float> radius = {1.0f, 1.0f};
    // std::vector<glm::vec3> colors = {glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0, 1.0, 1.0)};
    // std::vector<float> emPowers = {0.0f, 1.0f};
    // std::vector<float> mult = {0.3f, 0.3f};
    return 0;
}