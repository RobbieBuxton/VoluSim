#include "pointcloud.hpp"
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

PointCloud::PointCloud()
{
};


void PointCloud::drawWith(Model model, Shader shader, Display display)
{
    glm::mat4 modelMatrix, scaleMatrix, translationMatrix, centeringMatrix;
    centeringMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 2.0, 0.0));
    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1, 0.1, 0.1));
    shader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f));

    for (const auto& point : points)
    {
        glm::vec3 pointTranslation = point + glm::vec3(0.0, 0.0, 0.0f);
        translationMatrix = glm::translate(glm::mat4(1.0f), pointTranslation);
        modelMatrix = translationMatrix * scaleMatrix * centeringMatrix;
        shader.setMat4("model", modelMatrix);
        model.Draw(shader);
    }
}

void PointCloud::updateCloud(std::vector<glm::vec3> points)
{
    this->points = points;
}

PointCloud::~PointCloud()
{
};

void PointCloud::save(const std::string& filename)
{
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return;
    }

    // Write the header
    outFile << "x, y, z\n";

    // Write the points
    for (const auto& point : points) {
        outFile << point.x << ", " << point.y << ", " << point.z << "\n";
    }

    outFile.close();
}