#ifndef _VOXELREADER_H_
#define _VOXELREADER_H_

extern "C" {
    #include <color.h>
}
#include <vector>
#include <voxel.hpp>
#include <octree.hpp>

extern Voxel voxels[];
extern ColorRGBA voxelColors[];

bool load_vox_file(const char* filename, Octree* tree, int offsetX, int offsetY, int offsetZ);

#endif
