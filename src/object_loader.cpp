
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include <string>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include "tiny_obj_loader.h"
#include "filesystem.hpp"
#include "object_loader.hpp"
#include "mesh.hpp"

int loader()
{
    std::string inputfile = FileSystem::getPath("data/resources/models/crate.obj").c_str();
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = FileSystem::getPath("data/resources/materials/").c_str(); // Path to material files
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
    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    auto &materials = reader.GetMaterials();
    std::cout << "Shape Number: " << shapes.size() << std::endl;
    std::cout << "Materials Number: " << materials.size() << std::endl;


    // Loop over shapes
    std::cout << shapes.size() << std::endl;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        std::cout << "Shape Faces: " << shapes[s].mesh.num_face_vertices.size() << std::endl;

        std::vector<Vertex> vertices;

        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            std::cout << "Faces Vertices: " << fv << std::endl;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                // Check if `normal_index` is zero or positive. negative = no normal data
                tinyobj::real_t nx, ny, nz;
                if (idx.normal_index >= 0)
                {
                    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                } else {
                    std::cout << "ERROR: Model has no normal vec" << std::endl;
                    abort();
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                tinyobj::real_t tx, ty;
                if (idx.texcoord_index >= 0)
                {
                    tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                } else {
                    std::cout << "ERROR: Model has no tex coords" << std::endl;
                    abort();
                }

                // Create a Vertex instance and set its members.
                Vertex newVertex;
                newVertex.Position = glm::vec3(vx, vy, vz);  // Replace with your values.
                newVertex.Normal = glm::vec3(nx, ny, nz);    // Replace with your values.
                newVertex.TexCoords = glm::vec2(tx, ty);     // Replace with your values.

                std::cout << glm::to_string(newVertex.Position) << ", " << glm::to_string(newVertex.Normal) << ", " << glm::to_string(newVertex.TexCoords) << ", " <<std::endl;


                // vertices.push_back(vector{})
                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
            }
            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];
        }
    }
    return 0;
}
