
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include <string>
#include <iostream>
#include <map>
#include <glm/gtx/string_cast.hpp>

#include "tiny_obj_loader.h"
#include "filesystem.hpp"
#include "object_loader.hpp"
#include "mesh.hpp"

Mesh loadObjFile(const std::string &path)
{

    std::string inputfile = FileSystem::getPath(path).c_str();
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;
    std::map<Vertex, uint32_t> uniqueVertices;

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            Vertex newVertex;
            newVertex.Position = glm::vec3(
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]);

            newVertex.Normal = glm::vec3(
                attributes.normals[index.normal_index * 3],
                attributes.normals[index.normal_index * 3 + 1],
                attributes.normals[index.normal_index * 3 + 2]);

            newVertex.TexCoords = glm::vec2(
                attributes.texcoords[index.texcoord_index * 2],
                attributes.texcoords[index.texcoord_index * 2 + 1]);

            if (uniqueVertices.count(newVertex) == 0)
            {
                uniqueVertices[newVertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(Vertex{newVertex});
            }

            indices.push_back(uniqueVertices[newVertex]);
        }
    }
    std::cout << "Vertices Number: " << vertices.size() << " Indicies Number: " << indices.size() << std::endl;
    return Mesh{vertices, indices, textures};
}
