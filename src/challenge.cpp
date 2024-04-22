#include "challenge.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

Challenge::Challenge(std::shared_ptr<Renderer> renderer, std::shared_ptr<Hand> hand)
{
    this->renderer = renderer;
    this->hand = hand;

    glm::vec3 up = glm::vec3(0.0, 5.0, 0.0);
    glm::vec3 down = glm::vec3(0.0, -5.0, 0.0);
    glm::vec3 forward = glm::vec3(0.0, 0.0, 5.0);
    glm::vec3 backward = glm::vec3(0.0, 0.0, -5.0);
    glm::vec3 right = glm::vec3(5.0, 0.0, 0.0);
    glm::vec3 left = glm::vec3(-5.0, 0.0, 0.0);

    glm::vec3 oldPoint;
    glm::vec3 point = glm::vec3(-12.5, 20.0, 20.0);

    std::vector<glm::vec3> directions = {up, backward, up, right, forward, right, forward, left, down, right, right, backward, up, backward, right, down, forward, down};

    for (auto direction : directions)
    {
        oldPoint = point;
        point = oldPoint + direction;
        segments.push_back(Segment(oldPoint, point, 0.1));
    }
}

void Challenge::drawWith(Shader shader)
{
    for (Segment segment : segments)
    {
        if (hand->getGrabPosition().has_value())
        {
            glm::vec3 grabPos = hand->getGrabPosition().value();
            if (glm::distance(grabPos, segment.start) < 2.0)
            {
                renderer->drawPoint(segment.start, segment.radius * 3, 2);
            }
            else
            {
                renderer->drawPoint(segment.start, segment.radius * 3, 1);
            }
        }
        else
        {
            renderer->drawPoint(segment.start, segment.radius * 3, 1);
        }
        renderer->drawLine(segment.start, segment.end, segment.radius, 0);
    }
    renderer->drawPoint(segments.back().end, segments.back().radius * 3, 1);
}

Challenge::Segment::Segment(glm::vec3 start, glm::vec3 end, float radius)
{
    this->start = start;
    this->end = end;
    this->radius = radius;
}
