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
    Challenge(std::shared_ptr<Renderer> renderer,  std::shared_ptr<Hand> hand);
    void drawWith(Shader shader);
    void update();
private:
    class Segment
    {
    public:
        Segment(glm::vec3 start, glm::vec3 end, float radius);
        glm::vec3 start;
        glm::vec3 end;
        float radius;
        bool completed;
    };
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<Hand> hand;
    std::vector<Segment> segments;
    glm::vec3 grabPos;
    bool grabbing; 
    std::vector<glm::vec3> loadDirections(const std::string& path);
};

#endif
