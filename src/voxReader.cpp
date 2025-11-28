#include "voxReader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath>

// GLM é usado para matemática matricial (essencial para o grafo de cena)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- CONFIGURAÇÃO DE SEGURANÇA ---
// Limites rígidos para impedir que voxels "explodam" para fora da memória da Octree
const int SAFE_MIN_BOUND = -2048; 
const int SAFE_MAX_BOUND = 2048;

Voxel defaultVoxelType = voxels[0];

// --- ESTRUTURAS INTERNAS ---

struct VoxModel {
    glm::ivec3 size;
    struct VoxelData { uint8_t x, y, z, colorIndex; };
    std::vector<VoxelData> voxels;
};

enum NodeType { NODE_TRN, NODE_GRP, NODE_SHP };

struct SceneNode {
    int id;
    NodeType type;
    
    // TRN
    int child_node_id = -1;
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    uint8_t rotationByte = 4; // 4 = Identidade no padrão VOX
    
    // GRP
    std::vector<int> children_ids;
    
    // SHP
    int model_id = -1;
};

// --- HELPERS DE LEITURA ---

std::string ReadVoxString(FILE* fp) {
    int size;
    if (fread(&size, 4, 1, fp) != 1) return "";
    if (size <= 0 || size > 1024*1024) return ""; // Limite de segurança
    std::vector<char> buffer(size + 1);
    if (fread(buffer.data(), 1, size, fp) != (size_t)size) return "";
    buffer[size] = '\0';
    return std::string(buffer.data());
}

std::map<std::string, std::string> ReadVoxDict(FILE* fp) {
    std::map<std::string, std::string> dict;
    int numPairs;
    if (fread(&numPairs, 4, 1, fp) != 1) return dict;
    if (numPairs < 0 || numPairs > 1000) return dict; // Limite de segurança
    for (int i = 0; i < numPairs; i++) {
        std::string key = ReadVoxString(fp);
        std::string val = ReadVoxString(fp);
        dict[key] = val;
    }
    return dict;
}

// Função auxiliar para conversão segura float->int
inline int SafeRoundToInt(float value) {
    if (value >= 0.0f) {
        return static_cast<int>(value + 0.5f);
    } else {
        return static_cast<int>(value - 0.5f);
    }
}

// Converte o byte de rotação do MagicaVoxel para uma Matriz 4x4
glm::mat4 GetRotationMatrix(uint8_t rotByte) {
    // Formato VOX ROT:
    // bit | 0 1 | 2 3 | 4 | 5 | 6
    // val | r0  | r1  |s0 |s1 |s2
    
    int r0 = (rotByte & 3);
    int r1 = (rotByte >> 2) & 3;
    int r2 = 3 - r0 - r1; // A linha que sobra

    int s0 = (rotByte & 16) ? -1 : 1;
    int s1 = (rotByte & 32) ? -1 : 1;
    int s2 = (rotByte & 64) ? -1 : 1;

    // Monta a matriz 3x3
    glm::vec3 row0(0.0f), row1(0.0f), row2(0.0f);
    row0[r0] = (float)s0;
    row1[r1] = (float)s1;
    
    // A terceira linha é o produto vetorial das duas primeiras
    row2 = glm::cross(row0, row1);
    
    // Aplica o sinal da terceira linha se necessário
    if (s2 < 0) {
        row2 = -row2;
    }

    glm::mat4 mat(1.0f);
    // Preenche GLM (coluna por coluna)
    mat[0][0] = row0.x; mat[1][0] = row0.y; mat[2][0] = row0.z;
    mat[0][1] = row1.x; mat[1][1] = row1.y; mat[2][1] = row1.z;
    mat[0][2] = row2.x; mat[1][2] = row2.y; mat[2][2] = row2.z;

    return mat;
}

// --- TRAVESSIA DO GRAFO ---

void TraverseVoxGraph(
    int nodeId, 
    glm::mat4 parentTransform,
    const std::map<int, SceneNode>& nodes, 
    const std::vector<VoxModel>& models,
    const std::vector<ColorRGBA>& palette,
    Octree* tree,
    glm::ivec3 worldOrigin
) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return;

    const SceneNode& node = it->second;
    glm::mat4 currentTransform = parentTransform;

    // Aplica transformação se for um nó TRN
    if (node.type == NODE_TRN) {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), node.translation);
        glm::mat4 rotation = GetRotationMatrix(node.rotationByte);
        
        // Combina transformações
        currentTransform = parentTransform * translation * rotation;
        
        // Continua para o filho
        TraverseVoxGraph(node.child_node_id, currentTransform, nodes, models, palette, tree, worldOrigin);
    }
    else if (node.type == NODE_GRP) {
        // Grupo apenas repassa a matriz para os filhos
        for (int childId : node.children_ids) {
            TraverseVoxGraph(childId, currentTransform, nodes, models, palette, tree, worldOrigin);
        }
    }
    else if (node.type == NODE_SHP) {
        // Nó de Geometria (Shape)
        if (node.model_id < 0 || node.model_id >= (int)models.size()) {
            return; // Modelo inválido
        }
        
        const VoxModel& model = models[node.model_id];
        
        // Offset de centralização
        glm::vec3 centerOffset = glm::vec3(
            model.size.x / 2.0f, 
            model.size.y / 2.0f, 
            model.size.z / 2.0f
        );

        for (const auto& v : model.voxels) {
            // Validação do índice de cor
            int colorIdx = (int)v.colorIndex - 1;
            if (colorIdx < 0 || colorIdx >= 256) {
                colorIdx = 0; // Cor padrão
            }

            // Posição local do voxel
            glm::vec4 localPos(
                (float)v.x - centerOffset.x, 
                (float)v.y - centerOffset.y, 
                (float)v.z - centerOffset.z, 
                1.0f
            );

            // Aplica a transformação acumulada do grafo
            glm::vec4 finalPos = currentTransform * localPos;

            // Conversão segura float->int + offset do mundo
            // TROCA DE EIXOS: MagicaVoxel (X, Y, Z) -> Engine (X, Z, Y)
            // MV: X=Right, Y=Forward, Z=Up
            // Engine (assumindo): X=Right, Y=Up, Z=Forward
            
            int fx = worldOrigin.x + SafeRoundToInt(finalPos.x);
            int fy = worldOrigin.y + SafeRoundToInt(finalPos.z); // Z do VOX -> Y da Engine
            int fz = worldOrigin.z + SafeRoundToInt(finalPos.y); // Y do VOX -> Z da Engine

            // Proteção de Limites
            if (fx < SAFE_MIN_BOUND || fx > SAFE_MAX_BOUND ||
                fy < SAFE_MIN_BOUND || fy > SAFE_MAX_BOUND ||
                fz < SAFE_MIN_BOUND || fz > SAFE_MAX_BOUND) {
                continue;
            }

            // Criação e inserção do voxel
            Voxel_Object voxel = VoxelObjCreate(
                defaultVoxelType,  // CORRIGIDO: usa o tipo passado como parâmetro
                palette[colorIdx], 
                {fx, fy, fz}
            );
            octree_insert(tree, voxel);
        }
    }
}

// --- FUNÇÃO PRINCIPAL ---

bool load_vox_file(const char* filename, Octree* tree, int offsetX, int offsetY, int offsetZ) {
    if (!tree) {
        std::cerr << "Erro: Octree é NULL!" << std::endl;
        return false;
    }

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Erro ao abrir arquivo: " << filename << std::endl;
        return false;
    }

    // Cabeçalho
    char header[4];
    int version;
    if (fread(header, 1, 4, fp) != 4) { fclose(fp); return false; }
    if (fread(&version, 4, 1, fp) != 1) { fclose(fp); return false; }

    if (strncmp(header, "VOX ", 4) != 0) {
        std::cerr << "Arquivo invalido (Header != VOX)." << std::endl;
        fclose(fp);
        return false;
    }

    std::vector<VoxModel> models;
    std::map<int, SceneNode> sceneNodes;
    std::vector<ColorRGBA> palette(256);
    
    // Inicializa paleta padrão (grayscale)
    for(int i=0; i<256; i++) {
        palette[i] = make_color_rgba(i, i, i, 255);
    }

    glm::ivec3 lastSize = {0,0,0};
    
    // Tamanho do arquivo
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 8, SEEK_SET);

    // Parser de Chunks
    while (ftell(fp) < fileSize - 12) { // -12 para garantir espaço para header do chunk
        char chunkId[4];
        int contentSize, childrenSize;

        if (fread(chunkId, 1, 4, fp) < 4) break;
        if (fread(&contentSize, 4, 1, fp) < 1) break;
        if (fread(&childrenSize, 4, 1, fp) < 1) break;
        
        // Validação de tamanhos
        if (contentSize < 0 || childrenSize < 0) {
            std::cerr << "Tamanhos de chunk inválidos" << std::endl;
            break;
        }
        
        long nextChunkPos = ftell(fp) + contentSize;
        long endOfChunkPos = nextChunkPos + childrenSize;
        
        // Verifica se não excede o arquivo
        if (endOfChunkPos > fileSize) {
            std::cerr << "Chunk excede tamanho do arquivo" << std::endl;
            break;
        }

        if (strncmp(chunkId, "MAIN", 4) == 0) {
            continue; 
        }
        else if (strncmp(chunkId, "PACK", 4) == 0) {
            fseek(fp, contentSize, SEEK_CUR); 
        }
        else if (strncmp(chunkId, "SIZE", 4) == 0) {
            fread(&lastSize.x, 4, 1, fp);
            fread(&lastSize.y, 4, 1, fp);
            fread(&lastSize.z, 4, 1, fp);
        }
        else if (strncmp(chunkId, "XYZI", 4) == 0) {
            int numVoxels;
            fread(&numVoxels, 4, 1, fp);
            
            if (numVoxels < 0 || numVoxels > 10000000) { // Limite de segurança
                std::cerr << "Número de voxels suspeito: " << numVoxels << std::endl;
                fseek(fp, endOfChunkPos, SEEK_SET);
                continue;
            }
            
            VoxModel model;
            model.size = lastSize;
            if (numVoxels > 0) {
                model.voxels.resize(numVoxels);
                for(int i=0; i<numVoxels; i++) {
                    fread(&model.voxels[i].x, 1, 1, fp);
                    fread(&model.voxels[i].y, 1, 1, fp);
                    fread(&model.voxels[i].z, 1, 1, fp);
                    fread(&model.voxels[i].colorIndex, 1, 1, fp);
                }
            }
            models.push_back(model);
        }
        else if (strncmp(chunkId, "RGBA", 4) == 0) {
            for (int i = 0; i < 256; ++i) {
                unsigned char r, g, b, a;
                fread(&r, 1, 1, fp); fread(&g, 1, 1, fp);
                fread(&b, 1, 1, fp); fread(&a, 1, 1, fp);
                palette[i] = make_color_rgba(r, g, b, a);
            }
        }
        else if (strncmp(chunkId, "nTRN", 4) == 0) {
            SceneNode node;
            node.type = NODE_TRN;
            fread(&node.id, 4, 1, fp);
            ReadVoxDict(fp); 
            fread(&node.child_node_id, 4, 1, fp);
            int reserved, layerId, numFrames;
            fread(&reserved, 4, 1, fp);
            fread(&layerId, 4, 1, fp);
            fread(&numFrames, 4, 1, fp);

            for(int i=0; i<numFrames; i++) {
                std::map<std::string, std::string> dict = ReadVoxDict(fp);
                if (i == 0) { // Frame 0
                    if (dict.count("_t")) {
                        std::stringstream ss(dict["_t"]);
                        ss >> node.translation.x >> node.translation.y >> node.translation.z;
                    }
                    if (dict.count("_r")) {
                        node.rotationByte = (uint8_t)std::stoi(dict["_r"]);
                    }
                }
            }
            sceneNodes[node.id] = node;
        }
        else if (strncmp(chunkId, "nGRP", 4) == 0) {
            SceneNode node;
            node.type = NODE_GRP;
            fread(&node.id, 4, 1, fp);
            ReadVoxDict(fp); 
            int numChildren;
            fread(&numChildren, 4, 1, fp);
            for(int i=0; i<numChildren; i++) {
                int childId;
                fread(&childId, 4, 1, fp);
                node.children_ids.push_back(childId);
            }
            sceneNodes[node.id] = node;
        }
        else if (strncmp(chunkId, "nSHP", 4) == 0) {
            SceneNode node;
            node.type = NODE_SHP;
            fread(&node.id, 4, 1, fp);
            ReadVoxDict(fp); 
            int numModels;
            fread(&numModels, 4, 1, fp);
            for(int i=0; i<numModels; i++) {
                int modelId;
                fread(&modelId, 4, 1, fp);
                ReadVoxDict(fp); 
                if (i == 0) node.model_id = modelId;
            }
            sceneNodes[node.id] = node;
        }
        
        fseek(fp, endOfChunkPos, SEEK_SET);
    }
    fclose(fp);

    // --- CONSTRUÇÃO DO MUNDO ---

    if (sceneNodes.empty()) {
        // Modo RAW (Fallback para arquivos sem nTRN)
        if (!models.empty()) {
            std::cout << "Grafo nTRN ausente. Carregando modo RAW." << std::endl;
        }
        int count = 0;
        for(const auto& model : models) {
            for(const auto& v : model.voxels) {
                int colorIdx = (int)v.colorIndex - 1;
                if(colorIdx < 0 || colorIdx >= 256) colorIdx = 0;
                
                int fx = offsetX + v.x;
                int fy = offsetY + v.z; // Swap Z->Y
                int fz = offsetZ + v.y; // Swap Y->Z
                
                if (fx < SAFE_MIN_BOUND || fx > SAFE_MAX_BOUND ||
                    fy < SAFE_MIN_BOUND || fy > SAFE_MAX_BOUND ||
                    fz < SAFE_MIN_BOUND || fz > SAFE_MAX_BOUND) continue;

                Voxel_Object voxel = VoxelObjCreate(defaultVoxelType, palette[colorIdx], {fx, fy, fz});
                octree_insert(tree, voxel);
                count++;
            }
        }
        std::cout << "Carregados " << count << " voxels (modo RAW)." << std::endl;
        return count > 0;
    }

    // Modo Grafo de Cena
    if (sceneNodes.count(0)) {
        std::cout << "Processando Grafo de Cena (" << sceneNodes.size() << " nos)..." << std::endl;
        TraverseVoxGraph(0, glm::mat4(1.0f), sceneNodes, models, palette, tree, 
                        {offsetX, offsetY, offsetZ});
    }

    return true;
}