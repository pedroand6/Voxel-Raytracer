#ifndef _OCTREE_H
#define _OCTREE_H

#include <voxel.hpp>

extern "C" {
    #include <color.h>
    #include <vmm/ivec3.h>
    #include <vmm/ray.h>
}

#include <stdint.h>
#include <stdlib.h>

typedef struct _octree {
    Voxel_Object voxel;
    bool has_voxel;
    struct _octree **children, *parent; //always either NULL or with 8 member/
    IVector3 left_bot_back, right_top_front; //bounding box min and max;
} Octree;

Octree *octree_new(void);
Octree *octree_create(Octree *parent, IVector3 left_bot_back, IVector3 right_top_front);
void octree_insert(Octree *tree, Voxel_Object voxel);
Voxel_Object octree_find(Octree *tree, IVector3 coord);
Octree *octree_traverse(Octree *tree, Ray ray);
uint8_t *octree_texture(Octree *tree, size_t *arr_size);
size_t _octree_texel_size(Octree *tree);
void octree_remove(Octree *tree, IVector3 coord);
void octree_delete(Octree *tree);

#endif