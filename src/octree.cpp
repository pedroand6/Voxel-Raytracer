extern "C" {
    #include <vmm/ivec3.h>
    #include <vmm/vec3.h>
    #include <vmm/ray.h>
}
#include <iostream>
#include <octree.hpp>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h> // Certifique-se de que stdio.h está incluído
#ifndef MIN_HEIGHT
#define MIN_HEIGHT -1024
#endif
#ifndef MAX_HEIGHT
#define MAX_HEIGHT 1024
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

// Verifica se um nó é uma folha (tem voxel e não tem filhos)
bool _is_leaf(Octree *node) {
    return node && node->has_voxel && !node->children;
}

// Verifica se dois nós são visivelmente idênticos
bool _nodes_are_identical(Octree *a, Octree *b) {
    if (!_is_leaf(a) || !_is_leaf(b)) return false;
    
    // Compara cor e propriedades (ignorando coordenada, pois ela muda)
    // Nota: Aqui assumimos que se a cor é igual, o bloco é igual.
    // Se você tiver IDs de bloco, compare os IDs.
    return ((a->voxel.color == b->voxel.color) &&
           (a->voxel.voxel.refraction == b->voxel.voxel.refraction) &&
           (a->voxel.voxel.illumination == b->voxel.voxel.illumination)) 
           || (a->voxel.coord.y <= MIN_HEIGHT && b->voxel.coord.y <= MIN_HEIGHT);
}

// Divide um nó sólido em 8 filhos sólidos idênticos
int _split_node(Octree *tree) {
    if (tree->children) return 0; 

    // Backup dos dados
    Voxel_Object originalData = tree->voxel;
    bool wasSolid = tree->has_voxel;

    IVector3 min = tree->left_bot_back;
    IVector3 max = tree->right_top_front;
    
    IVector3 mid;
    mid.x = min.x + (max.x - min.x) / 2;
    mid.y = min.y + (max.y - min.y) / 2;
    mid.z = min.z + (max.z - min.z) / 2;

    // Cria os filhos (inicialmente vazios)
    if (_create_children(tree, mid) != 0) return -1;

    if (wasSolid) {
        // --- HEURÍSTICA DE DIFERENCIAÇÃO ---
        
        // Verifica se a coordenada do voxel coincide com a base do nó.
        // Em um nó Merged (sólido total), normalizamos a coord para a base.
        // Em um nó Lazy (ponto flutuante), a coord é a posição original do bloco.
        bool isVolume = ivec3_equal_vec(originalData.coord, tree->left_bot_back);

        if (isVolume) {
            // CASO A: O nó era um VOLUME SÓLIDO (ex: parede mergeada).
            // Preenchemos todos os 8 filhos com o material.
            for (int i = 0; i < 8; i++) {
                tree->children[i]->voxel = originalData;
                tree->children[i]->has_voxel = true;
                
                // Cada filho assume sua própria posição no espaço
                tree->children[i]->voxel.coord = tree->children[i]->left_bot_back;
            }
        } else {
            // CASO B: O nó era um PONTO ISOLADO (Lazy Insert).
            // Apenas movemos o voxel para o sub-nó correto, os outros 7 ficam vazios (Ar).
            
            int pos = _get_pos_in_octree(originalData.coord, mid);
            
            tree->children[pos]->voxel = originalData;
            tree->children[pos]->has_voxel = true;
            // Mantemos a coordenada original exata!
        }
        
        // O pai deixa de ser folha
        tree->has_voxel = false;
    }
    
    return 0;
}

// Tenta simplificar a árvore fundindo 8 filhos idênticos
void _try_merge_children(Octree *node) {
    if (!node || !node->children) return;

    // 1. Verifica se todos os 8 filhos são folhas
    for (int i = 0; i < 8; i++) {
        if (!_is_leaf(node->children[i])) return; // Não podemos fundir se um filho tiver netos
    }

    // 2. Verifica se todos são idênticos ao primeiro filho
    Octree *first = node->children[0];
    for (int i = 1; i < 8; i++) {
        if (!_nodes_are_identical(first, node->children[i])) return;
    }

    // 3. MERGE! Todos são iguais.
    // Copia os dados do primeiro filho para o pai
    node->voxel = first->voxel;
    // Ajusta a coordenada do pai para ser a base do nó (opcional, mas bom para consistência)
    node->voxel.coord = node->left_bot_back; 
    node->has_voxel = true;

    // 4. Apaga os filhos
    for (int i = 0; i < 8; i++) {
        free(node->children[i]);
    }
    free(node->children);
    node->children = NULL;
}

void octree_insert(Octree *tree, Voxel_Object voxel) {
    if (!tree) return;
    if (_coord_is_outside(voxel.coord, tree->left_bot_back, tree->right_top_front)) return;

    IVector3 size = ivec3_sub(tree->right_top_front, tree->left_bot_back);

    // --- CASO BASE: Tamanho 1x1x1 ---
    // Aqui nós substituímos o dado, seja ele qual for.
    if (size.x <= 1 && size.y <= 1 && size.z <= 1) {
        tree->voxel = voxel;
        tree->has_voxel = true;
        return;
    }

    // --- SPLIT DOWN (A CORREÇÃO) ---
    // Se este nó não tem filhos, mas precisamos descer mais (porque size > 1),
    // precisamos criar os filhos.
    if (!tree->children) {
        // Se já existe algo aqui (Blocão Sólido), o _split_node vai
        // copiar esse material para os filhos antes de continuarmos.
        if (_split_node(tree) != 0) return; 
    }

    // --- DESCIDA RECURSIVA ---
    IVector3 mid;
    mid.x = tree->left_bot_back.x + size.x / 2;
    mid.y = tree->left_bot_back.y + size.y / 2;
    mid.z = tree->left_bot_back.z + size.z / 2;
    
    int pos = _get_pos_in_octree(voxel.coord, mid);
    octree_insert(tree->children[pos], voxel);

    // --- MERGE UP (OTIMIZAÇÃO) ---
    // Na volta, tentamos juntar de novo, caso tenhamos preenchido um buraco
    // com o mesmo material que já existia ao redor.
    _try_merge_children(tree);
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
// Busca um nó folha contendo a coordenada global 'pos'
// Atualiza nodeMin e nodeMax com os limites desse nó
Octree* _octree_find_leaf(Octree *root, IVector3 pos, IVector3 *nodeMin, IVector3 *nodeMax) {
    Octree *curr = root;
    IVector3 min = root->left_bot_back;
    IVector3 max = root->right_top_front;

    // Se estiver fora do mundo, retorna NULL
    if (_coord_is_outside(pos, min, max)) return NULL;

    while (curr->children != NULL) {
        // Calcula ponto médio
        IVector3 mid;
        mid.x = min.x + (max.x - min.x) / 2;
        mid.y = min.y + (max.y - min.y) / 2;
        mid.z = min.z + (max.z - min.z) / 2;

        // Descobre índice do filho
        int childIdx = _get_pos_in_octree(pos, mid);
        
        // Atualiza os limites para o próximo nível (desce a caixa)
        if (childIdx & 4) min.x = mid.x; else max.x = mid.x;
        if (childIdx & 2) min.y = mid.y; else max.y = mid.y;
        if (childIdx & 1) min.z = mid.z; else max.z = mid.z;

        curr = curr->children[childIdx];
        
        // Se o filho for nulo (buraco na árvore), paramos aqui.
        // Retornamos este nó "vazio" (ou NULL se preferir tratar como ar)
        // Mas precisamos retornar os bounds corretos desse vazio!
        if (curr == NULL) {
            *nodeMin = min;
            *nodeMax = max;
            return NULL; 
        }
    }

    // Chegamos numa folha
    *nodeMin = min;
    *nodeMax = max;
    return curr;
}

Octree* octree_ray_cast(Octree *root, Ray ray, Vector3 worldMin, Vector3 worldMax) {
    // 1. Setup inicial
    Vector3 rayPos = ray.origin;
    Vector3 rayDir = ray.direction;

    // Evita divisão por zero (igual ao GLSL)
    Vector3 invDir;
    invDir.x = (fabsf(rayDir.x) < 1e-8f) ? 1e20f : 1.0f / rayDir.x;
    invDir.y = (fabsf(rayDir.y) < 1e-8f) ? 1e20f : 1.0f / rayDir.y;
    invDir.z = (fabsf(rayDir.z) < 1e-8f) ? 1e20f : 1.0f / rayDir.z;

    // Coordenada inteira atual do mapa
    IVector3 mapPos;
    mapPos.x = (int)floorf(rayPos.x);
    mapPos.y = (int)floorf(rayPos.y);
    mapPos.z = (int)floorf(rayPos.z);

    // Estado atual do Voxel
    IVector3 nodeMin = ivec3_vec3(worldMin), nodeMax = ivec3_vec3(worldMax);
    Octree *currNode = NULL;

    // Limite de passos (segurança)
    int maxSteps = 512; 

    for (int i = 0; i < maxSteps; i++) {
        // Busca o nó atual na árvore e seus limites
        currNode = _octree_find_leaf(root, mapPos, &nodeMin, &nodeMax);

        // Se encontrou um nó válido COM voxel e Y válido, é um HIT!
        if (currNode && currNode->has_voxel && currNode->voxel.coord.y > MIN_HEIGHT) {
            return currNode;
        }

        // Se não achou nada (ar), precisamos avançar o raio para sair deste nó vazio.
        // Usamos os bounds retornados por _octree_find_leaf (que ajustou min/max para o tamanho do vazio)
        
        // Cálculo de intersecção AABB (para sair do nó)
        float tMaxX = (rayDir.x > 0.0f ? (float)nodeMax.x - rayPos.x : (float)nodeMin.x - rayPos.x) * invDir.x;
        float tMaxY = (rayDir.y > 0.0f ? (float)nodeMax.y - rayPos.y : (float)nodeMin.y - rayPos.y) * invDir.y;
        float tMaxZ = (rayDir.z > 0.0f ? (float)nodeMax.z - rayPos.z : (float)nodeMin.z - rayPos.z) * invDir.z;

        // Descobre o menor passo positivo (qual parede atingimos)
        // Adicionamos epsilon para garantir cruzamento
        float tStep = fmin_fl(tMaxX, fmin_fl(tMaxY, tMaxZ));
        int axis = (tMaxX < tMaxY) ? ((tMaxX < tMaxZ) ? 0 : 2) : ((tMaxY < tMaxZ) ? 1 : 2);
        
        // Proteção contra passos muito pequenos (travamento numérico)
        if (tStep < 0.0001f) tStep = 0.0001f;

        // Avança o raio
        rayPos.x += rayDir.x * tStep;
        rayPos.y += rayDir.y * tStep;
        rayPos.z += rayDir.z * tStep;

        // Empurra levemente para dentro do vizinho (igual ao shader)
        Vector3 testPos = rayPos;
        switch (axis)
        {
        case 0:
            testPos.x += rayDir.x * 0.001f;
            break;
        case 1:
            testPos.y += rayDir.y * 0.001f;
            break;
        case 2:
            testPos.z += rayDir.z * 0.001f;
            break;
        }

        mapPos.x = (int)floorf(testPos.x);
        mapPos.y = (int)floorf(testPos.y);
        mapPos.z = (int)floorf(testPos.z);

        // Verifica se saiu do mundo
        if (_coord_is_outside(mapPos, root->left_bot_back, root->right_top_front)) {
            return NULL;
        }
    }

    return NULL;
}

// Retorna a máscara de filhos válidos (8 bits)
uint8_t _get_child_mask(Octree *node) {
    if (!node || !node->children) return 0;
    uint8_t mask = 0;
    for (int i = 0; i < 8; i++) {
        // Um filho existe se não for NULL e (tiver voxel OU tiver netos)
        if (node->children[i] && (node->children[i]->has_voxel || node->children[i]->children)) {
            mask |= (1 << i);
        }
    }
    return mask;
}

// Ajuda a decidir se o bloco de filhos deve ser tratado como folhas (stride largo)
bool _block_contains_leaves(Octree *node) {
    if (!node || !node->children) return false;
    for (int i = 0; i < 8; i++) {
        // Se existe algum filho que NÃO tem filhos (é folha e tem voxel), o bloco todo vira bloco de folhas
        if (node->children[i] && node->children[i]->children == NULL && node->children[i]->has_voxel) {
            return true;
        }
    }
    return false;
}

int _count_set_bits(uint8_t n) {
    int count = 0;
    while (n > 0) {
        n &= (n - 1);
        count++;
    }
    return count;
}

// --- FIXED: Octree Texel Size ---
// Calculates exact size: 
// 1 texel (Header) + N texels (Pointers) + Recursive Children
size_t _octree_texel_size(Octree *tree) {
    if(!tree) return 0;
    
    // CASO BASE: Folha com dados (2 texels)
    if(!tree->children) {
        return tree->has_voxel ? LEAF_SIZE : 0;
    }
    
    // CASO NÓ INTERNO:
    uint8_t mask = _get_child_mask(tree);
    if (mask == 0) return 0; // Se não tem filhos válidos, tamanho é 0
    
    // 1. Header do próprio nó
    size_t total = 1; 
    
    // 2. Ponteiros para os filhos (Packed/Espremidos)
    // Se a máscara for 00000101, temos 2 filhos, logo 2 texels de ponteiro.
    total += _count_set_bits(mask);

    // 3. Tamanho recursivo dos filhos
    for(int i = 0; i < CHILDREN_COUNT; i++) {
        // Verifica se o bit 'i' está setado
        if ((mask >> i) & 1) {
            total += _octree_texel_size(tree->children[i]);
        }
    }
    
    return total;
}

// Codifica um índice linear de até 16 Milhões (24 bits) nos canais R, G, B
// Usa o bit mais significativo (Bit 23 do Blue) como flag "IS_LEAF_BLOCK"
void _encode_pointer(size_t linear_index, bool is_leaf_block, uint8_t *out_voxel) {
    // Garante que o índice cabe em 23 bits (Max ~8.3 milhões de texels)
    // Se precisar de mais, use texturas maiores ou lógica RGBA32UI
    uint32_t val = (uint32_t)linear_index;
    
    // Se for bloco de folhas, liga o bit 23 (Top bit do 3º byte)
    if (is_leaf_block) {
        val |= 0x800000; // Seta o bit 23
    }

    out_voxel[0] = (uint8_t)(val & 0xFF);        // R (Low)
    out_voxel[1] = (uint8_t)((val >> 8) & 0xFF); // G (Mid)
    out_voxel[2] = (uint8_t)((val >> 16) & 0xFF);// B (High + Flag)
    // O Alpha (out_voxel[3]) é controlado pela função principal (Máscara ou Flag)
}

// Esta função usa a lógica SVO correta (nó pai -> bloco de 8 ponteiros -> filhos)
void _transform_node_to_texture(Octree *node, 
                                uint8_t *texture, 
                                size_t *next_free_block, 
                                size_t tex_dim) 
{
    if (!node) return;

    // --- A. SOU FOLHA? (Escreve Dados) ---
    if (node->children == NULL) {
        if (!node->has_voxel) return;

        size_t base_byte = (*next_free_block) * 4;
        
        // Texel 1: Cor + Marker
        texture[base_byte + 0] = get_red_rgba(node->voxel.color);
        texture[base_byte + 1] = get_green_rgba(node->voxel.color);
        texture[base_byte + 2] = get_blue_rgba(node->voxel.color);
        texture[base_byte + 3] = 255; // Marcador: Sou dados de folha
        
        // Texel 2: Propriedades Físicas
        texture[base_byte + 4] = (uint8_t)(node->voxel.voxel.refraction * 85.0f); // Scale correction
        texture[base_byte + 5] = (uint8_t)(node->voxel.voxel.illumination * 255.0f);
        texture[base_byte + 6] = (uint8_t)(node->voxel.voxel.k * 255.0f);
        texture[base_byte + 7] = get_alpha_rgba(node->voxel.color);
        
        (*next_free_block) += LEAF_SIZE; // Incrementa 2
        return;
    }

    // --- B. SOU NÓ INTERNO (Escreve Ponteiros) ---
    uint8_t mask = _get_child_mask(node);
    if (mask == 0) return;

    // 1. Escreve MEU Header
    size_t header_pos = *next_free_block;
    size_t header_byte = header_pos * 4;
    
    // Incrementa valor apontado (CORREÇÃO DE PRECEDÊNCIA)
    (*next_free_block)++; 

    // 2. Reserva espaço CONTÍGUO para os ponteiros dos filhos
    int active_children_count = _count_set_bits(mask);
    size_t pointers_start_idx = *next_free_block;
    
    // Já avança o "cursor" global para depois dos meus ponteiros
    // para que os filhos sejam escritos depois desta lista.
    (*next_free_block) += active_children_count;

    // Preenche o Header
    // Codifica ponteiro para o INÍCIO da lista de filhos
    // Nota: is_leaf_block no header geralmente não é útil se misturado, 
    // deixamos false aqui e setamos nos ponteiros individuais.
    _encode_pointer(pointers_start_idx, false, &texture[header_byte]);
    texture[header_byte + 3] = mask; // Alpha = Máscara de filhos

    // 3. Processa e Escreve Filhos Recursivamente
    int current_ptr_offset = 0; // Indice LOCAL na lista de ponteiros (0 a 7 mas compactado)

    for (int i = 0; i < 8; ++i) {
        if ((mask >> i) & 1) {
            // Onde este filho VAI ser escrito na textura (futuro)
            size_t child_future_addr = *next_free_block;
            
            // Onde eu devo escrever o ponteiro AGORA
            size_t ptr_slot_byte = (pointers_start_idx + current_ptr_offset) * 4;

            // Verifica se ESSE filho específico é folha
            bool child_is_leaf = (node->children[i]->children == NULL && 
                      node->children[i]->has_voxel);

            // Escreve o ponteiro na lista reservada
            _encode_pointer(child_future_addr, child_is_leaf, &texture[ptr_slot_byte]);
            
            // Texture[ptr_slot_byte + 3] (Alpha do ponteiro) pode ser usado 
            // para info extra se necessário, ou deixado 0.

            // Recurso: Vai lá no final e escreve os dados do filho
            _transform_node_to_texture(node->children[i], texture, next_free_block, tex_dim);
            
            current_ptr_offset++;
        }
    }
}

uint8_t *octree_texture(Octree *tree, size_t *arr_size, size_t tex_dim) {
    if(!tree || !arr_size) return NULL;
    
    size_t voxel_count = _octree_texel_size(tree); 
    if (voxel_count == 0) {
        *arr_size = 0;
        return NULL;
    }
    
    *arr_size = voxel_count * 4; 
    
    uint8_t *texture = (uint8_t*)calloc(voxel_count * 4, sizeof(uint8_t));
    if(!texture) return NULL;
    
    size_t next_free_block = 0;
    
    _transform_node_to_texture(tree, texture, &next_free_block, tex_dim);
    
    // DEBUG: Verifica se usamos exatamente o espaço calculado
    if (next_free_block != voxel_count) {
        fprintf(stderr, "WARNING: Size mismatch! Calculated: %zu, Used: %zu\n", 
                voxel_count, next_free_block);
    }

    return texture;
}

void octree_remove(Octree *tree, IVector3 coord) {
    if (!tree) return;
    
    // Se está fora, ignora
    if (_coord_is_outside(coord, tree->left_bot_back, tree->right_top_front)) return;

    IVector3 size = ivec3_sub(tree->right_top_front, tree->left_bot_back);

    // --- CASO BASE: Tamanho 1x1x1 (Atomic Voxel) ---
    // SÓ AQUI podemos deletar de fato.
    if (size.x <= 1 && size.y <= 1 && size.z <= 1) {
        tree->has_voxel = false; 
        return;
    }

    // --- LÓGICA DE UN-MERGE (Split Down) ---
    // Se este nó não tem filhos (é uma folha na árvore), mas tem tamanho > 1 (é um blocão),
    // e tem dados (é sólido), precisamos quebrá-lo antes de remover um pedaço.
    if (!tree->children && tree->has_voxel) {
        if (_split_node(tree) != 0) return; // Falha de memória
    }

    // Se depois de tentar dividir ele ainda não tem filhos, é porque era Ar (vazio).
    // Não tem o que remover.
    if (!tree->children) return;

    // --- RECURSÃO ---
    IVector3 mid;
    mid.x = tree->left_bot_back.x + size.x / 2;
    mid.y = tree->left_bot_back.y + size.y / 2;
    mid.z = tree->left_bot_back.z + size.z / 2;
    
    int pos = _get_pos_in_octree(coord, mid);

    octree_remove(tree->children[pos], coord);

    // --- LIMPEZA (Merge Empty) ---
    // Na volta, verificamos se todos os filhos ficaram vazios.
    // Se sim, deletamos os filhos e marcamos este nó como Ar.
    bool all_empty = true;
    for(int i=0; i<8; i++) {
        // Um filho não é vazio se tiver voxel OU se tiver netos
        if (tree->children[i]->has_voxel || tree->children[i]->children) {
            all_empty = false;
            break;
        }
    }

    if (all_empty) {
        for(int i=0; i<8; i++) free(tree->children[i]);
        free(tree->children);
        tree->children = NULL;
        tree->has_voxel = false; // Virou Ar
    } 
    // Opcional: Adicione _try_merge_children(tree) aqui se quiser que 
    // remover um bloco e colocar outro igual funda novamente.
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