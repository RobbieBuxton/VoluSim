#include "display.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Display::Display(glm::vec3 origin, GLfloat width, GLfloat height, GLfloat depth, GLfloat n, GLfloat f) : width(width), height(height), depth(depth), origin(origin), n(n), f(f)
{
    // Screen corner coords
    pa = origin + glm::vec3(-width / 2.0, 0.0f, 0.0f);
    pb = origin + glm::vec3(width / 2.0, 0.0f, 0.0f);
    pc = origin + glm::vec3(-width / 2.0, height, 0.0f);

    // Orthonormal basis of the screen
    sr = glm::normalize(pb - pa);
    su = glm::normalize(pc - pa);
    sn = glm::normalize(glm::cross(sr, su));
}

glm::mat4 Display::projectionToEye(glm::vec3 eye)
{

    // Vectors from eye to opposite screen corners
    glm::vec3 vb = pb - eye;
    glm::vec3 vc = pc - eye;

    // Distance from eye to screen
    GLfloat d = -glm::dot(sn, vc);

    // Frustum extents (scaled to the near clipping plane)
    GLfloat l = glm::dot(sr, vc) * n / d;
    GLfloat r = glm::dot(sr, vb) * n / d;
    GLfloat b = glm::dot(su, vb) * n / d;
    GLfloat t = glm::dot(su, vc) * n / d;

    // Create the projection matrix
    glm::mat4 projMatrix = glm::frustum(l, r, b, t, n, f);

    // Rotate the projection to be aligned with screen basis.
    glm::mat4 rotMatrix(1.0f);
    rotMatrix[0] = glm::vec4(sr, 0);
    rotMatrix[1] = glm::vec4(su, 0);
    rotMatrix[2] = glm::vec4(sn, 0);

    // Translate the world so the eye is at the origin of the viewing frustum
    glm::mat4 transMatrix = glm::translate(glm::mat4(1.0f), -eye);

    return projMatrix * rotMatrix * transMatrix;
}