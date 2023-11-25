#ifndef MAIN_H
#define MAIN_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window, glm::vec3 &peChange);
#endif