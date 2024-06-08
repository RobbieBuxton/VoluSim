#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <array>
#include "shader.hpp"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    GLint materialID;

    bool operator<(const Vertex &other) const
    {
        const float *lhsArray = &Position.x;
        const float *rhsArray = &other.Position.x;

        for (int i = 0; i < 7; ++i)
        {
            if (lhsArray[i] < rhsArray[i])
                return true;
            if (lhsArray[i] > rhsArray[i])
                return false;
        }
        return false;
    }
};

struct Texture {
    unsigned int id;
    std::string type;
    bool hasAlpha;  // Add this to track if the texture has an alpha channel
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    unsigned int VAO, VBO, EBO;
    
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<Texture> textures);
    ~Mesh();

private:
    void setupMesh();
};

#endif