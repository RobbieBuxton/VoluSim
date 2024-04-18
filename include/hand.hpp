#ifndef HAND_H
#define HAND_H

#include <glm/glm.hpp>
#include <vector>
#include "model.hpp"
#include "shader.hpp"
#include "mediapipe.h"

class Hand
{
public:
    Hand(std::vector<glm::vec3> inputLandmarks);
    void checkIfGrabbing();
    void drawWith(Model model, Shader shader, glm::vec3 cameraOffset, glm::vec3 currentEyePos);
    void save(const std::string &filename);

private:
    glm::vec3 landmarks[21];
};

#endif