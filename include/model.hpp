#ifndef OBJECT_LOADER_H
#define OBJECT_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include "mesh.hpp"

class Model 
{
    public:
        Model(char *objPath)
        {
            loadObjFile(objPath);
        }
        Model(char *objPath, char *texturePath)
        {
            loadObjFile(objPath);
            loadTextureFile(texturePath);  
        }
        void Draw(Shader &shader);	
    private:
        // model data
        std::vector<std::shared_ptr<Mesh>> meshes;
        // std::string directory;

        void loadObjFile(const std::string& objPath);
        Texture loadTextureFile(const std::string& texturePath);  
};

#endif

