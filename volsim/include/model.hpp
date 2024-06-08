#ifndef OBJECT_LOADER_H
#define OBJECT_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include "mesh.hpp"

struct Material {
    std::string name;
    float ns;         // Specular Exponent
    float ni;         // Optical Density (Index of Refraction)
    float d;          // Dissolve (Transparency)
    float tr;         // Transparency
    glm::vec3 tf;     // Transmission Filter
    int illum;        // Illumination Model
    glm::vec3 ka;     // Ambient Color
    glm::vec3 kd;     // Diffuse Color
    glm::vec3 ks;     // Specular Color
    glm::vec3 ke;     // Emissive Color

	unsigned int ambientTextureID = 0;
    unsigned int diffuseTextureID = 0;
    unsigned int alphaTextureID = 0;
};

class Model 
{
    public:
        Model(const char *objPath)
        {
            loadObjFile(objPath);
        }
        Model(const std::string &objPath, std::vector<Texture> textures)
        {
            loadObjFile(objPath);
        }
        void draw(Shader &shader);	
    private:
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<Material> meshMaterials;

        void loadObjFile(const std::string& objPath);
};

Texture loadTextureFile(const std::string &texturePath, const std::string type,  bool isAlphaMap);

#endif