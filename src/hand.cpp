#include "hand.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Hand::Hand(std::vector<glm::vec3> inputLandmarks)
{
    for (int i = 0; i < 21; i++)
    {
        landmarks[i] = inputLandmarks[i];
    }
}

std::optional<glm::vec3> Hand::getGrabPosition()
{
    // Check if the hand is grabbing
    // If the distance between the thumb and the index finger is less than a certain threshold, the hand is grabbing
    float distance = glm::distance(landmarks[mp_hand_landmark_thumb_tip], landmarks[mp_hand_landmark_index_finger_tip]);
    if (distance < 3)
    {
       return (landmarks[mp_hand_landmark_thumb_tip] + landmarks[mp_hand_landmark_index_finger_tip])/2.0f;
    }

    return {};
}

void Hand::drawWith(Model model, Shader shader, glm::vec3 cameraOffset, glm::vec3 currentEyePos)
{
    glm::mat4 modelMatrix, scaleMatrix, translationMatrix, centeringMatrix;
    centeringMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 2.0, 0.0));
    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1, 0.1, 0.1));
    shader.setVec3("objectColor", glm::vec3(0.0f, 1.0f, 0.0f));

    for (const auto &point : landmarks)
    {
        if (point.z < currentEyePos.z)
        {
            glm::vec3 pointTranslation = point + cameraOffset;
            translationMatrix = glm::translate(glm::mat4(1.0f), pointTranslation);
            modelMatrix = translationMatrix * scaleMatrix * centeringMatrix;
            shader.setMat4("model", modelMatrix);
            model.draw(shader);
        }
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
    for (const auto &point : landmarks)
    {
        outFile << point.x << ", " << point.y << ", " << point.z << "\n";
    }

    outFile.close();
}