#include <voxel.hpp>

Voxel_Object VoxelObjCreate(Voxel voxel, ColorRGBA color, IVector3 coord) {
    Voxel_Object obj;
    obj.voxel = voxel;
    obj.color = color;
    obj.coord = coord;
    return obj;
}