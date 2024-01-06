#ifndef MAIN_H
#define MAIN_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "tracker.hpp"

void debugInitPrint(); 
void framebufferSizeCallback(GLFWwindow *window, int width, int height);
GLFWwindow *initOpenGL(GLuint pixelWidth, GLuint pixelHeight);
void processInput(GLFWwindow *window);
void pollTracker(Tracker *tracker, GLFWwindow *window);
#endif