extern "C" {
    #include <vmm/ivec3.h>
    #include <vmm/vec3.h>
    #include <vmm/ray.h>
}
#include <octree.hpp>
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

#define LEAF_SIZE 2

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

Voxel_Object _invalid_voxel(void) {
    Voxel_Object a = {0};
    a.coord.y = MIN_HEIGHT;
    return a;
}

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

Voxel_Object octree_find(Octree *tree, IVector3 coord) {
    if(!_coord_in_space(coord, tree->left_bot_back, tree->right_top_front)) return _invalid_voxel();
    if(ivec3_equal_vec(tree->voxel.coord, coord)) return tree->voxel;
    if(!tree->children) return _invalid_voxel();
    IVector3 mid_points = ivec3_scalar_div(ivec3_add(tree->left_bot_back, tree->right_top_front), 2);
    int pos = _get_pos_in_octree(coord, mid_points);
    Octree *ref = tree->children[pos];
    while(true) {
        if(!_coord_in_space(coord, ref->left_bot_back, ref->right_top_front)) return _invalid_voxel();
        if(ivec3_equal_vec(ref->voxel.coord, coord)) return ref->voxel;
        if(!ref->children) return _invalid_voxel();
        mid_points = ivec3_scalar_div(ivec3_add(ref->left_bot_back, ref->right_top_front), 2);
        pos = _get_pos_in_octree(coord, mid_points);
        ref = ref->children[pos];
    }
    return _invalid_voxel();
}

int _create_children(Octree *tree, IVector3 mid_points) {
    tree->children = (Octree**)malloc(sizeof(Octree*) * CHILDREN_COUNT);
    if(!tree->children) return -1;
    IVector3 min = tree->left_bot_back;
    IVector3 max = tree->right_top_front;
    IVector3 mid = ivec3_scalar_div(ivec3_add(min, max), 2);
    tree->children[LEFTBOTBACK]   = octree_create(tree, min, mid);
    tree->children[LEFTBOTFRONT]  = octree_create(tree, {{min.x, min.y, mid.z}},
                                                        {{mid.x, mid.y, max.z}});
    tree->children[LEFTTOPBACK]   = octree_create(tree, {{min.x, mid.y, min.z}},
                                                        {{mid.x, max.y, mid.z}});
    tree->children[LEFTTOPFRONT]  = octree_create(tree, {{min.x, mid.y, mid.z}},
                                                        {{mid.x, max.y, max.z}});
    tree->children[RIGHTBOTBACK]  = octree_create(tree, {{mid.x, min.y, min.z}},
                                                        {{max.x, mid.y, mid.z}});
    tree->children[RIGHTBOTFRONT] = octree_create(tree, {{mid.x, min.y, mid.z}},
                                                        {{max.x, mid.y, max.z}});
    tree->children[RIGHTTOPBACK]  = octree_create(tree, {{mid.x, mid.y, min.z}},
                                                        {{max.x, max.y, mid.z}});
    tree->children[RIGHTTOPFRONT] = octree_create(tree, mid, max);
    for(int i = 0; i < CHILDREN_COUNT; i++) tree->children[i]->voxel = {0};
    int pos = _get_pos_in_octree(tree->voxel.coord, mid_points);
    tree->children[pos]->voxel = tree->voxel;
    tree->voxel = _invalid_voxel();
    tree->voxel.coord.y = MIN_HEIGHT;
    return 0;
}

void octree_insert(Octree *tree, Voxel_Object voxel) {
    if(voxel_obj_compare(octree_find(tree, voxel.coord), voxel)) return;
    if(!tree) return;
    Octree *curr = tree;
    while(true) {
        if(!_coord_in_space(voxel.coord, curr->left_bot_back, curr->right_top_front)) return;
        if(!curr->has_voxel) {
            curr->voxel = voxel;
            return;
        }
        if(ivec3_equal_vec(curr->voxel.coord, voxel.coord)) return;
        IVector3 mid_points = ivec3_scalar_div(ivec3_add(curr->left_bot_back, curr->right_top_front), 2);
        int pos = _get_pos_in_octree(voxel.coord, mid_points);
        if(!curr->children) {
            _create_children(curr, mid_points);
            curr->children[pos]->voxel = voxel;
            return;
        }
        curr = curr->children[pos];
    }
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

Octree *octree_traverse(Octree *tree, Ray ray) {
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
                if(child && child->has_voxel && voxel_obj_compare(octree_find(child, ivec3_vec3(cell)), _invalid_voxel())) {
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
        if(!voxel_obj_compare(octree_find(node, ivec3_vec3(cell)), _invalid_voxel())) {
            /// if we stepped out of this node, grab the appropriate neighbor. 
            Vector3 neighbor_dir = neighbour_dirs[dir];
            node = _get_neighbour(node, ivec3_vec3(neighbor_dir));
        }
        else if(!node->children)
            return node;
    }
    return NULL;
}

size_t _octree_texel_size(Octree *tree) {
    if(!tree) return 0;
    if(!tree->children) return LEAF_SIZE;
    size_t total = 1;
    for(int i = 0; i < CHILDREN_COUNT; i++) {
        total += _octree_texel_size(tree->children[i]);
    }
    return total;
}

void _encode_pointer(int block_index, uint8_t *out_voxel, size_t tex_dim) {
    int z = block_index / (tex_dim * tex_dim);
    int y = (block_index / tex_dim) % tex_dim;
    int x = block_index % tex_dim;

    out_voxel[0] = (uint8_t)x;
    out_voxel[1] = (uint8_t)y;
    out_voxel[2] = (uint8_t)z;
    out_voxel[3] = 0; //Flag for internal node
}

void _transform_node_to_texture(Octree *node, 
                                uint8_t *texture, 
                                size_t *next_free_block, 
                                size_t tex_dim) 
{
    size_t base = *next_free_block * sizeof(ColorRGBA);
    if (node->children == NULL) {
        texture[base + 0] = get_red_rgba(node->voxel.color);
        texture[base + 1] = get_green_rgba(node->voxel.color);
        texture[base + 2] = get_blue_rgba(node->voxel.color);
        texture[base + 3] = 255; //Flag for leaf node
        texture[base + 4] = (uint8_t)(node->voxel.voxel.refraction * (255.0 / 3.0)); //(output end / input end) * input
        texture[base + 5] = (uint8_t)(node->voxel.voxel.illumination * 255.0);
        texture[base + 6] = (uint8_t)(node->voxel.voxel.k * 255.0);
        texture[base + 7] = 0; //no use yet
        *next_free_block += LEAF_SIZE;
        return;
    }

    *next_free_block += 1;

    //Encode the address of this block as a "pointer"
    _encode_pointer(*next_free_block, &texture[base], tex_dim);
    
    for (int i = 0; i < 8; ++i)
        if (node->children[i])
            _transform_node_to_texture(node->children[i], texture, next_free_block, tex_dim);
}

uint8_t *octree_texture(Octree *tree, size_t *arr_size) {
    if(!tree || !arr_size) return NULL;
    size_t voxel_count = _octree_texel_size(tree);
    *arr_size = voxel_count * sizeof(ColorRGBA);
    uint8_t *texture = (uint8_t*)calloc(1, *arr_size);
    if(!texture) return NULL;
    size_t next_free_block = 0;
    size_t tex_dim = (size_t)ceil(cbrt((double)voxel_count));
    if(tex_dim == 0) tex_dim = 1;
    _transform_node_to_texture(tree, texture, &next_free_block, tex_dim);
    return texture;
}

void octree_remove(Octree *tree, IVector3 coord) {
    //find node with voxel with coord
    //delete such voxel
    //recursively test if parent node could be reduced
}

void octree_delete(Octree *tree) {
    if(!tree) return;
    for(int i = 0; i < CHILDREN_COUNT; i++) octree_delete(tree);
    free(tree->children);
    free(tree);
}

