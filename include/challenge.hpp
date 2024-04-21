#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "model.hpp"
#include "shader.hpp"
#include "hand.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <memory>

class Challenge
{
public:
    Challenge();
    void drawWith(Shader shader, glm::vec3 cameraOffset);
    void updateHand(Hand hand);
private:
    class Segment
    {
    public:
        Segment(glm::vec3 start, glm::vec3 end, float radius);
        void drawWith(Model cube, Model cylinder, Shader shader, glm::vec3 lastGrabPos, glm::vec3 cameraOffset);
    
    private:
        glm::vec3 start;
        glm::vec3 end;
        float radius;
    };
    std::vector<Segment> segments;
    std::unique_ptr<Model> cube;
    std::unique_ptr<Model> cylinder;
    std::unique_ptr<Hand> hand;
    glm::vec3 lastGrabPos = glm::vec3(0.0f, 0.0f, 0.0f);
};

#endif
