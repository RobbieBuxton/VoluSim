#include "renderer.hpp"
#include "model.hpp"
#include "filesystem.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(Display display)
{
    this->sphere = std::make_unique<Model>("data/resources/models/sphere.obj");
    this->line = std::make_unique<Model>("data/resources/models/cylinder.obj");
    this->room = std::make_unique<Model>("data/resources/models/room.obj");
    this->shader = std::make_unique<Shader>(FileSystem::getPath("data/shaders/camera.vs").c_str(), FileSystem::getPath("data/shaders/camera.fs").c_str());
    this->display = std::make_unique<Display>(display);
}

glm::mat4 Renderer::calculateRotation(glm::vec3 start, glm::vec3 end)
{
    glm::vec3 initialAxis(0, 1, 0);
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 rotationAxis = glm::cross(initialAxis, direction);

    if (glm::length(rotationAxis) != 0)
        rotationAxis = glm::normalize(rotationAxis);
    else
        return glm::mat4(1.0f);

    // Compute the angle of rotation
    float angle = acos(glm::dot(initialAxis, direction));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

    return rotationMatrix;
}


void Renderer::drawPoint(glm::vec3 position) 
{
    glm::mat4 modelMatrix, translationMatrix, scaleMatrix;
    shader->use();
    shader->setMat4("projection", projectionToEye);
    shader->setVec3("viewPos", currentEyePos);
    shader->setVec3("lightPos", glm::vec3(0.0f, display->height, 80.0f));
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    translationMatrix = glm::translate(glm::mat4(1.0f), position);
    
    modelMatrix = translationMatrix * scaleMatrix;
    shader->setMat4("model", modelMatrix);
    shader->setInt("overrideMaterialID", 2);
    sphere->draw(*shader.get());
    shader->setInt("overrideMaterialID", -1);
}

void Renderer::drawLine(glm::vec3 start, glm::vec3 end)
{
    glm::mat4 modelMatrix, scaleMatrix, translationMatrix, rotationMatrix;

    shader->use();
    shader->setMat4("projection", projectionToEye);
    shader->setVec3("viewPos", currentEyePos);
    shader->setVec3("lightPos", glm::vec3(0.0f, display->height, 80.0f));
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, glm::distance(start, end), 1.0f));
    rotationMatrix = calculateRotation(start, end);
    translationMatrix = glm::translate(glm::mat4(1.0f), ((start + end) / 2.0f));
    modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    shader->setMat4("model", modelMatrix);
    shader->setInt("overrideMaterialID", 2);
    line->draw(*shader.get());
    shader->setInt("overrideMaterialID", -1);
}

void Renderer::drawRoom()
{
    glm::mat4 model, scaleMatrix, translationMatrix, centeringMatrix, rotationMatrix;
    // activate shader
    shader->use();

    shader->setMat4("projection", projectionToEye);
    shader->setVec3("viewPos", currentEyePos);
    shader->setVec3("lightPos", glm::vec3(0.0f, display->height, 80.0f));
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(display->width, display->height, display->depth));
    translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, display->height / 2.0, -display->depth / 2.0));
    model = translationMatrix * scaleMatrix;
    shader->setMat4("model", model);

    room->draw(*shader.get());

}

void Renderer::updateEyePos(glm::vec3 currentEyePos)
{
    this->currentEyePos = currentEyePos;
    this->projectionToEye = display->projectionToEye(currentEyePos);
}

void Renderer::clear()
{
    glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}