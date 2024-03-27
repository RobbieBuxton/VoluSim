#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H

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
private:
    std::vector<glm::vec3> points;
};

#endif
