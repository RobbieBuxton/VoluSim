#include "challenge.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include "filesystem.hpp"
#include <iostream>

Challenge::Challenge(std::shared_ptr<Renderer> renderer, std::shared_ptr<Hand> hand, int challengeNum)
{
    this->renderer = renderer;
    this->hand = hand;

    std::string challengePath = "data/challenges/demo" + std::to_string(challengeNum) + ".txt";
    std::vector<glm::vec3> directions = loadDirections(FileSystem::getPath(challengePath));

    glm::vec3 point = centre + glm::vec3(-4.0, -2.0, 2.0);
    glm::vec3 oldPoint;

    for (auto direction : directions)
    {
        oldPoint = point;
        point = oldPoint + direction;
        segments.push_back(Segment(oldPoint, point, 0.1));
    }

    startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

std::vector<glm::vec3> Challenge::loadDirections(const std::string &path)
{
    std::vector<glm::vec3> directions;
    std::ifstream file(path);
    std::string keyword;

    // Mapping of direction keywords to glm::vec3 vectors
    float len = 2.0f;
    std::unordered_map<std::string, glm::vec3> directionMap = {
        {"up", glm::vec3(0.0, len, 0.0)},
        {"down", glm::vec3(0.0, -len, 0.0)},
        {"forward", glm::vec3(0.0, 0.0, len)},
        {"back", glm::vec3(0.0, 0.0, -len)},
        {"right", glm::vec3(len, 0.0, 0.0)},
        {"left", glm::vec3(-len, 0.0, 0.0)}};

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
    if (segments.size() == completedCnt)
    {
        finished = true;
        return;
    }
    if (hand->getGrabPosition().has_value())
    {
        grabbing = true;
        grabPos = hand->getGrabPosition().value();

        Segment lastSegment = Segment(glm::vec3(0), glm::vec3(0), 0);
        lastSegment.completed = true;
        for (int i = completedCnt; i < segments.size(); i++)
        {
            Segment &segment = segments[i];
            if (segment.completed)
            {
                lastSegment = segment;
                completedCnt++;
                continue;
            }
            if (!lastSegment.completed)
            {
                break;
            }

            if (!segment.completed && glm::distance(segment.end, grabPos) < 0.5)
            {
                segment.completed = true;
                segment.completedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                std::cout << "Segment completed in " << (segment.completedTime - startTime).count() << "ms" << std::endl;
            }
            lastSegment = segment;
        }
    }
    else
    {
        grabbing = false;
    }
}

void Challenge::draw()
{	
	float width = 10;
	renderer->drawCuboid(centre, width, width, 0.1, 0);

	renderer->drawLine(centre + glm::vec3(-width/2.0f,-width/2.0f,0), centre + glm::vec3(-width/2.0f,-width/2.0f,width), 0.05, 0);
	renderer->drawLine(centre + glm::vec3(width/2.0f,-width/2.0f,0), centre + glm::vec3(width/2.0f,-width/2.0f,width), 0.05, 0);
	renderer->drawLine(centre + glm::vec3(-width/2.0f,width/2.0f,0), centre + glm::vec3(-width/2.0f,width/2.0f,width), 0.05, 0);
	renderer->drawLine(centre + glm::vec3(width/2.0f,width/2.0f,0), centre + glm::vec3(width/2.0f,width/2.0f,width), 0.05, 0);
	
	renderer->drawLine(centre + glm::vec3(-width/2.0f,-width/2.0f,width), centre + glm::vec3(width/2.0f,-width/2.0f,width), 0.05, 0);
	renderer->drawLine(centre + glm::vec3(-width/2.0f,width/2.0f,width), centre + glm::vec3(width/2.0f,width/2.0f,width), 0.05, 0);
	renderer->drawLine(centre + glm::vec3(-width/2.0f,-width/2.0f,width), centre + glm::vec3(-width/2.0f,width/2.0f,width), 0.05, 0);
    renderer->drawLine(centre + glm::vec3(width/2.0f,-width/2.0f,width), centre + glm::vec3(width/2.0f,width/2.0f,width), 0.05, 0);

	renderer->drawPoint(segments.front().start, segments.front().radius * 3, 2);
    Segment lastSegment = Segment(glm::vec3(0), glm::vec3(0), 0);
    lastSegment.completed = true;
    for (Segment &segment : segments)
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

nlohmann::json Challenge::returnJson()
{
    nlohmann::json jsonOutput;
    for (int i = 0; i < segments.size(); i++)
    {
        Segment &segment = segments[i];
        jsonOutput.push_back({{"index", i},
                              {"completedTime", segment.completedTime.count()}});
    }
		return jsonOutput;
}

Challenge::Segment::Segment(glm::vec3 start, glm::vec3 end, float radius)
{
    this->start = start;
    this->end = end;
    this->radius = radius;
    this->completed = false;
	this->completedTime = std::chrono::milliseconds(0);
}

bool Challenge::isFinished()
{
    return finished;
}