#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H

#include "model.hpp"
#include "display.hpp"
#include "shader.hpp"

#include <vector> 
#include <string>
#include <glm/glm.hpp>

class PointCloud
{
public:
    PointCloud();
    ~PointCloud();
    void updateCloud(std::vector<glm::vec3> points);
    void save(const std::string& filename);
    void drawWith(Model model, Shader shader, Display display);
private:
    std::vector<glm::vec3> points;
};

#endif
