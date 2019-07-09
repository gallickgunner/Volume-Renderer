// (c) by Stefan Roettger, licensed under GPL 2+

/*  This file is taken from VVV (V^3) Volume Renderer written by Roettger. The file is modified
 *  for use as a helper function instead of as a whole tool. The original files can be found at <https://sourceforge.net/p/volren/code>
 */

#include "codebase.h"
#include "ddsbase.h"
#include "glm/vec3.hpp"

bool pvm2raw(const char* pvm_fn, const char* raw_fn, glm::vec3& voxel_size, glm::vec3& tex3D_dim);

bool pvm2raw(const char* pvm_fn, const char* raw_fn, glm::vec3& voxel_size, glm::vec3& tex3D_dim)
{
    unsigned char *volume;
    unsigned int width, height, depth, components;
    float scalex, scaley, scalez;

    printf("\nReading PVM file\n");

    volume = readPVMvolume(pvm_fn, &width, &height, &depth, &components, &scalex, &scaley, &scalez);
    if (volume == NULL)
        return false;

    tex3D_dim.x = width;
    tex3D_dim.y = height;
    tex3D_dim.z = depth;
    voxel_size.x = scalex;
    voxel_size.y = scaley;
    voxel_size.z = scalez;

    printf("Found volume with width = %d, height = %d, depth = %d, components = %d\n", width,height,depth,components);

    if (scalex != 1.0f || scaley != 1.0f || scalez != 1.0f)
      printf("and Edge Length %g/%g/%g\n",scalex,scaley,scalez);

    printf("and Data Checksum=%08X\n", checksum(volume,width*height*depth*components));

    writeRAWfile(raw_fn, volume, width*height*depth*components, TRUE);
    free(volume);
    return true;
}
