#ifndef RENDERER_H
#define RENDERER_H

#include "model.hpp"
#include "display.hpp"
#include <memory>


class Renderer
{
public:
    Renderer(Display display);
    glm::mat4 calculateRotation(glm::vec3 start, glm::vec3 end);

    void drawLine(glm::vec3 start, glm::vec3 end, float radius = 0.1f, int colorIdx = 0);
    void drawPoint(glm::vec3 position, float radius = 0.1f, int colorIdx = 0);
    void drawRoom();
    void updateEyePos(glm::vec3 currentEyePos);
    void clear();
private:
    void setupShader();
    std::unique_ptr<Model> sphere;
    std::unique_ptr<Model> line;
    std::unique_ptr<Model> room;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Display> display;
    glm::vec3 currentEyePos;
    //Cached for speedup
    glm::mat4 projectionToEye;
};

#endif