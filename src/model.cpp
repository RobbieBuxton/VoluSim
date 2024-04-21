
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

Texture loadTextureFile(const std::string &texturePath)
{
    // load and create a texture
    // -------------------------
    unsigned int textureId;
    // texture 1
    // ---------
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load(FileSystem::getPath(texturePath).c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return Texture{textureId, "crate"};
}

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

    for (const auto &mat : materials)
    {
        Material newMat;

        newMat.name = mat.name;
        newMat.ns = mat.shininess;
        newMat.ni = mat.ior;
        newMat.d = mat.dissolve;
        newMat.tr = 1.0f - mat.dissolve; // need to investigate this
        newMat.tf = glm::vec3(mat.transmittance[0], mat.transmittance[1], mat.transmittance[2]);
        newMat.illum = mat.illum;
        newMat.ka = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        newMat.kd = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        newMat.ks = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        newMat.ke = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);

        // Add the new material to your custom materials list
        meshMaterials.push_back(newMat);
    }

    for (const auto &shape : shapes)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture> textures;
        std::map<Vertex, uint32_t> uniqueVertices;

        // Loop over faces (polygon)
        size_t index_offset = 0; // the offset in the 'indices' array
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f];

            // Retrieve the material ID for this face
            int currentMaterialID = shape.mesh.material_ids[f];
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                Vertex newVertex;

                // Vertex position
                newVertex.Position = glm::vec3(
                    attributes.vertices[3 * idx.vertex_index + 0],
                    attributes.vertices[3 * idx.vertex_index + 1],
                    attributes.vertices[3 * idx.vertex_index + 2]);

                // Vertex normal
                if (idx.normal_index >= 0)
                {
                    newVertex.Normal = glm::vec3(
                        attributes.normals[3 * idx.normal_index + 0],
                        attributes.normals[3 * idx.normal_index + 1],
                        attributes.normals[3 * idx.normal_index + 2]);
                }
                else
                {
                    newVertex.Normal = glm::vec3(0, 0, 0); // Placeholder
                }

                // Vertex texture coordinates
                if (idx.texcoord_index >= 0)
                {
                    newVertex.TexCoords = glm::vec2(
                        attributes.texcoords[2 * idx.texcoord_index + 0],
                        attributes.texcoords[2 * idx.texcoord_index + 1]);
                }
                else
                {
                    newVertex.TexCoords = glm::vec2(0, 0); // Placeholder
                }

                // Assign material ID to vertex
                newVertex.materialID = currentMaterialID;

                // Add the vertex, or get the index of the existing one
                if (uniqueVertices.count(newVertex) == 0)
                {
                    uniqueVertices[newVertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(newVertex);
                }

                // Add the index
                indices.push_back(uniqueVertices[newVertex]);
            }

            index_offset += fv; // move to next face
        }

        // Create mesh with vertices, indices, textures, and material ID
        meshes.push_back(std::make_shared<Mesh>(vertices, indices, textures));
    }
}

void Model::draw(Shader &shader)
{
    // Assuming you have a std::vector<Material> myMaterials with your material data
    for (int i = 0; i < meshMaterials.size(); ++i)
    {
        shader.setVec3("ambient", meshMaterials[i].ka, i);
        shader.setVec3("diffuse", meshMaterials[i].kd, i);
        shader.setVec3("specular", meshMaterials[i].ks, i);
        shader.setInt("shininess", meshMaterials[i].ns, i);
    }

    for (const auto &mesh : meshes)
    {
        glBindVertexArray(mesh->VAO);
        glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}
