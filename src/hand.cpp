#include "hand.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Hand::Hand(std::shared_ptr<Renderer> renderer)
{
    this->renderer = renderer;
}

void Hand::updateLandmarks(std::optional<std::vector<glm::vec3>> inputLandmarks)
{
    if (inputLandmarks.has_value())
    {
        index = inputLandmarks.value()[0];
        thumb = inputLandmarks.value()[1];
    }
}

std::optional<glm::vec3> Hand::getGrabPosition()
{
    // Check if the hand is grabbing
    // If the distance between the thumb and the index finger is less than a certain threshold, the hand is grabbing
    float distance = glm::distance(thumb, index);
    if (distance < 2.0f)
    {
        return (thumb + index) / 2.0f;
    }

    return {};
}

void Hand::draw()
{
    renderer.get()->drawPoint(thumb, 0.1f, 2);
    renderer.get()->drawPoint(index,  0.1f, 2);
    renderer.get()->drawLine(thumb, index, 0.05f, 2);
    if (getGrabPosition().has_value())
    {
        renderer.get()->drawPoint(getGrabPosition().value(), 0.1f, 1);
    }
}

// void Hand::save(const std::string &filename)
// {
//     std::ofstream outFile(filename);
//     if (!outFile.is_open())
//     {
//         std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
//         return;
//     }

//     // Write the header
//     outFile << "x, y, z\n";

//     // Write the points
//     for (const auto &point : landmarks)
//     {
//         outFile << point.x << ", " << point.y << ", " << point.z << "\n";
//     }

//     outFile.close();
// }