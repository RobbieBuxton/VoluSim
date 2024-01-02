#ifndef MAIN_H
#define MAIN_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "tracker.hpp"

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
GLFWwindow *initOpenGL();
void processInput(GLFWwindow *window);
void pollTracker(Tracker *tracker, GLFWwindow *window);
#endif