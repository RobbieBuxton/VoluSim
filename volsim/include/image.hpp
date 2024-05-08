#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/core.hpp>
#include "shader.hpp"

class Image
{

public:
    Image(glm::vec2 topLeft, glm::vec2 bottomRight);
    ~Image();

    void updateImage(const cv::Mat &image);
    void save(const std::string& filename);
    GLuint textureID;
    GLuint VBO, VAO, EBO;
};

#endif