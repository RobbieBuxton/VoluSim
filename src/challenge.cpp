#include "challenge.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include "filesystem.hpp"

Challenge::Challenge(std::shared_ptr<Renderer> renderer, std::shared_ptr<Hand> hand)
{
    this->renderer = renderer;
    this->hand = hand;

    std::vector<glm::vec3> directions = loadDirections(FileSystem::getPath("data/challenges/demo1.txt"));

    glm::vec3 point = glm::vec3(-12.5, 20.0, 20.0);
    glm::vec3 oldPoint;

    for (auto direction : directions)
    {
        oldPoint = point;
        point = oldPoint + direction;
        segments.push_back(Segment(oldPoint, point, 0.1));
    }
}

std::vector<glm::vec3> Challenge::loadDirections(const std::string &path)
{
    std::vector<glm::vec3> directions;
    std::ifstream file(path);
    std::string keyword;

    // Mapping of direction keywords to glm::vec3 vectors
    std::unordered_map<std::string, glm::vec3> directionMap = {
        {"up", glm::vec3(0.0, 5.0, 0.0)},
        {"down", glm::vec3(0.0, -5.0, 0.0)},
        {"forward", glm::vec3(0.0, 0.0, 5.0)},
        {"back", glm::vec3(0.0, 0.0, -5.0)},
        {"right", glm::vec3(5.0, 0.0, 0.0)},
        {"left", glm::vec3(-5.0, 0.0, 0.0)}};

    while (getline(file, keyword))
    {
        auto it = directionMap.find(keyword);
        if (it != directionMap.end())
        {
            directions.push_back(it->second);
        }
        else
        {
            std::cerr << "Warning: Unknown direction keyword '" << keyword << "' in file." << std::endl;
        }
    }

    return directions;
}

void Challenge::update()
{
    if (hand->getGrabPosition().has_value())
    {
        grabbing = true;
        grabPos = hand->getGrabPosition().value();

        Segment lastSegment = Segment(glm::vec3(0), glm::vec3(0), 0);
        lastSegment.completed = true;
        for (Segment &segment : segments)
        {
            if (segment.completed)
            {
                lastSegment = segment;
                continue;
            }
            if (!lastSegment.completed)
            {
                break;
            }

            if (!segment.completed && glm::distance(segment.end, grabPos) < 0.5)
            {
                segment.completed = true;
            }
            lastSegment = segment;
        }
    } else 
    {
        grabbing = false;
    }
}

void Challenge::drawWith(Shader shader)
{
    renderer->drawPoint(segments.front().start, segments.front().radius * 3, 2);
    Segment lastSegment = Segment(glm::vec3(0), glm::vec3(0), 0);
    lastSegment.completed = true;
    for (Segment segment : segments)
    {
        if (segment.completed)
        {
            renderer->drawLine(segment.start, segment.end, segment.radius, 0);
            renderer->drawPoint(segment.end, segment.radius * 3, 2);
        }
        else if (lastSegment.completed)
        {
            if (grabbing && glm::distance(segment.start, grabPos) < 10.0f)
            {
                renderer->drawLine(segment.start, grabPos, segment.radius, 0);
            }
            
            renderer->drawPoint(segment.end, segment.radius * 3, 4);
        }
        else
        {
            renderer->drawPoint(segment.end, segment.radius * 3, 0);
        }
        lastSegment = segment;
    }
}

Challenge::Segment::Segment(glm::vec3 start, glm::vec3 end, float radius)
{
    this->start = start;
    this->end = end;
    this->radius = radius;
    this->completed = false;
}
