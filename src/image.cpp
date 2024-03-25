#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "image.hpp"
#include "shader.hpp"
#include "filesystem.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

Image::Image(glm::vec2 topLeft, glm::vec2 bottomRight)
{
    shader = new Shader(FileSystem::getPath("data/shaders/image.vs").c_str(), FileSystem::getPath("data/shaders/image.fs").c_str());

    // Calculate positions from screen coordinates
    float vertices[] = {
        // positions          // texture coords
        bottomRight.x, topLeft.y,       1.0f, 0.0f,   // top right
        bottomRight.x, bottomRight.y,   1.0f, 1.0f,   // bottom right
        topLeft.x,     bottomRight.y,   0.0f, 1.0f,   // bottom left
        topLeft.x,     topLeft.y,       0.0f, 0.0f    // top left 
    };

    unsigned int indices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    // Set up texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);


    // Set up vertex data (and buffer(s)) and attribute pointers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attribute(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    return;
}

void Image::updateImage(const cv::Mat &image)
{
  
    glBindTexture(GL_TEXTURE_2D, textureID);
    cv::Mat imageRGB;

    cv::cvtColor(image, imageRGB, cv::COLOR_BGR2RGB); // Convert BGR to RGB

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageRGB.cols, imageRGB.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, imageRGB.data);

    imageRGB.release();

    glBindTexture(GL_TEXTURE_2D, 0);
    return;
}

void Image::displayImage()
{
    shader->use();
    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, textureID);
    shader->setInt("textureMap", 0); // Ensure the shader uniform matches this texture unit

    // Draw the quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind VAO (not always necessary, but it's a good practice)
    glBindVertexArray(0);
}

void Image::save(const std::string& filename)
{
    // Assume the image dimensions are known or stored within the class. If not, they need to be provided.
    int width, height;
    // Bind the texture to read from
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Retrieve the texture size
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    // Allocate memory for reading back the texture
    auto* pixels = new unsigned char[width * height * 3]; // 3 channels for RGB

    // Get the texture data
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Convert from RGB (OpenGL) to BGR (OpenCV)
    for (int i = 0; i < width * height; ++i) {
        std::swap(pixels[i * 3], pixels[i * 3 + 2]);
    }

    // Create an OpenCV Mat from the pixel data
    cv::Mat image(height, width, CV_8UC3, pixels);

    // Save the image
    cv::imwrite(filename, image);

    // Clean up
    delete[] pixels;

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

Image::~Image()
{
    delete shader;
    return;
}