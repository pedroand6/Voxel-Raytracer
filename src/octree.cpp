extern "C" {
    #include <vmm/ivec3.h>
    #include <vmm/vec3.h>
    #include <vmm/ray.h>
}
#include <octree.hpp>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h> // Certifique-se de que stdio.h está incluído
#ifndef MIN_HEIGHT
#define MIN_HEIGHT -256
#endif
#ifndef MAX_HEIGHT
#define MAX_HEIGHT 256
#endif

#define CELL_COUNT (MAX_HEIGHT - MIN_HEIGHT)

#define CHILDREN_COUNT 8

#define LEAF_SIZE 3

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
    // Match GPU: use >= for "right/top/front"
    if (coord.x >= mid_points.x) { // RIGHT (changed from <)
        if (coord.y >= mid_points.y) { // TOP
            if (coord.z >= mid_points.z) // FRONT
                return RIGHTTOPFRONT;
            else // BACK
                return RIGHTTOPBACK;
        }
        else { // BOTTOM
            if (coord.z >= mid_points.z) // FRONT
                return RIGHTBOTFRONT;
            else // BACK
                return RIGHTBOTBACK;
        }
    }
    else { // LEFT (coord.x < mid_points.x)
        if (coord.y >= mid_points.y) { // TOP
            if (coord.z >= mid_points.z) // FRONT
                return LEFTTOPFRONT;
            else // BACK
                return LEFTTOPBACK;
        }
        else { // BOTTOM
            if (coord.z >= mid_points.z) // FRONT
                return LEFTBOTFRONT;
            else // BACK
                return LEFTBOTBACK;
        }
    }
}

// CORRIGIDO: Renomeado de _coord_in_space para clareza
// Retorna 'true' se a coordenada está FORA dos limites
bool _coord_is_outside(IVector3 coord, IVector3 left_bot_back, IVector3 right_top_front) {
    return coord.x < left_bot_back.x
        || coord.x >= right_top_front.x
        || coord.y < left_bot_back.y
        || coord.y >= right_top_front.y
        || coord.z < left_bot_back.z
        || coord.z >= right_top_front.z;
}

Octree *octree_new(void) {
    return (Octree*)calloc(1, sizeof(Octree));
}

Octree *octree_create(Octree *parent, IVector3 left_bot_back, IVector3 right_top_front) {
    Octree *ot = octree_new();
    ot->parent = parent;
    ot->left_bot_back = left_bot_back;
    ot->right_top_front = right_top_front;
    // (Você deve inicializar ot->has_voxel = false aqui se estiver usando)
    return ot;
}

Voxel_Object octree_find(Octree *tree, IVector3 coord) {
    // CORRIGIDO: Lógica invertida
    if(_coord_is_outside(coord, tree->left_bot_back, tree->right_top_front)) return _invalid_voxel();
    
    // (Assumindo que has_voxel existe, esta lógica está incompleta se não existir)
    if(tree->has_voxel && ivec3_equal_vec(tree->voxel.coord, coord)) return tree->voxel;
    
    if(!tree->children) return _invalid_voxel();
    
    IVector3 mid_points = ivec3_scalar_div(ivec3_add(tree->left_bot_back, tree->right_top_front), 2);
    int pos = _get_pos_in_octree(coord, mid_points);
    Octree *ref = tree->children[pos];
    
    while(true) {
        if(!ref) return _invalid_voxel(); // Checa se o filho é nulo

        // CORRIGIDO: Lógica invertida
        if(_coord_is_outside(coord, ref->left_bot_back, ref->right_top_front)) return _invalid_voxel();
        
        if(ref->has_voxel && ivec3_equal_vec(ref->voxel.coord, coord)) return ref->voxel;
        
        if(!ref->children) return _invalid_voxel();
        
        mid_points = ivec3_scalar_div(ivec3_add(ref->left_bot_back, ref->right_top_front), 2);
        pos = _get_pos_in_octree(coord, mid_points);
        ref = ref->children[pos];
    }
    return _invalid_voxel();
}

int _create_children(Octree *tree, IVector3 mid_points_ignoradas) {
    tree->children = (Octree**)malloc(sizeof(Octree*) * CHILDREN_COUNT);
    if(!tree->children) return -1;
    
    IVector3 min = tree->left_bot_back;
    IVector3 max = tree->right_top_front;

    IVector3 size = ivec3_sub(max, min);
    if (size.x <= 1 && size.y <= 1 && size.z <= 1) {
        // não dividir mais
        return 0;
    }
    
    // --- INÍCIO DA CORREÇÃO ---
    // Use a matemática de divisão correta que evita o loop infinito.
    IVector3 mid;
    mid.x = min.x + (max.x - min.x) / 2;
    mid.y = min.y + (max.y - min.y) / 2;
    mid.z = min.z + (max.z - min.z) / 2;
    // --- FIM DA CORREÇÃO ---
    
    // (A sua lógica de criação de limites estava correta, 
    // ela só precisava do 'mid' corrigido.)
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

    for(int i = 0; i < CHILDREN_COUNT; i++) {
        tree->children[i]->voxel = _invalid_voxel(); 
    }
    
    int pos = _get_pos_in_octree(tree->voxel.coord, mid); 
    
    tree->children[pos]->voxel = tree->voxel;
    tree->children[pos]->has_voxel = true; 
    tree->voxel = _invalid_voxel();
    tree->has_voxel = false; 
    
    return 0;
}

void octree_insert(Octree *tree, Voxel_Object voxel) {
    if (!tree) return;
    
    Octree *curr = tree;
    while(true) {
        if (_coord_is_outside(voxel.coord, curr->left_bot_back, curr->right_top_front)) return;

        if (curr->children) {
            IVector3 mid;
            mid.x = curr->left_bot_back.x + (curr->right_top_front.x - curr->left_bot_back.x) / 2;
            mid.y = curr->left_bot_back.y + (curr->right_top_front.y - curr->left_bot_back.y) / 2;
            mid.z = curr->left_bot_back.z + (curr->right_top_front.z - curr->left_bot_back.z) / 2;
            
            int pos = _get_pos_in_octree(voxel.coord, mid);
            curr = curr->children[pos];
            continue;
        }
        
        if (!curr->has_voxel) {
            curr->voxel = voxel;
            curr->has_voxel = true;
            return;
        }

        if (ivec3_equal_vec(curr->voxel.coord, voxel.coord)) {
            curr->voxel = voxel;
            return;
        }

        IVector3 mid;
        mid.x = curr->left_bot_back.x + (curr->right_top_front.x - curr->left_bot_back.x) / 2;
        mid.y = curr->left_bot_back.y + (curr->right_top_front.y - curr->left_bot_back.y) / 2;
        mid.z = curr->left_bot_back.z + (curr->right_top_front.z - curr->left_bot_back.z) / 2;

        _create_children(curr, mid); 
    }
}

// NOTA: Esta função e octree_traverse são complexas, 
// provavelmente ineficientes (ou incorretas) e não foram
// totalmente depuradas. Elas não afetam a geração da textura.
Octree *_get_neighbour(Octree *node, IVector3 dir) {
    IVector3 node_size = _get_node_size(node);

    Octree *parent = node->parent;
    IVector3 coord_find = ivec3_add(node->left_bot_back, dir);
    
    // CORRIGIDO: Lógica invertida
    while(parent && _coord_is_outside(coord_find, parent->left_bot_back, parent->right_top_front)) 
        parent = parent->parent;
    
    if(!parent) return NULL; //got out of octree
    
    IVector3 parent_size = _get_node_size(parent);
    while(!ivec3_equal_vec(parent_size, node_size)) {
        if(!parent->children) break;
        // CORRIGIDO: Lógica invertida
        if(_coord_is_outside(coord_find, parent->left_bot_back, parent->right_top_front)) break;
        
        for(int i = 0; i < CHILDREN_COUNT; i++){
            // CORRIGIDO: Lógica invertida (deve ser "se está DENTRO")
            if(!_coord_is_outside(coord_find, parent->children[i]->left_bot_back, parent->children[i]->right_top_front)) {
                parent = parent->children[i];
                break;
            }
        }
    }
    return parent;
}

// --- Math Helpers ---

float fmax_fl(float a, float b) { return a > b ? a : b; }
float fmin_fl(float a, float b) { return a < b ? a : b; }

// --- Core Recursive Traversal ---

/**
 * tx0, ty0, tz0: t-values where ray ENTERS the current node on x,y,z axis
 * tx1, ty1, tz1: t-values where ray EXITS the current node on x,y,z axis
 * a: A bitmask (0-7) fixes negative ray directions (mirroring)
 */
Octree* proc_subtree(float tx0, float ty0, float tz0, 
                     float tx1, float ty1, float tz1, 
                     Octree *node, uint8_t a) {
    
    // 1. Base Case: If leaf, check for content
    if (node->children == NULL) {
        if (node->has_voxel) {
            return node; // HIT!
        }
        return NULL; // MISS (Empty leaf)
    }

    // If the entry t is further than exit t, the ray misses this node entirely
    // (This check is often implicitly handled by the logic below, but good for safety)
    if (fmax_fl(tx0, fmax_fl(ty0, tz0)) >= fmin_fl(tx1, fmin_fl(ty1, tz1))) {
        return NULL;
    }

    // 2. Calculate Midpoint t-values (intersection with the node's internal cross-planes)
    float txM = 0.5f * (tx0 + tx1);
    float tyM = 0.5f * (ty0 + ty1);
    float tzM = 0.5f * (tz0 + tz1);

    // 3. Determine the first child to visit
    // Since we mirrored the ray to be positive, we typically start at child 0 (min,min,min)
    // unless the entry plane (t0) indicates we started "past" a midpoint.
    // However, standard algorithm determines the first node based on max(t0) values.
    
    // Let's simplify: We find the "current node" index (0-7) relative to positive ray
    int currNode = 0;
    if (tx0 > ty0) {
        if (tx0 > tz0) {
            // Plane YZ is hit first? No, this logic finds which sub-cube we enter.
            // If tx0 is large, it means we enter X late. 
            // Standard Revelles logic uses explicit plane comparisons:
             if (tyM < tx0) currNode |= 2;
             if (tzM < tx0) currNode |= 4;
             // We are effectively in the node that starts at tx0.
        }
    } 
    
    // --- SIMPLIFIED LOGIC ---
    // We assume the ray enters at max(tx0, ty0, tz0). 
    // We determine which side of the midplanes (txM, etc) that entry point lies.
    // Note: This logic assumes the ray segment overlaps the node.
    
    // Does the entry point lie on the 'far' side of the X midpoint?
    if(txM < tx0) currNode |= 1; 
    if(tyM < ty0) currNode |= 2;
    if(tzM < tz0) currNode |= 4;

    // 4. Traverse children in order
    while (currNode < 8) {
        
        Octree *child = node->children[currNode ^ a]; // XOR undoes the mirroring
        Octree *hit = NULL;

        // Determine the exit params for the *current child* // based on the axis we are traversing.
        // e.g. if currNode's X bit is 0, it spans tx0..txM. If 1, it spans txM..tx1.
        
        switch (currNode) {
            case 0: // 000
                hit = proc_subtree(tx0, ty0, tz0, txM, tyM, tzM, child, a);
                // Next: move to closest next plane (x, y, or z)
                if (txM < tyM) {
                    if (txM < tzM) { currNode |= 1; }      // Cross X plane
                    else           { currNode |= 4; }      // Cross Z plane
                } else {
                    if (tyM < tzM) { currNode |= 2; }      // Cross Y plane
                    else           { currNode |= 4; }      // Cross Z plane
                }
                break;
                
            case 1: // 001 (X+)
                hit = proc_subtree(txM, ty0, tz0, tx1, tyM, tzM, child, a);
                // We are at High X. We can only go Y+ or Z+ now.
                if (tyM < tzM) currNode |= 2; // Cross Y
                else           currNode |= 4; // Cross Z
                break;

            case 2: // 010 (Y+)
                hit = proc_subtree(tx0, tyM, tz0, txM, ty1, tzM, child, a);
                if (txM < tzM) currNode |= 1; // Cross X
                else           currNode |= 4; // Cross Z
                break;

            case 3: // 011 (X+, Y+)
                hit = proc_subtree(txM, tyM, tz0, tx1, ty1, tzM, child, a);
                currNode |= 4; // Only way is Z+
                break;

            case 4: // 100 (Z+)
                hit = proc_subtree(tx0, ty0, tzM, txM, tyM, tz1, child, a);
                if (txM < tyM) currNode |= 1; 
                else           currNode |= 2;
                break;

            case 5: // 101 (Z+, X+)
                hit = proc_subtree(txM, ty0, tzM, tx1, tyM, tz1, child, a);
                currNode |= 2; // Only way is Y+
                break;

            case 6: // 110 (Z+, Y+)
                hit = proc_subtree(tx0, tyM, tzM, txM, ty1, tz1, child, a);
                currNode |= 1; // Only way is X+
                break;

            case 7: // 111
                hit = proc_subtree(txM, tyM, tzM, tx1, ty1, tz1, child, a);
                currNode = 8; // Done, exit node
                break;
        }

        if (hit) return hit; // Found something, bubble it up
    }

    return NULL;
}

// --- Entry Point ---

Octree* octree_ray_cast(Octree *root, Ray ray, Vector3 box_min, Vector3 box_max) {
    // 1. Handle Negative Directions (Mirroring)
    uint8_t a = 0;

    // --- FIX X ---
    if (ray.direction.x < 0) {
        ray.origin.x = box_min.x + box_max.x - ray.origin.x; // Mirror Origin
        ray.direction.x = -ray.direction.x;                  // Mirror Direction
        a |= 1; 
    }
    
    // --- FIX Y (CRITICAL UPDATE) ---
    if (ray.direction.y < 0) {
        ray.origin.y = box_min.y + box_max.y - ray.origin.y; // Mirror Origin
        ray.direction.y = -ray.direction.y;                  // Mirror Direction
        a |= 2;
    }

    // --- FIX Z (CRITICAL UPDATE) ---
    if (ray.direction.z < 0) {
        ray.origin.z = box_min.z + box_max.z - ray.origin.z; // Mirror Origin
        ray.direction.z = -ray.direction.z;                  // Mirror Direction
        a |= 4;
    }

    // 2. Calculate Initial T values
    // (Now that direction is always positive, standard T calculation works safely)
    float tx0 = (box_min.x - ray.origin.x) / ray.direction.x;
    float tx1 = (box_max.x - ray.origin.x) / ray.direction.x;
    
    float ty0 = (box_min.y - ray.origin.y) / ray.direction.y;
    float ty1 = (box_max.y - ray.origin.y) / ray.direction.y;
    
    float tz0 = (box_min.z - ray.origin.z) / ray.direction.z;
    float tz1 = (box_max.z - ray.origin.z) / ray.direction.z;

    // 3. Clamp to ensure we start at the box surface if ray starts outside
    float t_entry = fmax_fl(fmax_fl(tx0, ty0), tz0);
    float t_exit  = fmin_fl(fmin_fl(tx1, ty1), tz1);

    // 4. Check if ray misses box completely
    if (t_entry > t_exit || t_exit < 0) {
        return NULL;
    }

    // 5. Launch Recursion
    return proc_subtree(tx0, ty0, tz0, tx1, ty1, tz1, root, a);
}

size_t _octree_texel_size(Octree *tree) {
    if(!tree) return 0;
    if(!tree->children) return LEAF_SIZE; // <-- Agora retorna 3
    
    size_t total = 1 + 8; // 1 (pai) + 8 (bloco de ponteiros)
    
    for(int i = 0; i < CHILDREN_COUNT; i++) {
        total += _octree_texel_size(tree->children[i]);
    }
    return total;
}

void _encode_pointer(int block_index, uint8_t *out_voxel, size_t tex_dim) {
    // (Assumindo que ColorRGBA é 4 bytes)
    // Precisamos de (x,y,z) *por texel*, não por byte.
    // O ponteiro de entrada out_voxel já deve estar na base correta (ex: &texture[base])
    
    size_t tex_dim_sq = tex_dim * tex_dim;
    int z = block_index / (tex_dim_sq);
    int y = (block_index / tex_dim) % tex_dim;
    int x = block_index % tex_dim;

    out_voxel[0] = (uint8_t)x;
    out_voxel[1] = (uint8_t)y;
    out_voxel[2] = (uint8_t)z;
    out_voxel[3] = 0; //Flag for internal node
}

// Esta função usa a lógica SVO correta (nó pai -> bloco de 8 ponteiros -> filhos)
void _transform_node_to_texture(Octree *node, 
                                uint8_t *texture, 
                                size_t *next_free_block, 
                                size_t tex_dim) 
{
    size_t base = *next_free_block * 4; // (4 bytes por texel)
    
    if (node->children == NULL) {
        // ---- Início da Lógica da Folha (LEAF_SIZE = 3) ----
        
        // TEXEL 0: Cor (RGB) e Flag de Folha (A)
        texture[base + 0] = get_red_rgba(node->voxel.color);
        texture[base + 1] = get_green_rgba(node->voxel.color);
        texture[base + 2] = get_blue_rgba(node->voxel.color);
        texture[base + 3] = 255; //Flag for leaf node
        
        // TEXEL 1: Coordenada do Voxel (XYZ) e Alpha da Cor (A)
        // (Isso assume que as coordenadas do mundo estão entre 0-255)
        texture[base + 4] = (uint8_t)(node->voxel.coord.x);
        texture[base + 5] = (uint8_t)(node->voxel.coord.y);
        texture[base + 6] = (uint8_t)(node->voxel.coord.z);
        texture[base + 7] = get_alpha_rgba(node->voxel.color); // color alpha
        
        // TEXEL 2: Propriedades (RGB) e (A não usado)
        texture[base + 8] = (uint8_t)(node->voxel.voxel.refraction * (255.0 / 3.0));
        texture[base + 9] = (uint8_t)(node->voxel.voxel.illumination * 255.0);
        texture[base + 10] = (uint8_t)(node->voxel.voxel.k * 255.0);
        texture[base + 11] = 255 * node->has_voxel; // Mostra se é um voxel ou leaf vazio

        *next_free_block += LEAF_SIZE;
        // ---- Fim da Lógica da Folha ----
        return;
    }

    // --- Lógica do Nó Interno (igual a antes) ---
    size_t pai_base_addr = *next_free_block;
    *next_free_block += 1; 
    size_t ponteiro_bloco_addr = *next_free_block;
    *next_free_block += 8;
    
    _encode_pointer(ponteiro_bloco_addr, &texture[pai_base_addr * 4], tex_dim);
    
    for (int i = 0; i < 8; ++i)
    {
        if (node->children[i])
        {
            size_t filho_addr = *next_free_block;
            size_t ptr_slot_base = (ponteiro_bloco_addr + i) * 4;
            _encode_pointer(filho_addr, &texture[ptr_slot_base], tex_dim);
            _transform_node_to_texture(node->children[i], texture, next_free_block, tex_dim);
        }
        else
        {
            size_t ptr_slot_base = (ponteiro_bloco_addr + i) * 4;
            _encode_pointer(0, &texture[ptr_slot_base], tex_dim);
        }
    }
}

uint8_t *octree_texture(Octree *tree, size_t *arr_size, size_t tex_dim) {
    if(!tree || !arr_size) return NULL;
    
    // _octree_texel_size agora está correto
    size_t voxel_count = _octree_texel_size(tree); 
    if (voxel_count == 0) {
        *arr_size = 0;
        return NULL;
    }
    
    // (Assumindo sizeof(ColorRGBA) == 4 bytes)
    *arr_size = voxel_count * 4; 
    
    uint8_t *texture = (uint8_t*)calloc(1, *arr_size);
    if(!texture) return NULL;
    
    size_t next_free_block = 0;
    
    _transform_node_to_texture(tree, texture, &next_free_block, tex_dim);

    return texture;
}

void octree_remove(Octree *tree, IVector3 coord) {
    //find node with voxel with coord
    //delete such voxel
    //recursively test if parent node could be reduced
}

// CORRIGIDO: Esta é a correção CRÍTICA para evitar o stack overflow.
void octree_delete(Octree *tree) {
    if (!tree) return; // Guarda de nó nulo

    if (tree->children) { // Verifica se há filhos
        for(int i = 0; i < CHILDREN_COUNT; i++) {
            octree_delete(tree->children[i]); // Chama recursivamente nos *filhos*
        }
        free(tree->children); // Libera o array de ponteiros dos filhos
    }
    
    free(tree); // Libera o nó atual
}