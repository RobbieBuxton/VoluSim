#ifndef HAND_H
#define HAND_H

#include <glm/glm.hpp>
#include <vector>
#include "model.hpp"
#include "shader.hpp"
#include "mediapipe.h"
#include "optional"
#include "renderer.hpp"

class Hand
{
public:
    Hand(std::shared_ptr<Renderer> renderer, glm::vec3 offset);
    std::optional<glm::vec3> getGrabPosition();
    void draw();
    void save(const std::string &filename);
    void updateLandmarks(std::optional<std::vector<glm::vec3>> inputLandmarks);

private:
    glm::vec3 index;
    glm::vec3 thumb;
    glm::vec3 offset;
    std::shared_ptr<Renderer> renderer;
};

#endif