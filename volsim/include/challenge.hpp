#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "model.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "hand.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include "json.hpp"

class Challenge
{
public:
    Challenge(std::shared_ptr<Renderer> renderer,  std::shared_ptr<Hand> hand, int challengeNum, glm::vec3 centre);
    void draw();
    void update();
    bool isFinished();
    nlohmann::json returnJson();
private:
    class Segment
    {
    public:
        Segment(glm::vec3 start, glm::vec3 end, float radius);
        glm::vec3 start;
        glm::vec3 end;
        float radius;
        bool completed;
        std::chrono::milliseconds completedTime;
    };
    std::chrono::milliseconds startTime;
	glm::vec3 centre;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<Hand> hand;
    std::vector<Segment> segments;
    int completedCnt = 0;
    bool finished = false;
    glm::vec3 grabPos;
    bool grabbing; 
    std::vector<glm::vec3> loadDirections(const std::string& path);
};

#endif
