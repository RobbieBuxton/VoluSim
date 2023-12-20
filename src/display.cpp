#include "display.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Display::Display(glm::vec3 origin, GLfloat width, GLfloat height, GLfloat depth, GLfloat nearClip, GLfloat farClip) : width(width), height(height), depth(depth), origin(origin), nearClip(nearClip), farClip(farClip)
{
    pa = origin + glm::vec3(-width / 2, -height / 2, 0.0f);
    pb = origin + glm::vec3(width / 2, -height / 2, 0.0f);
    pc = origin + glm::vec3(-width / 2, height / 2, 0.0f);
    vr = glm::normalize(pb - pa);
    vu = glm::normalize(pc - pa);
    vn = glm::normalize(glm::cross(vr, vu));
}

glm::mat4 Display::projectionToEye(glm::vec3 eye){
    glm::vec3 va = pa - eye;
    glm::vec3 vb = pb - eye;
    glm::vec3 vc = pc - eye;

    GLfloat d = -glm::dot(vn, va);

    GLfloat scaleFactor = nearClip / d;
    GLfloat l = glm::dot(vr, va) * scaleFactor;
    GLfloat r = glm::dot(vr, vb) * scaleFactor;
    GLfloat b = glm::dot(vu, va) * scaleFactor;
    GLfloat t = glm::dot(vu, vc) * scaleFactor;

    // This is missing the required basis change (i.e assuming the screen will be already on the unit vectors)
    glm::mat4 projection = glm::translate(glm::frustum(l, r, b, t, nearClip, farClip), glm::vec3(-eye.x, -eye.y, -eye.z));
    return projection;
}