#ifndef _OCTREE_H
#define _OCTREE_H

#include <voxel.hpp>
#include <vmm/ivec3.h>
#include <vmm/ray.h>

typedef struct _octree {
    Voxel_Object *voxel;
    struct _octree **children, *parent; //always either NULL or with 8 member/
    IVector3 left_bot_back, right_top_front; //bounding box min and max;
} Octree;

Octree *octree_new(void);
Octree *octree_create(Octree *parent, IVector3 left_bot_back, IVector3 right_top_front);
void octree_insert(Octree *tree, Voxel_Object *voxel);
Voxel_Object *octree_find(Octree *tree, IVector3 coord);
Octree *octree_traverse(Octree *tree, Ray ray);
void octree_delete(Octree *tree);

#endif