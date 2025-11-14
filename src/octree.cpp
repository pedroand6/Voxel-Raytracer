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

// CORRIGIDO: Renomeado de _coord_in_space para clareza
// Retorna 'true' se a coordenada está FORA dos limites
bool _coord_is_outside(IVector3 coord, IVector3 left_bot_back, IVector3 right_top_front) {
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

    // Inicializa todos os novos filhos como vazios
    for(int i = 0; i < CHILDREN_COUNT; i++) {
        tree->children[i]->voxel = _invalid_voxel(); // ou {0}
        tree->children[i]->has_voxel = false; // <-- CORREÇÃO
    }
    
    // Move o voxel antigo do pai para o filho correto
    int pos = _get_pos_in_octree(tree->voxel.coord, mid_points);
    tree->children[pos]->voxel = tree->voxel;
    tree->children[pos]->has_voxel = true; // <-- CORREÇÃO CRÍTICA
    
    // Limpa o voxel do pai (agora é um nó interno)
    tree->voxel = _invalid_voxel();
    tree->has_voxel = false; // <-- CORREÇÃO CRÍTICA
    
    return 0;
}

void octree_insert(Octree *tree, Voxel_Object voxel) {
    if (!tree) return;
    
    Octree *curr = tree;
    while(true) {
        if (_coord_is_outside(voxel.coord, curr->left_bot_back, curr->right_top_front)) return;

        // --- INÍCIO DA CORREÇÃO DE LÓGICA ---
        
        // 1. Se eu sou um NÓ INTERNO (tenho filhos), eu NUNCA armazeno um voxel.
        //    Apenas desço para o filho correto.
        if (curr->children) {
            IVector3 mid_points = ivec3_scalar_div(ivec3_add(curr->left_bot_back, curr->right_top_front), 2);
            int pos = _get_pos_in_octree(voxel.coord, mid_points);
            curr = curr->children[pos];
            continue; // Reinicia o loop no nível do filho
        }
        
        // 2. Se eu cheguei aqui, eu sou um NÓ FOLHA (NÃO tenho filhos).

        // 2a. Se eu sou um nó folha VAZIO, armazeno o voxel e termino.
        if (!curr->has_voxel) {
            curr->voxel = voxel;
            curr->has_voxel = true;
            return;
        }

        // 2b. Se eu sou um nó folha CHEIO, e o voxel é o mesmo, atualizo e termino.
        if (ivec3_equal_vec(curr->voxel.coord, voxel.coord)) {
            curr->voxel = voxel; // Atualiza
            return;
        }
        
        IVector3 mid_points = ivec3_scalar_div(ivec3_add(curr->left_bot_back, curr->right_top_front), 2);
        _create_children(curr, mid_points); 
        
        // _create_children moveu o voxel ANTIGO para um filho e
        // definiu curr->has_voxel = false.
        
        // O loop agora vai rodar novamente. Na próxima iteração:
        // 1. `if (curr->children)` será VERDADEIRO.
        // 2. O código (corretamente) descerá para o filho `pos`.
        // 3. O NOVO voxel será inserido no filho correto.
        
        // --- FIM DA CORREÇÃO ---
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

// NOTA: Esta função é muito ineficiente pois chama octree_find()
// dentro do loop DDA. A lógica correta de DDA/travessia de octree
// não requer 'find'.
Octree *octree_traverse(Octree *tree, Ray ray) {
    float tmin;
    float tmax;

    Vector3 cell_count = vec3_float(CELL_COUNT, CELL_COUNT, CELL_COUNT);

    if (!ray_hits_box(ray, vec3_ivec3(tree->left_bot_back), vec3_ivec3(tree->right_top_front), &tmin, &tmax))
        return NULL;

    ray.origin = vec3_scalar_mul(ray.direction, fmin(tmin, tmax));
    Vector3 cell = vec3_min(vec3_ivec3(ivec3_vec3(vec3_sub(ray.origin, vec3_ivec3(tree->left_bot_back)))), vec3_sub(cell_count, vec3_one()));
    Vector3 cell_lbb = vec3_float(cell.x - 0.5, cell.y - 0.5, cell.z - 0.5);
    Vector3 cell_rtf = vec3_float(cell.x + 0.5, cell.y + 0.5, cell.z + 0.5);
    Vector3 tMaxNeg = vec3_div(vec3_sub(cell_lbb, ray.origin), ray.direction);
    Vector3 tMaxPos = vec3_div(vec3_sub(cell_rtf, ray.origin), ray.direction);

    Vector3 t_max = vec3_float(
        ray.direction.x < 0 ? tMaxNeg.x : tMaxPos.x,
        ray.direction.y < 0 ? tMaxNeg.y : tMaxPos.y,
        ray.direction.z < 0 ? tMaxNeg.z : tMaxPos.z
    );

    Vector3 step = vec3_sign(ray.direction);
    Vector3 t_delta = vec3_abs(vec3_div(vec3_one(), ray.direction));

    Vector3 neighbour_dirs[] = {
        (step.x < 0) ? vec3_left() : vec3_right(),
        (step.y < 0) ? vec3_down() : vec3_up(),
        (step.z < 0) ? vec3_back() : vec3_forward(),
    };

    Octree *node = tree;

    while (node) {
        if(node->children)
            for(int i = 0; i < CHILDREN_COUNT; i++) {
                Octree *child = node->children[i];
                if(child && child->has_voxel && !voxel_obj_compare(octree_find(child, ivec3_vec3(cell)), _invalid_voxel())) {
                    node = child;
                    i = -1;

                    if(!node->children)
                        return node;
                }
            }

        int dir = 0;

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

        if(voxel_obj_compare(octree_find(node, ivec3_vec3(cell)), _invalid_voxel())) {
            Vector3 neighbor_dir = neighbour_dirs[dir];
            node = _get_neighbour(node, ivec3_vec3(neighbor_dir));
        }
        else if(!node->children)
            return node;
    }
    return NULL;
}

// CORRIGIDO: Esta é a correção CRÍTICA para evitar o buffer overflow.
size_t _octree_texel_size(Octree *tree) {
    if(!tree) return 0;
    if(!tree->children) return LEAF_SIZE; // Nó folha = 2 texels
    
    // Nó interno:
    size_t total = 1 + 8; // <-- CORRETO: 1 (pai) + 8 (bloco de ponteiros)
    
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
    // (Assumindo sizeof(ColorRGBA) == 4 bytes)
    size_t base = *next_free_block * 4; 
    
    if (node->children == NULL) {
        // (Assumindo que has_voxel está sendo usado corretamente)
        texture[base + 0] = get_red_rgba(node->voxel.color);
        texture[base + 1] = get_green_rgba(node->voxel.color);
        texture[base + 2] = get_blue_rgba(node->voxel.color);
        texture[base + 3] = 255; //Flag for leaf node
        
        texture[base + 4] = (uint8_t)(node->voxel.voxel.refraction * (255.0 / 3.0)); 
        texture[base + 5] = (uint8_t)(node->voxel.voxel.illumination * 255.0);
        texture[base + 6] = (uint8_t)(node->voxel.voxel.k * 255.0);
        texture[base + 7] = get_alpha_rgba(node->voxel.color); //color alpha
        *next_free_block += LEAF_SIZE;
        return;
    }

    // --- Lógica do Nó Interno ---
    
    // 1. Guarda o endereço deste nó pai
    size_t pai_base_addr = *next_free_block;
    *next_free_block += 1; // Aloca espaço para o nó pai
    
    // 2. Guarda o endereço do futuro "bloco de ponteiros"
    size_t ponteiro_bloco_addr = *next_free_block;
    *next_free_block += 8; // Aloca 8 blocos para os ponteiros!
    
    // 3. Escreve o ponteiro no nó pai (pai_base_addr) para apontar
    //    para o bloco de ponteiros (ponteiro_bloco_addr)
    _encode_pointer(ponteiro_bloco_addr, &texture[pai_base_addr * 4], tex_dim);
    
    // 4. Loop para processar os filhos
    for (int i = 0; i < 8; ++i)
    {
        if (node->children[i])
        {
            // 5. Guarda onde este filho VAI começar (o bloco livre atual)
            size_t filho_addr = *next_free_block;
            
            // 6. Escreve o ponteiro para este filho DENTRO do bloco de ponteiros
            // (O endereço é: ponteiro_bloco_addr + i)
            size_t ptr_slot_base = (ponteiro_bloco_addr + i) * 4;
            _encode_pointer(filho_addr, &texture[ptr_slot_base], tex_dim);
            
            // 7. Chama recursivamente para preencher a sub-árvore do filho
            _transform_node_to_texture(node->children[i], texture, next_free_block, tex_dim);
        }
        else
        {
            // Se o filho for nulo (árvore esparsa), 
            // escreve um ponteiro nulo (ex: 0,0,0)
            size_t ptr_slot_base = (ponteiro_bloco_addr + i) * 4;
            _encode_pointer(0, &texture[ptr_slot_base], tex_dim);
        }
    }
}

uint8_t *octree_texture(Octree *tree, size_t *arr_size) {
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