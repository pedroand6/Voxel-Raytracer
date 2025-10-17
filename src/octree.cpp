#include <octree.hpp>
#include <vmm/ivec3.h>
#include <vmm/ray.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#ifndef MIN_HEIGHT
#define MIN_HEIGHT -256
#endif
#ifndef MAX_HEIGHT
#define MAX_HEIGHT 256
#endif

#define CELL_COUNT (MAX_HEIGHT - MIN_HEIGHT)

#define CHILDREN_COUNT 8

enum pos_in_octree {
    LEFTBOTBACK,
    LEFTBOTFRONT,
    LEFTTOPBACK,
    LEFTTOPFRONT,
    RIGHTBOTBACK,
    RIGHTBOTFRONT,
    RIGHTTOPBACK,
    RIGHTTOPFRONT
};

IVector3 _get_node_size(Octree *node) {
    return ivec3_sub(node->right_top_front, node->left_bot_back);
}

int _get_pos_in_octree(IVector3 coord, IVector3 mid_points) {
    if (coord.x <= mid_points.x) {
        if (coord.y <= mid_points.y) {
            if (coord.z <= mid_points.z)
                return LEFTBOTBACK;
            else
                return LEFTBOTFRONT;
        }
        else {
            if (coord.z <= mid_points.z)
                return LEFTTOPBACK;
            else
                return LEFTTOPFRONT;
        }
    }
    else {
        if (coord.y <= mid_points.y) {
            if (coord.z <= mid_points.z)
                return RIGHTBOTBACK;
            else
                return RIGHTBOTFRONT;
        }
        else {
            if (coord.z <= mid_points.z)
                return RIGHTTOPBACK;
            else
                return RIGHTTOPFRONT;
        }
    }
}

bool _coord_in_space(IVector3 coord, IVector3 left_bot_back, IVector3 right_top_front) {
    return coord.x < left_bot_back.x
        || coord.x > right_top_front.x
        || coord.y < left_bot_back.y
        || coord.y > right_top_front.y
        || coord.z < left_bot_back.z
        || coord.z > right_top_front.z;
}

Octree *octree_new(void) {
    return (Octree*)calloc(1, sizeof(Octree));
}

Octree *octree_create(Octree *parent, IVector3 left_bot_back, IVector3 right_top_front) {
    Octree *ot = octree_new();
    ot->parent = parent;
    ot->left_bot_back = left_bot_back;
    ot->right_top_front = right_top_front;
    return ot;
}

Voxel_Object *octree_find(Octree *tree, IVector3 coord) {
    if(!_coord_in_space(coord, tree->left_bot_back, tree->right_top_front)) return NULL;
    if(!tree->voxel) return NULL;
    if(ivec3_equal_vec(tree->voxel->coord, coord)) return tree->voxel;
    if(!tree->children) return NULL;
    IVector3 mid_points = ivec3_scalar_div(ivec3_add(tree->left_bot_back, tree->right_top_front), 2);
    int pos = _get_pos_in_octree(coord, mid_points);
    Octree *ref = tree->children[pos];
    while(true) {
        if(!_coord_in_space(coord, ref->left_bot_back, ref->right_top_front)) return NULL;
        if(!ref->voxel) return NULL;
        if(ivec3_equal_vec(ref->voxel->coord, coord)) return ref->voxel;
        if(!ref->children) return NULL;
        mid_points = ivec3_scalar_div(ivec3_add(ref->left_bot_back, ref->right_top_front), 2);
        pos = _get_pos_in_octree(coord, mid_points);
        ref = ref->children[pos];
    }
    return NULL;
}

void _create_children(Octree *tree, IVector3 mid_points) {
    tree->children = (Octree**)malloc(sizeof(Octree*) * CHILDREN_COUNT);
    if(!tree->children) return; //FIXME: no err handling? :(
    IVector3 min = tree->left_bot_back;
    IVector3 max = tree->right_top_front;
    IVector3 mid = ivec3_scalar_div(ivec3_add(min, max), 2);
    tree->children[LEFTBOTBACK]   = octree_create(tree, min, mid);
    tree->children[LEFTBOTFRONT]  = octree_create(tree, {min.x, min.y, mid.z},
                                                        {mid.x, mid.y, max.z});
    tree->children[LEFTTOPBACK]   = octree_create(tree, {min.x, mid.y, min.z},
                                                        {mid.x, max.y, mid.z});
    tree->children[LEFTTOPFRONT]  = octree_create(tree, {min.x, mid.y, mid.z},
                                                        {mid.x, max.y, max.z});
    tree->children[RIGHTBOTBACK]  = octree_create(tree, {mid.x, min.y, min.z},
                                                        {max.x, mid.y, mid.z});
    tree->children[RIGHTBOTFRONT] = octree_create(tree, {mid.x, min.y, mid.z},
                                                        {max.x, mid.y, max.z});
    tree->children[RIGHTTOPBACK]  = octree_create(tree, {mid.x, mid.y, min.z},
                                                        {max.x, max.y, mid.z});
    tree->children[RIGHTTOPFRONT] = octree_create(tree, mid, max);
    for(int i = 0; i < CHILDREN_COUNT; i++) tree->children[i]->voxel = NULL;
    int pos = _get_pos_in_octree(tree->voxel->coord, mid_points);
    tree->children[pos]->voxel = tree->voxel;
    tree->voxel = (Voxel_Object*)(sizeof(Voxel_Object));
    if(!tree->voxel) return;
    tree->voxel->coord.y = MIN_HEIGHT;
}

void octree_insert(Octree *tree, Voxel_Object *voxel) {
    if(octree_find(tree, voxel->coord)) return;
    if(!tree) return;
    Octree *curr = tree;
    while(true) {
        if(!_coord_in_space(voxel->coord, curr->left_bot_back, curr->right_top_front)) return;
        if(!curr->voxel) {
            curr->voxel = voxel;
            return;
        }
        if(ivec3_equal_vec(curr->voxel->coord, voxel->coord)) return;
        IVector3 mid_points = ivec3_scalar_div(ivec3_add(curr->left_bot_back, curr->right_top_front), 2);
        int pos = _get_pos_in_octree(voxel->coord, mid_points);
        if(curr->voxel->coord.y > MIN_HEIGHT || !curr->children) {
            _create_children(curr, mid_points);
            curr->children[pos]->voxel = voxel;
            return;
        }
        curr = curr->children[pos];
    }
}

//TODO: void octree_remove(IVector3 coord);

void octree_delete(Octree *tree) {
    if(!tree) return;
    for(int i = 0; i < CHILDREN_COUNT; i++) octree_delete(tree);
    free(tree->voxel);
    free(tree->children);
    free(tree);
}

Octree *_get_neighbour(Octree *node, IVector3 dir) {
    IVector3 node_size = _get_node_size(node);

    Octree *parent = node->parent;
    IVector3 coord_find = ivec3_add(node->left_bot_back, dir);
    while(parent && !_coord_in_space(coord_find, parent->left_bot_back, parent->right_top_front)) parent = parent->parent;
    if(!parent) return NULL; //got out of octree
    IVector3 parent_size = _get_node_size(parent);
    while(!ivec3_equal_vec(parent_size, node_size)) {
        if(!parent->children) break;
        if(!_coord_in_space(coord_find, parent->left_bot_back, parent->right_top_front)) break;
        for(int i = 0; i < CHILDREN_COUNT; i++){
            if(_coord_in_space(coord_find, parent->children[i]->left_bot_back, parent->children[i]->right_top_front)) {
                parent = parent->children[i];
                break;
            }
        }
    }
    return parent;
}

Octree *octree_dda(Octree *tree, Ray ray) {
    float tmin;
    float tmax;

    Vector3 cell_count = vec3_float(CELL_COUNT, CELL_COUNT, CELL_COUNT);


    /// make sure the ray hits the bounding box of the root octree node
    if (!ray_hits_box(ray, vec3_ivec3(tree->left_bot_back), vec3_ivec3(tree->right_top_front), &tmin, &tmax))
        return NULL;


    /// move the ray position to the point of intersection with the bounding volume.
    ray.origin = vec3_scalar_mul(ray.direction, fmin(tmin, tmax));

    /// get integer cell coordinates for the given position
    ///     leafSize is a Vector3 containing the dimensions of a leaf node in world-space coordinates
    ///     cellCount is a Vector3 containng the number of cells in each direction, or the size of the tree root divided by leafSize.
    Vector3 cell = vec3_min(vec3_ivec3(ivec3_vec3(vec3_sub(ray.origin, vec3_ivec3(tree->left_bot_back)))), vec3_sub(cell_count, vec3_one()));


    /// get the Vector3 where of the intersection point relative to the tree root.
    Vector3 pos = vec3_sub(ray.origin, vec3_ivec3(tree->left_bot_back));

    /// get the bounds of the starting cell - leaf size offset by "pos"
    Vector3 cell_lbb = vec3_float(cell.x - 0.5, cell.y - 0.5, cell.z - 0.5);
    Vector3 cell_rtf = vec3_float(cell.x + 0.5, cell.y + 0.5, cell.z + 0.5);

    /// calculate initial t values for each axis based on the sign of the ray.
    /// See any good 3D DDA tutorial for an explanation of t, but it basically tells us the 
    /// distance we have to move from ray.origin along ray.direction to reach the next cell boundary
    /// This calculates t values for both positive and negative ray directions.
    Vector3 tMaxNeg = vec3_div(vec3_sub(cell_lbb, ray.origin), ray.direction);
    Vector3 tMaxPos = vec3_div(vec3_sub(cell_rtf, ray.origin), ray.direction);

    /// calculate t values within the cell along the ray direction.
    /// This may be buggy as it seems odd to mix and match ray directions
    Vector3 t_max = vec3_float(
        ray.direction.x < 0 ? tMaxNeg.x : tMaxPos.x
        ,
        ray.direction.y < 0 ? tMaxNeg.y : tMaxPos.y
        ,
        ray.direction.z < 0 ? tMaxNeg.z : tMaxPos.z
    );

    /// get cell coordinate step directions
    /// .Sign() is an extension method that returns a Vector3 with each component set to +/- 1
    Vector3 step = vec3_sign(ray.direction);

    /// calculate distance along the ray direction to move to advance from one cell boundary 
    /// to the next on each axis. Assumes ray.direction is normalized.
    /// Takes the absolute value of each ray component since this value is in units along the
    /// ray direction, which makes sure the sign is correct.
    Vector3 t_delta = vec3_abs(vec3_div(vec3_one(), ray.direction));

    /// neighbor node indices to use when exiting cells
    Vector3 neighbour_dirs[] = {
        (step.x < 0) ? vec3_left() : vec3_right(),
        (step.y < 0) ? vec3_down() : vec3_up(),
        (step.z < 0) ? vec3_back() : vec3_forward(),
    };

    Octree *node = tree;

    /// step across the bounding volume, generating a marker entity at each
    /// cell that we touch. Extension methods GreaterThanOrEEqual and LessThan
    /// ensure that we stay within the bounding volume.
    while (node) {
        /// if the current node isn't a leaf, find one.
        /// this version exits when it encounters the first leaf.
        if(node->children)
            for(int i = 0; i < CHILDREN_COUNT; i++) {
                Octree *child = node->children[i];
                if(child && child->voxel && octree_find(child, ivec3_vec3(cell))) {
                    node = child;
                    i = -1;

                    if(!node->children)
                        return node;
                }
            }

        /// index into the node's Neighbor[] array to move
        int dir = 0;

        /// This is off-the-shelf DDA.
        if(t_max.x < t_max.y) {
            if(t_max.x < t_max.z) {
                t_max.x += t_delta.x;
                cell.x += step.x;
                dir = 0;
            }
            else {
                t_max.z += t_delta.z;
                cell.z += step.z;
                dir = 2;
            }
        }
        else {
            if(t_max.y < t_max.z) {
                t_max.y += t_delta.y;
                cell.y += step.y;
                dir = 1;
            }
            else {
                t_max.z += t_delta.z;
                cell.z += step.z;
                dir = 2;
            }
        }

        /// see if the new cell coordinates fall within the current node.
        /// this is important when moving from a leaf into empty space within 
        /// the tree.
        if(!octree_find(node, ivec3_vec3(cell))) {
            /// if we stepped out of this node, grab the appropriate neighbor. 
            Vector3 neighbor_dir = neighbour_dirs[dir];
            node = _get_neighbour(node, ivec3_vec3(neighbor_dir));
        }
        else if(!node->children)
            return node;
    }
    return NULL;
}

