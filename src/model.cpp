
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include <string>
#include <iostream>
#include <map>
#include <glm/gtx/string_cast.hpp>

#include "tiny_obj_loader.h"
#include "filesystem.hpp"
#include "model.hpp"
#include "mesh.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


// Texture loadTextureFile(const std::string& texturePath) {
//     // load and create a texture 
//     // -------------------------
//     unsigned int textureId;
//     // texture 1
//     // ---------
//     glGenTextures(1, &textureId);
//     glBindTexture(GL_TEXTURE_2D, textureId);
//     // set the texture wrapping parameters
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//     // set texture filtering parameters
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     // load image, create texture and generate mipmaps
//     int width, height, nrChannels;
//     stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
//     unsigned char *data = stbi_load(FileSystem::getPath(texturePath).c_str(), &width, &height, &nrChannels, 0);
//     if (data)
//     {
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//         glGenerateMipmap(GL_TEXTURE_2D);
//     }
//     else
//     {
//         std::cout << "Failed to load texture" << std::endl;
//     }
//     stbi_image_free(data);
//     return Texture{textureId, "crate"};
// }

void Model::loadObjFile(const std::string &objPath)
{

    std::string inputfile = FileSystem::getPath(objPath).c_str();
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = FileSystem::getPath("data/resources/materials/").c_str();
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config))
    {
        if (!reader.Error().empty())
        {
            std::cout << "TinyObjReader: " << reader.Error() << std::endl;
        }
        exit(1);
    }
    if (!reader.Warning().empty())
    {
        std::cout << "TinyObjReader: " << reader.Warning() << std::endl;
    }
    tinyobj::attrib_t attributes = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
    std::vector<tinyobj::material_t> materials = reader.GetMaterials();
    std::cout << "Shape Number: " << shapes.size() << std::endl;
    std::cout << "Materials Number: " << materials.size() << std::endl;

    for (const auto &shape : shapes)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture> textures;
        std::map<Vertex, uint32_t> uniqueVertices;
        for (const auto &index : shape.mesh.indices)
        {
            Vertex newVertex;
            newVertex.Position = glm::vec3(
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]);

            if (index.normal_index >= 0)
            {
                newVertex.Normal = glm::vec3(
                    attributes.normals[index.normal_index * 3],
                    attributes.normals[index.normal_index * 3 + 1],
                    attributes.normals[index.normal_index * 3 + 2]);
            } else {
                newVertex.Normal = glm::vec3(0,0,0); //This is placeholder
            }

            if (index.texcoord_index >= 0)
            {
                newVertex.TexCoords = glm::vec2(
                    attributes.texcoords[index.texcoord_index * 2],
                    attributes.texcoords[index.texcoord_index * 2 + 1]);
            } else {
                newVertex.TexCoords = glm::vec2(0,0); //This is placeholder
            }


            if (uniqueVertices.count(newVertex) == 0)
            {
                uniqueVertices[newVertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(Vertex{newVertex});
            }

            indices.push_back(uniqueVertices[newVertex]);
        }
        std::cout << "Vertices Number: " << vertices.size() << " Indicies Number: " << indices.size() << std::endl;
        meshes.push_back(std::make_shared<Mesh>(vertices, indices, textures));
    }
}

void Model::Draw(Shader &shader)
{   
    // std::cout << meshes.size() << std::endl;
    for(unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i]->Draw(shader);
    }
} 
