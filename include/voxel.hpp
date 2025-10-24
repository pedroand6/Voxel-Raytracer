#ifndef _VOXEL_H_
#define _VOXEL_H_
#include <vmm/ivec3.h>
#include <color.h>

typedef uint32_t Voxel_Type;

Voxel_Type VOX_AIR = 0;
Voxel_Type VOX_GRASS = 1;
Voxel_Type VOX_DIRT = 2;
Voxel_Type VOX_STONE = 3;
Voxel_Type VOX_WATER = 4;

struct Voxel {
    Voxel_Type type;
    float refraction;
};

struct Voxel_Object {
    IVector3 coord;
    ColorRGBA color;
    Voxel voxel;

    uint32_t padding[2]; // 8 bytes
};

Voxel_Object VoxelObjCreate(Voxel voxel, ColorRGBA color, IVector3 coord);
#endif // _VOXEL_H_