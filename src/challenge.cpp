#include "challenge.hpp"
#include <glm/gtc/matrix_transform.hpp>
// Top class of challenge
// Sub classes are segements which represent rods where the user tries to enter in the rod through one face and
// exit through the other face while not exceeding the radius of the rod
// The rods would change colour when completed vs in the process of being completed vs not completed
// calling render on challenger should call render on all segments

Challenge::Challenge(std::shared_ptr<Renderer> renderer)
{
    this->renderer = renderer;

    segments.push_back(Segment(glm::vec3(0.0, 25.0, 20.0), glm::vec3(5.0, 30.0, 25.0), 0.1));
    segments.push_back(Segment(glm::vec3(5.0, 30.0, 25.0), glm::vec3(10.0, 25.0, 20.0), 0.1));
}

void Challenge::drawWith(Shader shader)
{
    for (Segment segment : segments)
    {
        renderer->drawLine(segment.start, segment.end);
    }
    renderer->drawPoint(lastGrabPos);
}

void Challenge::updateHand(Hand updatedHand)
{
    hand = std::make_unique<Hand>(updatedHand);
    std::optional<glm::vec3> grabPos = hand->getGrabPosition();
    if (grabPos.has_value())
    {
        lastGrabPos = grabPos.value();
    }
}

Challenge::Segment::Segment(glm::vec3 start, glm::vec3 end, float radius)
{
    this->start = start;
    this->end = end;
    this->radius = radius;
}
