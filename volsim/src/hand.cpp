#include "hand.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

Hand::Hand(std::shared_ptr<Renderer> renderer, glm::vec3 offset)
{
    this->renderer = renderer;
    this->offset = offset;
}

void Hand::updateLandmarks(std::optional<std::vector<glm::vec3>> inputLandmarks)
{
    if (inputLandmarks.has_value())
    {
        index = inputLandmarks.value()[0] + offset;
        middle = inputLandmarks.value()[1] + offset;
		// std::cout << "Index Pos: " << glm::to_string(index) << std::endl;
		// std::cout << "Middle Pos: " << glm::to_string(middle) << std::endl;
		// std::cout << std::endl;
		// std::cout << std::endl;
    }
}

std::optional<glm::vec3> Hand::getGrabPosition()
{
    // Check if the hand is grabbing
    // If the distance between the middle and the index finger is less than a certain threshold, the hand is grabbing
    float distance = glm::distance(middle, index);
    if (distance < 2.0f)
    {
        return (middle + index) / 2.0f;
    }

    return {};
}

void Hand::draw()
{
    renderer.get()->drawPoint(middle, 0.1f, 2);
    renderer.get()->drawPoint(index,  0.1f, 2);
    renderer.get()->drawLine(middle, index, 0.05f, 2);
    if (getGrabPosition().has_value())
    {
        renderer.get()->drawPoint(getGrabPosition().value(), 0.1f, 1);
    }
}

void Hand::save(const std::string &filename)
{
    std::ofstream outFile(filename);
    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return;
    }

    // Write the header
    outFile << "x, y, z\n";

    // Write the points
    
    outFile << index.x << ", " << index.y << ", " << index.z << "\n";
    outFile << middle.x << ", " << middle.y << ", " << middle.z << "\n";
    
    outFile.close();
}