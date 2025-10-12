#include <vmm/ivec3.h>
#include <color.h>

typedef uint32_t Voxel_Type;

Voxel_Type VOX_AIR = 0;
Voxel_Type VOX_GRASS = 1;
Voxel_Type VOX_DIRT = 2;
Voxel_Type VOX_STONE = 3;
Voxel_Type VOX_WATER = 4;

typedef struct _voxel {
    Voxel_Type type;
    float refraction;
} Voxel;

typedef struct _vox_obj {
    IVector3 coord;
    ColorRGBA color;
    Voxel voxel;

    uint32_t padding[2]; // 8 bytes
} Voxel_Object;

Voxel_Object VoxelObjCreate(Voxel voxel, ColorRGBA color, IVector3 coord);
