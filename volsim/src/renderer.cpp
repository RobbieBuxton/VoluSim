#include "renderer.hpp"
#include "model.hpp"
#include "filesystem.hpp"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

Renderer::Renderer(Display display)
{
    this->sphere = std::make_unique<Model>("data/resources/models/sphere.obj");
    this->line = std::make_unique<Model>("data/resources/models/cylinder.obj");
	this->cube = std::make_unique<Model>("data/resources/models/cube.obj");
    this->room = std::make_unique<Model>("data/resources/models/room.obj");
	this->teapot = std::make_unique<Model>("data/resources/models/teapot.obj");
    this->modelShader = std::make_unique<Shader>(FileSystem::getPath("data/shaders/camera.vs").c_str(), FileSystem::getPath("data/shaders/camera.fs").c_str());
    this->imageShader = std::make_unique<Shader>(FileSystem::getPath("data/shaders/image.vs").c_str(), FileSystem::getPath("data/shaders/image.fs").c_str());
    this->display = std::make_unique<Display>(display);
}

void Renderer::lazyLoadModel(std::unique_ptr<Model>& model, const std::string& path) {
    if (!model) {
        model = std::make_unique<Model>(path);
    }
}

void Renderer::drawRungholt() {
    setupShader();
    lazyLoadModel(rungholt, "data/resources/models/rungholt.obj");

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.04f, 0.04f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 25.0f, 0.0f));
    glm::mat4 model = translationMatrix * rotationMatrix * scaleMatrix;

    modelShader->setMat4("model", model);
    modelShader->setBool("usePhongShading", true);
    rungholt->draw(*modelShader.get());
}

void Renderer::drawHouse() {
    setupShader();
    lazyLoadModel(house, "data/resources/models/house.obj");

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 25.0f, 0.0f));
    glm::mat4 model = translationMatrix * rotationMatrix * scaleMatrix;

    modelShader->setMat4("model", model);
    modelShader->setBool("usePhongShading", true);
    house->draw(*modelShader.get());
}

void Renderer::drawMolecule() {
    setupShader();
    lazyLoadModel(molecule, "data/resources/models/8qbk.obj");

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    // glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 15.0f, 5.0f));
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 25.0f, 0.0f));
    glm::mat4 model = translationMatrix * rotationMatrix * scaleMatrix;

    modelShader->setMat4("model", model);
    modelShader->setBool("usePhongShading", true);
    molecule->draw(*modelShader.get());
}

void Renderer::drawErato() {
    setupShader();
    lazyLoadModel(erato, "data/resources/models/erato.obj");

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 158.0f, 0.0f));
    glm::mat4 model = translationMatrix * rotationMatrix * scaleMatrix;

    modelShader->setMat4("model", model);
    modelShader->setBool("usePhongShading", false);
    erato->draw(*modelShader.get());
}

void Renderer::drawTeapot() {
	setupShader();

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 25.0f, 3.0f));
    glm::mat4 model = translationMatrix * rotationMatrix * scaleMatrix;
	
    modelShader->setMat4("model", model);
	modelShader->setInt("overrideMaterialID", 4);
	modelShader->setBool("usePhongShading", false);
    teapot->draw(*modelShader.get());
	modelShader->setInt("overrideMaterialID", -1);
	modelShader->setBool("usePhongShading", true);
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

void Renderer::setupShader() {
    modelShader->use();
    modelShader->setMat4("projection", projectionToEye);
    modelShader->setVec3("viewPos", currentEyePos);
    modelShader->setVec3("lightPos", glm::vec3(0.0f, -45.0f, 40.0f));
    modelShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
}

void Renderer::drawPoint(glm::vec3 position, float radius, int colorIdx) {
    setupShader();

    // point object has a radius of 0.5 so we scale accordingly
    float scaleFactor = radius / 0.5f;

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor,scaleFactor, scaleFactor));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 modelMatrix = translationMatrix * scaleMatrix;

    modelShader->setMat4("model", modelMatrix);
    modelShader->setInt("overrideMaterialID", colorIdx);
    sphere->draw(*modelShader.get());
    modelShader->setInt("overrideMaterialID", -1);
}

void Renderer::drawLine(glm::vec3 start, glm::vec3 end, float radius, int colorIdx) {
    setupShader();

    // cylinder object has a radius of 0.25 so we scale accordingly
    float scaleFactor = radius / 0.25f;
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor, glm::distance(start, end), scaleFactor));


    glm::mat4 rotationMatrix = calculateRotation(start, end);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), (start + end) / 2.0f);
    glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

    modelShader->setMat4("model", modelMatrix);
    modelShader->setInt("overrideMaterialID", colorIdx);
    line->draw(*modelShader.get());
    modelShader->setInt("overrideMaterialID", -1);
}

void Renderer::drawCuboid(glm::vec3 position, float width, float length, float height, int colorIdx) {
	setupShader();

	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(width, length, height));
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 modelMatrix = translationMatrix * scaleMatrix;

	modelShader->setMat4("model", modelMatrix);
	modelShader->setInt("overrideMaterialID", colorIdx);
	cube->draw(*modelShader.get());
	modelShader->setInt("overrideMaterialID", -1);
}

void Renderer::drawRoom() {
    setupShader();

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(display->width, display->height, display->depth));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, display->height / 2.0, -display->depth / 2.0));
    glm::mat4 model = translationMatrix * scaleMatrix;

    modelShader->setMat4("model", model);
	modelShader->setInt("overrideMaterialID", 0);
    room->draw(*modelShader.get());
	modelShader->setInt("overrideMaterialID", -1);
}

void Renderer::updateEyePos(glm::vec3 currentEyePos)
{	
	// std::cout << "L Eye Pos: " << glm::to_string(currentEyePos) << std::endl;
    this->currentEyePos = currentEyePos;
    this->projectionToEye = display->projectionToEye(currentEyePos);
}

void Renderer::drawImage(Image &image)
{
    imageShader->use();
    glBindVertexArray(image.VAO);
    glBindTexture(GL_TEXTURE_2D, image.textureID);
    imageShader->setInt("textureMap", 0); // Ensure the modelShader uniform matches this texture unit

    // Draw the quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind VAO (not always necessary, but it's a good practice)
    glBindVertexArray(0);
}

void Renderer::clear()
{ 
	// Apricot
	// glClearColor(0.96f, 0.81f, 0.64f, 1.0f);
	// Black
	// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Gress Green 
	// glClearColor(0.47f, 0.53f, 0.47f, 1.0f);
	// Light Pink 
	glClearColor(0.96f, 0.76f, 0.76f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}