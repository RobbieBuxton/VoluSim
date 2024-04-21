#include "challenge.hpp"
#include <glm/gtc/matrix_transform.hpp>
// Top class of challenge
// Sub classes are segements which represent rods where the user tries to enter in the rod through one face and
// exit through the other face while not exceeding the radius of the rod
// The rods would change colour when completed vs in the process of being completed vs not completed
// calling render on challenger should call render on all segments

Challenge::Challenge()
{
    cube = std::make_unique<Model>("data/resources/models/sphere.obj");
    line = std::make_unique<Model>("data/resources/models/cylinder.obj");

    Segment segment1(glm::vec3(0.0, 25.0, 20.0), glm::vec3(5.0, 30.0, 25.0), 0.1);
    segments.push_back(segment1);
}

void Challenge::drawWith(Shader shader)
{
    for (Segment segment : segments)
    {
        segment.drawWith(*cube.get(), *line.get(), shader, lastGrabPos);
    }
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

glm::mat4 calculateRotation(glm::vec3 start, glm::vec3 end)
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


void Challenge::Segment::drawLine(Shader shader, glm::vec3 start, glm::vec3 end, Model line)
{
    glm::mat4 modelMatrix, scaleMatrix, translationMatrix, rotationMatrix;
    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, glm::distance(start,end), 1.0f));
    // scaleMatrix = glm::mat4(1.0f);
    rotationMatrix = calculateRotation(start, end);
    translationMatrix = glm::translate(glm::mat4(1.0f), ((start + end) / 2.0f));
    modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    shader.setMat4("model", modelMatrix);
    shader.setInt("overrideMaterialID", 2);
    line.draw(shader);
    shader.setInt("overrideMaterialID", -1);
}

void Challenge::Segment::drawWith(Model cube, Model line, Shader shader, glm::vec3 lastGrabPos)
{
    glm::mat4 modelMatrix, translationMatrix, scaleMatrix;
    scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

    translationMatrix = glm::translate(glm::mat4(1.0f), start);
    modelMatrix = translationMatrix * scaleMatrix;
    shader.setMat4("model", modelMatrix);

    cube.draw(shader);

    drawLine(shader, start, lastGrabPos, line);

    translationMatrix = glm::translate(glm::mat4(1.0f), end);
    modelMatrix = translationMatrix * scaleMatrix;
    shader.setMat4("model", modelMatrix);
    shader.setInt("overrideMaterialID", -1);
    
    cube.draw(shader);
}