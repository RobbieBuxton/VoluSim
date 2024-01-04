#include "display.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Display::Display(glm::vec3 origin, GLfloat width, GLfloat height, GLfloat depth, GLfloat n, GLfloat f) : width(width), height(height), depth(depth), origin(origin), n(n), f(f)
{
    // Window coords
    pa = origin + glm::vec3(-width / 2.0, 0.0f, 0.0f);
    pb = origin + glm::vec3(width / 2.0, 0.0f, 0.0f);
    pc = origin + glm::vec3(-width / 2.0, height, 0.0f);

    //Basis of the window
    vr = glm::normalize(pb - pa);
    vu = glm::normalize(pc - pa);
    vn = glm::normalize(glm::cross(vr, vu));
}

glm::mat4 Display::projectionToEye(glm::vec3 eye)
{

    glm::vec3 va = pa - eye;
    glm::vec3 vb = pb - eye;
    glm::vec3 vc = pc - eye;

    GLfloat d = -glm::dot(vn, va);

    GLfloat l = glm::dot(vr, va) * n / d;
    GLfloat r = glm::dot(vr, vb) * n / d;
    GLfloat b = glm::dot(vu, va) * n / d;
    GLfloat t = glm::dot(vu, vc) * n / d;

    // Create the projection matrix
    glm::mat4 projMatrix = glm::frustum(l, r, b, t, n, f);

    // Rotate the projection to be non-perpendicular.
    glm::mat4 rotMatrix(1.0f);
    rotMatrix[0] = glm::vec4(vr, 0);
    rotMatrix[1] = glm::vec4(vu, 0);
    rotMatrix[2] = glm::vec4(vn, 0);

    glm::mat4 finalProjMatrix = projMatrix * rotMatrix  * glm::translate(glm::mat4(1.0f), -eye);

    return finalProjMatrix;
}