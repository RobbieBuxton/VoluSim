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
    glm::vec3 tf;      // Transmission Filter
    int illum;        // Illumination Model
    glm::vec3 ka;      // Ambient Color
    glm::vec3 kd;      // Diffuse Color
    glm::vec3 ks;      // Specular Color
    glm::vec3 ke;      // Emissive Color
};

class Model 
{
    public:
        Model(const char *objPath)
        {
            loadObjFile(objPath);
        }
        Model(const char *objPath, char *texturePath)
        {
            loadObjFile(objPath);
        }
        void draw(Shader &shader);	
    private:
        // model data
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<Material> meshMaterials;
        // std::string directory;

        void loadObjFile(const std::string& objPath);
};




Texture loadTextureFile(const std::string& texturePath);  
#endif

