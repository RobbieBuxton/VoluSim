#ifndef RENDERER_H
#define RENDERER_H

#include "model.hpp"
#include "display.hpp"
#include "image.hpp"

#include <memory>


class Renderer
{
public:
    Renderer(Display display);
    glm::mat4 calculateRotation(glm::vec3 start, glm::vec3 end);

    void drawLine(glm::vec3 start, glm::vec3 end, float radius = 0.1f, int colorIdx = 0);
    void drawPoint(glm::vec3 position, float radius = 0.1f, int colorIdx = 0);
	void drawCuboid(glm::vec3 position, float width, float length, float height, int colorIdx = 0);
    void drawRungholt();
	void drawMolecule();
	void drawErato();
	void drawHouse();
	void drawTeapot();
	void drawImage(Image &image);
    void drawRoom();
    void updateEyePos(glm::vec3 currentEyePos);
	void lazyLoadModel(std::unique_ptr<Model>& model, const std::string& path);
    void clear();

private:
    void setupShader();
    std::unique_ptr<Model> sphere;
    std::unique_ptr<Model> line;
	std::unique_ptr<Model> cube;
    std::unique_ptr<Model> room;
	std::unique_ptr<Model> rungholt = nullptr;
	std::unique_ptr<Model> erato = nullptr;
	std::unique_ptr<Model> house = nullptr;
	std::unique_ptr<Model> molecule = nullptr;
	std::unique_ptr<Model> teapot;
    std::unique_ptr<Shader> modelShader;
    std::unique_ptr<Shader> imageShader;
    std::unique_ptr<Display> display;
    glm::vec3 currentEyePos;
    //Cached for speedup
    glm::mat4 projectionToEye;
};

#endif