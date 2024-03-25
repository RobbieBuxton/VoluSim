#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H

#include <vector> 
#include <opencv2/core/types.hpp> 

class PointCloud
{
public:

    PointCloud();
    ~PointCloud();
    void updateCloud(std::vector<cv::Point3d> points);
    void save(const std::string& filename);
private:
    std::vector<cv::Point3d> points;
};

#endif
