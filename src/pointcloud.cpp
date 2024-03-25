#include "pointcloud.hpp"
#include <fstream>
#include <iostream>


PointCloud::PointCloud()
{
};

void PointCloud::updateCloud(std::vector<cv::Point3d> points)
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