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

void Hand::checkIfGrabbing()
{
    // Check if the hand is grabbing
    // If the distance between the thumb and the index finger is less than a certain threshold, the hand is grabbing
    float distance = glm::distance(landmarks[mp_hand_landmark_thumb_tip], landmarks[mp_hand_landmark_index_finger_tip]);
    if (distance < 2)
    {
        std::cout << "Grabbing at distance:"  << distance << std::endl;
    } else {
        std::cout << "Not Grabbing at distance:" << distance << std::endl;
        // std::cout << "Thumb at: " << landmarks[mp_hand_landmark_thumb_tip].x << ", " << landmarks[mp_hand_landmark_thumb_tip].y << ", " << landmarks[mp_hand_landmark_thumb_tip].z << std::endl;
        // std::cout << "Index finger at: " << landmarks[mp_hand_landmark_index_finger_tip].x << ", " << landmarks[mp_hand_landmark_index_finger_tip].y << ", " << landmarks[mp_hand_landmark_index_finger_tip].z << std::endl;
    }
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
            model.Draw(shader);
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