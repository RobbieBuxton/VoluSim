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
        for (int i = 0; i < 21; i++)
        {
            landmarks[i] = inputLandmarks.value()[i];
        }
    }
}

std::optional<glm::vec3> Hand::getGrabPosition()
{
    // Check if the hand is grabbing
    // If the distance between the thumb and the index finger is less than a certain threshold, the hand is grabbing
    float distance = glm::distance(landmarks[mp_hand_landmark_thumb_tip], landmarks[mp_hand_landmark_index_finger_tip]);
    if (distance < 3)
    {
        return (landmarks[mp_hand_landmark_thumb_tip] + landmarks[mp_hand_landmark_index_finger_tip]) / 2.0f;
    }

    return {};
}

void Hand::draw()
{
    renderer.get()->drawPoint(landmarks[mp_hand_landmark_thumb_tip], 0.1f, 2);
    renderer.get()->drawPoint(landmarks[mp_hand_landmark_index_finger_tip],  0.1f, 2);
    renderer.get()->drawLine(landmarks[mp_hand_landmark_thumb_tip], landmarks[mp_hand_landmark_index_finger_tip], 0.05f, 2);
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
    for (const auto &point : landmarks)
    {
        outFile << point.x << ", " << point.y << ", " << point.z << "\n";
    }

    outFile.close();
}