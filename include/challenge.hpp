#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "model.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "hand.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <memory>

class Challenge
{
public:
    Challenge(std::shared_ptr<Renderer> globalRenderer);
    void drawWith(Shader shader);
    void updateHand(Hand hand);
private:
    class Segment
    {
    public:
        Segment(glm::vec3 start, glm::vec3 end, float radius);
        glm::vec3 start;
        glm::vec3 end;
        float radius;
    };
    std::shared_ptr<Renderer> renderer;
    std::vector<Segment> segments;
    std::unique_ptr<Hand> hand;
    glm::vec3 lastGrabPos = glm::vec3(0.0f, 0.0f, 0.0f);
};

#endif
