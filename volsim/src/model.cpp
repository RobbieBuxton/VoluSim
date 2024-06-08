#define TINYOBJLOADER_IMPLEMENTATION 
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

Texture loadTextureFile(const std::string &texturePath, const std::string type, bool isAlphaMap = false)
{
	std::string fileSystemTexturePath = "data/resources/textures/" + texturePath;
	
    unsigned int textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(FileSystem::getPath(fileSystemTexturePath).c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = isAlphaMap ? GL_RED : (nrChannels == 4 ? GL_RGBA : GL_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture{textureId, type};
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
	std::vector<Texture> textures;


    for (const auto &mat : materials)
    {
        Material newMat;

        newMat.name = mat.name;
        newMat.ns = mat.shininess;
        newMat.ni = mat.ior;
        newMat.d = mat.dissolve;
        newMat.tr = 1.0f - mat.dissolve;
        newMat.tf = glm::vec3(mat.transmittance[0], mat.transmittance[1], mat.transmittance[2]);
        newMat.illum = mat.illum;
        newMat.ka = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        newMat.kd = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        newMat.ks = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        newMat.ke = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);

        // Load textures
        if (!mat.ambient_texname.empty())
        {
            textures.push_back(loadTextureFile(mat.ambient_texname, "ambientTexture", false));
        }
        if (!mat.diffuse_texname.empty())
        {
            textures.push_back(loadTextureFile(mat.diffuse_texname, "diffuseTexture",false));
        }
        if (!mat.alpha_texname.empty())
        {
            textures.push_back(loadTextureFile(mat.alpha_texname, "alphaTexture", true));
        } else {
			std::cout << "No alpha texture found" << std::endl;
		}

        meshMaterials.push_back(newMat);
    }

    for (const auto &shape : shapes)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::map<Vertex, uint32_t> uniqueVertices;

        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f];
            int currentMaterialID = shape.mesh.material_ids[f];

            for (int v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                Vertex newVertex;

                newVertex.Position = glm::vec3(
                    attributes.vertices[3 * idx.vertex_index + 0],
                    attributes.vertices[3 * idx.vertex_index + 1],
                    attributes.vertices[3 * idx.vertex_index + 2]);

                if (idx.normal_index >= 0)
                {
                    newVertex.Normal = glm::vec3(
                        attributes.normals[3 * idx.normal_index + 0],
                        attributes.normals[3 * idx.normal_index + 1],
                        attributes.normals[3 * idx.normal_index + 2]);
                }
                else
                {
                    newVertex.Normal = glm::vec3(0, 0, 0);
                }

                if (idx.texcoord_index >= 0)
                {
                    newVertex.TexCoords = glm::vec2(
                        attributes.texcoords[2 * idx.texcoord_index + 0],
                        attributes.texcoords[2 * idx.texcoord_index + 1]);
                }
                else
                {
                    newVertex.TexCoords = glm::vec2(0, 0);
                }

                newVertex.materialID = currentMaterialID;

                if (uniqueVertices.count(newVertex) == 0)
                {
                    uniqueVertices[newVertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(newVertex);
                }

                indices.push_back(uniqueVertices[newVertex]);
            }

            index_offset += fv;
        }

        meshes.push_back(std::make_shared<Mesh>(vertices, indices, textures));
    }
}
void Model::draw(Shader &shader)
{
    for (int i = 0; i < (int)meshMaterials.size(); ++i)
    {
        shader.setVec3("ambient", meshMaterials[i].ka, i);
        shader.setVec3("diffuse", meshMaterials[i].kd, i);
        shader.setVec3("specular", meshMaterials[i].ks, i);
        shader.setFloat("shininess", meshMaterials[i].ns, i);
    }

    for (const auto &mesh : meshes)
    {
        bool hasAlphaMap = false;

        for (unsigned int j = 0; j < mesh->textures.size(); ++j)
        {
            if (mesh->textures[j].type == "ambientTexture")
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh->textures[j].id);
                shader.setInt("ambientTexture", 0);
            }
            else if (mesh->textures[j].type == "diffuseTexture")
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mesh->textures[j].id);
                shader.setInt("diffuseTexture", 1);
            }
            else if (mesh->textures[j].type == "alphaTexture")
            {
                hasAlphaMap = true;
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, mesh->textures[j].id);
                shader.setInt("alphaTexture", 2);
            }
        }
        shader.setBool("useAlphaMap", hasAlphaMap);

        glBindVertexArray(mesh->VAO);
        glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
}