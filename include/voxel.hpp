#ifndef _VOXEL_H_
#define _VOXEL_H_
extern "C" {
    #include <vmm/ivec3.h>
    #include <color.h>
}

typedef uint32_t Voxel_Type;

struct Voxel {
    float refraction, illumination, k;
};

struct Voxel_Object {
    IVector3 coord;
    ColorRGBA color;
    Voxel voxel;
    
    float padding;
};


Voxel_Object VoxelObjCreate(Voxel voxel, ColorRGBA color, IVector3 coord);

bool voxel_compare(Voxel a, Voxel b);
bool voxel_obj_compare(Voxel_Object a, Voxel_Object b);

#endif
