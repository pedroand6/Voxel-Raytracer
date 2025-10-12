#include <voxel.hpp>
#include <vmm/ivec3.h>

typedef struct _octree {
    Voxel_Object *voxel;
    struct _octree **children; //always either NULL or with 8 members
    IVector3 top_left_front, bot_right_back;
} Octree;

Octree *octree_new(void);
Octree *octree_create(IVector3 top_left_front, IVector3 bot_right_back);
void octree_insert(Octree *tree, Voxel_Object *voxel);
Voxel_Object *octree_find(Octree *tree, IVector3 coord);
void octree_delete(Octree *tree);
