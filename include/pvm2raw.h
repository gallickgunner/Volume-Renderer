// (c) by Stefan Roettger, licensed under GPL 2+

/*  This file is taken from VVV (V^3) Volume Renderer written by Roettger. The file is modified
 *  for use as a helper function instead of as a whole tool. The original files can be found at <https://sourceforge.net/p/volren/code>
 */

#include "codebase.h"
#include "ddsbase.h"
#include "glm/vec3.hpp"
#include <string>
#include <fstream>

bool pvm2raw(const std::string& pvm_fn, std::string& raw_fn, glm::vec3& voxel_size, glm::ivec3& tex3D_dim);

bool pvm2raw(const std::string& pvm_fn, std::string& raw_fn, glm::vec3& voxel_size, glm::ivec3& tex3D_dim)
{
    unsigned char *volume;
    unsigned int width, height, depth, components;
    float scalex, scaley, scalez;

    volume = readPVMvolume(pvm_fn.c_str(), &width, &height, &depth, &components, &scalex, &scaley, &scalez);
    if (volume == NULL)
        return false;

    tex3D_dim.x = width;
    tex3D_dim.y = height;
    tex3D_dim.z = depth;
    voxel_size.x = scalex;
    voxel_size.y = scaley;
    voxel_size.z = scalez;

    raw_fn = pvm_fn.substr(0, pvm_fn.length() - 4);
    raw_fn = raw_fn + "-" + std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + ".raw";

    //Write a raw file if not already present
    std::ifstream file;
    file.open(raw_fn);
    if(!file)
        writeRAWfile(raw_fn.c_str(), volume, width*height*depth*components, TRUE);
    free(volume);
    return true;
}
