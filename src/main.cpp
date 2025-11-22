#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <ShaderUtils.hpp>
#include <Camera.hpp>
#include <voxel.hpp>
#include <octree.hpp>

extern "C" {
    #include <color.h>
}

#define WORLD_SIZE_X 256
#define WORLD_SIZE_Y 256
#define WORLD_SIZE_Z 256

// --- PHYSICS CONSTANTS ---
const float PLAYER_WIDTH = 1.6f;  // Voxel scale relative (1 voxel = 1 unit usually)
const float PLAYER_HEIGHT = 4.8f; // Standard height
const float EYE_LEVEL = 4.7f;

const float GRAVITY = 80.0f;
const float JUMP_FORCE = 25.0f;
const float MOVE_SPEED = 20.0f;
const float FRICTION = 2.0f;
const float AIR_RESISTANCE = 1.0f;

glm::vec3 playerVelocity(0.0f);
bool isGrounded = false;

// --- BUILD STATE ---
int selectedMaterialIndex = 2; // Default to Wood
bool worldDirty = false;       // Flag to tell us if we need to update GPU

// --- GL GLOBALS ---
// We make these global (or struct members) so the update function can access them
GLuint textureID; 
GLuint pboID;
size_t currentTexDim = 0; // Track texture size to know if we need to resize

std::string computeSrc = ShaderUtils::readShaderFile("shaders/raytracing.comp");
std::string quadVsSrc  = ShaderUtils::readShaderFile("shaders/quad.vert");
std::string quadFsSrc  = ShaderUtils::readShaderFile("shaders/quad.frag");

const char* compute = computeSrc.c_str();
const char* quad_vs = quadVsSrc.c_str();
const char* quad_fs = quadFsSrc.c_str();

const int screenWidth = 1280;
const int screenHeight = 720;

Camera camera(glm::vec3(64.0f, 60.0f, 64.0f)); // Adjusted camera start to be above ground

// Variáveis para o mouse
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

// Variáveis para o tempo (deltaTime)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

typedef struct Vertex
{
    glm::vec2 pos;
} Vertex;

static const Vertex quadVertices[] =
{
    { {  1.0f, 1.0f } }, //top right
    { {  1.0f, -1.0f } }, //bottom right
    { {  -1.0f, -1.0f } }, //bottom left
    { { -1.0f, 1.0f } } //top left
};

unsigned int quadIndices[] = {
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};

// Returns true if a voxel exists at (x,y,z) using your octree_find
bool isVoxelSolid(Octree* tree, int x, int y, int z) {
    IVector3 coord = {x, y, z};
    Voxel_Object v = octree_find(tree, coord);
    
    // In your octree code, _invalid_voxel sets y = MIN_HEIGHT (-256)
    // If the returned voxel has a Y > MIN_HEIGHT, it is a real voxel.
    return (v.coord.y > -256);
}

// AABB Collision Detection
bool checkCollision(Octree* tree, glm::vec3 pos) {
    // Determine the integer bounds of the player's bounding box
    int minX = floor(pos.x - PLAYER_WIDTH / 2.0f);
    int maxX = floor(pos.x + PLAYER_WIDTH / 2.0f);
    int minY = floor(pos.y);
    int maxY = floor(pos.y + PLAYER_HEIGHT - 1.0f); // -1 to avoid head sticking in ceiling
    int minZ = floor(pos.z - PLAYER_WIDTH / 2.0f);
    int maxZ = floor(pos.z + PLAYER_WIDTH / 2.0f);

    

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (isVoxelSolid(tree, x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Libera o cursor
}

void processInput(GLFWwindow *window) {
    glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
    glm::vec3 wishDir(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wishDir += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wishDir -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wishDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wishDir += right;

    if (glm::length(wishDir) > 0.01f) {
        wishDir = glm::normalize(wishDir);
        playerVelocity.x += wishDir.x * MOVE_SPEED * deltaTime;
        playerVelocity.z += wishDir.z * MOVE_SPEED * deltaTime;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        playerVelocity.y = JUMP_FORCE;
        isGrounded = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void checkShaderCompilation(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }
}

void checkProgramLinking(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERRO::PROGRAMA::LINK_FALHOU\n" << infoLog << std::endl;
    }
}

// Lista de todos os tipos de voxels possívels
// IOF, Illumination, Metallicity
Voxel voxels[] = {
    {3.0f, 0.0f, 0.0f},   // VOX_SNOW (Matte, bright)
    {3.0f, 0.0f, 0.0f},   // VOX_DIRT
    {3.0f, 0.0f, 0.0f},   // VOX_DARK_WOOD (Trunks)
    {3.0f, 0.0f, 0.0f},   // VOX_SAKURA_LEAVES
    {1.33f, 0.0f, 0.0f},  // VOX_WATER
    {3.0f, 0.0f, 0.0f},   // VOX_QUARTZ (House structure)
    {1.52f, 0.0f, 0.0f},  // VOX_CLEAR_GLASS (High transparency)
    {2.42f, 0.0f, 0.0f},  // VOX_DIAMOND
    {1.38f, 2.0f, 0.0f},  // VOX_LANTERN (Glowing)
    {3.0f, 0.0f, 1.0f},   // VOX_MIRROR
};

Voxel_Type VOX_SNOW = 0;
Voxel_Type VOX_DIRT = 1;
Voxel_Type VOX_DARK_WOOD = 2;
Voxel_Type VOX_SAKURA_LEAVES = 3;
Voxel_Type VOX_WATER = 4;
Voxel_Type VOX_QUARTZ = 5;
Voxel_Type VOX_CLEAR_GLASS = 6;
Voxel_Type VOX_DIAMOND = 7;
Voxel_Type VOX_LANTERN = 8;
Voxel_Type VOX_MIRROR = 9;

// Colors for the materials above (Simplification)
ColorRGBA voxelColors[] = {
    make_color_rgba(80, 180, 60, 255),   // Grass
    make_color_rgba(100, 70, 40, 255),   // Dirt
    make_color_rgba(120, 70, 30, 255),   // Wood
    make_color_rgba(30, 160, 30, 255),   // Leaves
    make_color_rgba(60, 100, 220, 150),  // Water
    make_color_rgba(160, 160, 160, 255), // Stone
    make_color_rgba(200, 220, 255, 80),  // Glass
    make_color_rgba(0, 255, 255, 255),   // Diamond
    make_color_rgba(255, 100, 100, 180), // Jelly
    make_color_rgba(255, 255, 255, 255), // Mirror
};

void updateGPUTexture(Octree* tree) {
    size_t total_texels = _octree_texel_size(tree);
    size_t tex_dim = (size_t)ceil(cbrt((double)total_texels));
    if (tex_dim == 0) tex_dim = 1;

    size_t arr_size;
    uint8_t* texture_data = octree_texture(tree, &arr_size, tex_dim);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, textureID);
    
    // Resize texture if the octree grew significantly
    // (In a real engine you might allocate a large fixed size to avoid this)
    if (tex_dim != currentTexDim) {
        currentTexDim = tex_dim;
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8UI, (GLsizei)tex_dim, (GLsizei)tex_dim, (GLsizei)tex_dim, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
        
        // Reallocate PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboID);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, tex_dim*tex_dim*tex_dim*4, NULL, GL_STREAM_DRAW);
    } else {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboID);
    }

    // Upload
    void* gpu_pointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (gpu_pointer) {
        memcpy(gpu_pointer, texture_data, arr_size);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0,0,0, (GLsizei)tex_dim, (GLsizei)tex_dim, (GLsizei)tex_dim, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    free(texture_data);
}

// --- HELPER: MATH FOR CONSTRUCTION ---
// Calculates intersection of ray with a box to find the surface normal side
glm::ivec3 get_placement_coord(glm::vec3 origin, glm::vec3 dir, glm::ivec3 targetVoxel) {
    // Define the AABB of the target voxel
    glm::vec3 boxMin = glm::vec3(targetVoxel);
    glm::vec3 boxMax = boxMin + 1.0f;

    // Calculate intersection distances to planes (Slab Method)
    // We handle division by zero by letting float infinity do its work
    float tMinX = (boxMin.x - origin.x) / dir.x;
    float tMaxX = (boxMax.x - origin.x) / dir.x;
    float tMinY = (boxMin.y - origin.y) / dir.y;
    float tMaxY = (boxMax.y - origin.y) / dir.y;
    float tMinZ = (boxMin.z - origin.z) / dir.z;
    float tMaxZ = (boxMax.z - origin.z) / dir.z;

    // Ensure Min is the near intersection and Max is the far
    if (tMinX > tMaxX) std::swap(tMinX, tMaxX);
    if (tMinY > tMaxY) std::swap(tMinY, tMaxY);
    if (tMinZ > tMaxZ) std::swap(tMinZ, tMaxZ);

    // The entry intersection 't' is the largest of the near values
    // (The ray must pass X, Y, and Z planes to enter the volume)
    float tEntry = fmax(fmax(tMinX, tMinY), tMinZ);

    // Determine which axis corresponds to tEntry. 
    // That axis is the face we hit.
    glm::ivec3 placement = targetVoxel;

    // Use a small epsilon to handle floating point errors
    const float EPSILON = 1e-4f;

    if (fabs(tEntry - tMinX) < EPSILON) {
        // Hit the X plane.
        // If dir.x > 0, we hit the Left face (min x), so place at x - 1
        // If dir.x < 0, we hit the Right face (max x), so place at x + 1
        placement.x += (dir.x > 0) ? -1 : 1;
    } 
    else if (fabs(tEntry - tMinY) < EPSILON) {
        // Hit the Y plane.
        placement.y += (dir.y > 0) ? -1 : 1;
    } 
    else {
        // Hit the Z plane.
        placement.z += (dir.z > 0) ? -1 : 1;
    }

    return placement;
}

int main(void)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Voxel Raytracing", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Captura e esconde o cursor
    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(0);

    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture);

    // Configurar a textura para ter o tamanho da tela, sem dados iniciais.
    // Usamos RGBA8 para ter 4 canais de 8 bits para a cor.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Configurações da textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Initialize Global GL objects
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenBuffers(1, &pboID);

    glm::ivec3 min_bounds(0, 0, 0);
    glm::ivec3 max_bounds(WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);
    Octree* chunk0 = octree_create(NULL, {0, 0, 0}, {WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z});

    glm::vec4 global_light(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 light_dir = glm::normalize(glm::vec3(0.3481553f, 0.870388f, 0.3481553f));

    // --- SCENE GENERATION: SNOWY SAKURA & GLASS MANSION ---
    
    // 1. BASE TERRAIN (Snow Plane)
    int groundLevel = 18;
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int z = 0; z < WORLD_SIZE_Z; ++z) {
            // Fill dirt below
            for(int y = 0; y < groundLevel - 2; ++y) {
                 Voxel_Object voxel = VoxelObjCreate(voxels[VOX_DIRT], make_color_rgba(60, 40, 30, 255), {x, y, z});
                 octree_insert(chunk0, voxel);
            }
            // Top layers are snow
            for(int y = groundLevel - 2; y <= groundLevel; ++y) {
                 Voxel_Object voxel = VoxelObjCreate(voxels[VOX_SNOW], make_color_rgba(240, 248, 255, 255), {x, y, z});
                 octree_insert(chunk0, voxel);
            }
        }
    }

    // 2. THE HUGE GLASS MANSION
    int houseCx = WORLD_SIZE_X / 2;
    int houseCz = WORLD_SIZE_Z / 2;
    int houseWidth = 70;  // Total width
    int houseDepth = 60;  // Total depth
    int houseStartY = groundLevel + 1;
    
    int hMinX = houseCx - houseWidth/2;
    int hMaxX = houseCx + houseWidth/2;
    int hMinZ = houseCz - houseDepth/2;
    int hMaxZ = houseCz + houseDepth/2;

    int floor1Height = 18;
    int floor2Height = 16;
    int roofHeight = 2;

    // Construct the House
    for (int x = hMinX; x <= hMaxX; ++x) {
        for (int y = houseStartY; y <= houseStartY + floor1Height + floor2Height + roofHeight; ++y) {
            for (int z = hMinZ; z <= hMaxZ; ++z) {
                
                // Determine relative height in house
                int relY = y - houseStartY;
                
                // Check Borders
                bool isWallX = (x == hMinX || x == hMaxX);
                bool isWallZ = (z == hMinZ || z == hMaxZ);
                bool isCorner = isWallX && isWallZ;
                bool isPillar = (isWallX || isWallZ) && ((x % 15 == 0) || (z % 15 == 0)); // Structural pillars
                
                // Floors and Ceilings
                bool isFloor1 = (relY == 0);
                bool isCeiling1 = (relY == floor1Height);
                bool isCeiling2 = (relY == floor1Height + floor2Height);
                
                // --- ARCHITECTURE LOGIC ---

                // 1. Solid Quartz Foundation and Frame
                if (isFloor1 || isCeiling1 || isCeiling2 || isCorner || (isPillar && !isCeiling2)) {
                    Voxel_Object voxel = VoxelObjCreate(voxels[VOX_QUARTZ], make_color_rgba(235, 235, 235, 255), {x, y, z});
                    octree_insert(chunk0, voxel);
                }
                // 2. Glass Curtain Walls (Huge windows reflecting forest)
                else if (isWallX || isWallZ) {
                    if (relY < floor1Height + floor2Height) {
                         // Use low alpha for transparency simulation (if supported by shader logic)
                         // Blue tint for glass
                        Voxel_Object voxel = VoxelObjCreate(voxels[VOX_CLEAR_GLASS], make_color_rgba(200, 220, 255, 60), {x, y, z});
                        octree_insert(chunk0, voxel);
                    }
                }
                // 3. Interior details (Stairs/Lights)
                // Add a central lantern/chandelier
                else if (x == houseCx && z == houseCz && (relY == floor1Height - 2 || relY == floor1Height + floor2Height - 2)) {
                     Voxel_Object voxel = VoxelObjCreate(voxels[VOX_LANTERN], make_color_rgba(255, 200, 100, 255), {x, y, z});
                     octree_insert(chunk0, voxel);
                }
                // Rooftop Railing
                else if (relY == floor1Height + floor2Height + 1) {
                    if (x > hMinX && x < hMaxX && z > hMinZ && z < hMaxZ) {
                         // Empty walking space on roof
                    } else if (isWallX || isWallZ) {
                         // Railing
                         Voxel_Object voxel = VoxelObjCreate(voxels[VOX_MIRROR], make_color_rgba(100, 100, 100, 255), {x, y, z});
                         octree_insert(chunk0, voxel);
                    }
                }
            }
        }
    }

    // 3. SAKURA TREES (Procedural & Non-Concentrated)
    srand(12345); // Seed for consistent generation
    int numTrees = 350; // Lots of trees, but spread over 256x256
    
    for(int i = 0; i < numTrees; ++i) {
        int tx = rand() % (WORLD_SIZE_X - 10) + 5;
        int tz = rand() % (WORLD_SIZE_Z - 10) + 5;

        // Do not spawn inside the house or too close to it
        if (tx >= hMinX - 5 && tx <= hMaxX + 5 && tz >= hMinZ - 5 && tz <= hMaxZ + 5) {
            continue;
        }

        int treeHeight = 8 + (rand() % 8); // Random height 8-16
        int canopyRadius = 4 + (rand() % 3); // Big gorgeous canopy

        // Trunk
        for(int ty = groundLevel + 1; ty < groundLevel + treeHeight; ++ty) {
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_DARK_WOOD], make_color_rgba(60, 30, 10, 255), {tx, ty, tz});
            octree_insert(chunk0, voxel);
        }

        // Canopy (Spherical-ish clouds of pink)
        int canopyCenterY = groundLevel + treeHeight;
        for (int cx = tx - canopyRadius; cx <= tx + canopyRadius; ++cx) {
            for (int cy = canopyCenterY - canopyRadius; cy <= canopyCenterY + canopyRadius; ++cy) {
                for (int cz = tz - canopyRadius; cz <= tz + canopyRadius; ++cz) {
                    // Bounds check
                    if (cx < 0 || cx >= WORLD_SIZE_X || cy < 0 || cy >= WORLD_SIZE_Y || cz < 0 || cz >= WORLD_SIZE_Z) continue;

                    float dist = sqrt(pow(cx - tx, 2) + pow(cy - canopyCenterY, 2) + pow(cz - tz, 2));
                    
                    if (dist < canopyRadius) {
                        // Randomize pink shades for "gorgeous" detail
                        int r = 255;
                        int g = 180 + (rand() % 40); // 180-220
                        int b = 190 + (rand() % 40); // 190-230
                        
                        // Randomly miss some voxels to make leaves look airy, not solid blocks
                        if ((rand() % 10) > 1) { 
                            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_SAKURA_LEAVES], make_color_rgba(r, g, b, 255), {cx, cy, cz});
                            octree_insert(chunk0, voxel);
                        }
                    }
                }
            }
        }

        // Fallen Petals on the snow (detail)
        for(int fx = tx - canopyRadius; fx <= tx + canopyRadius; ++fx) {
            for(int fz = tz - canopyRadius; fz <= tz + canopyRadius; ++fz) {
                if (rand() % 20 == 0 && fx >= 0 && fx < WORLD_SIZE_X && fz >= 0 && fz < WORLD_SIZE_Z) {
                    // Place on top of snow
                    Voxel_Object voxel = VoxelObjCreate(voxels[VOX_SAKURA_LEAVES], make_color_rgba(255, 192, 203, 255), {fx, groundLevel + 1, fz});
                    octree_insert(chunk0, voxel);
                }
            }
        }
    }


    // --- END OF SCENE GENERATION ---

    size_t total_texels = _octree_texel_size(chunk0);
    size_t tex_dim = (size_t)ceil(cbrt((double)total_texels));
    if (tex_dim == 0) tex_dim = 1;

    float voxelScale = 1.0; // A escala do voxel no mundo, ex: 2.0 significa 1 voxel a cada 0.5 unidades de espaço

    // Initial GPU Upload
    updateGPUTexture(chunk0);

    // UBO para a câmera
    struct CameraData {
        glm::mat4 invProjection;
        glm::mat4 invView;
        glm::vec4 cameraPos;
    };

    GLuint uboCamera;
    glGenBuffers(1, &uboCamera);
    glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboCamera);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    // Configurar o Quad renderizado na tela
    GLuint quadVAO, quadVBO, quadEBO;

    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &quadEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0); // Ativar o atributo

    glBindVertexArray(0);

    // Configurar o Compute Shader
    const GLuint compute_raytracing = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_raytracing, 1, &compute, NULL);
    glCompileShader(compute_raytracing);
    checkShaderCompilation(compute_raytracing);

    const GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, compute_raytracing);
    glLinkProgram(computeProgram);
    checkProgramLinking(computeProgram);

    GLint texDimLoc = glGetUniformLocation(computeProgram, "u_texDim");
    GLint voxScaleLoc = glGetUniformLocation(computeProgram, "u_voxelScale");
    GLint minBoundsLoc = glGetUniformLocation(computeProgram, "u_worldBoundsMin");
    GLint maxBoundsLoc = glGetUniformLocation(computeProgram, "u_worldBoundsMax");
    GLint globalLightLoc = glGetUniformLocation(computeProgram, "globalLight");
    GLint lightDirLoc = glGetUniformLocation(computeProgram, "lightDir");

    // Configurar os shaders do Quad
    const GLuint quad_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(quad_vertex_shader, 1, &quad_vs, NULL);
    glCompileShader(quad_vertex_shader);
    checkShaderCompilation(quad_vertex_shader);

    const GLuint quad_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(quad_fragment_shader, 1, &quad_fs, NULL);
    glCompileShader(quad_fragment_shader);
    checkShaderCompilation(quad_fragment_shader);

    const GLuint quadProgram = glCreateProgram();
    glAttachShader(quadProgram, quad_vertex_shader);
    glAttachShader(quadProgram, quad_fragment_shader);
    glLinkProgram(quadProgram);
    checkProgramLinking(quadProgram);
    glUseProgram(quadProgram);
    glUniform1i(glGetUniformLocation(quadProgram, "screenTexture"), 0);

    float now, lastTime = 0.0f;
    // Dentro do seu game loop
    while (!glfwWindowShouldClose(window)) {
        now = (float)glfwGetTime();
        //std::cout << "FPS: " << 1.0f / (now - lastTime) << "\n";
        lastTime = now;
        
        // Lógica do deltaTime
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Processar input do teclado
        processInput(window);

        // 2. PHYSICS UPDATE (The loop I designed)
        // Apply friction
        float damping = isGrounded ? FRICTION : AIR_RESISTANCE;
        playerVelocity.x -= playerVelocity.x * damping * deltaTime;
        playerVelocity.z -= playerVelocity.z * damping * deltaTime;
        
        // Apply Gravity
        playerVelocity.y -= GRAVITY * deltaTime;

        // Current position (Feet)
        glm::vec3 feetPos = camera.Position;
        feetPos.y -= EYE_LEVEL;

        // Move & Collide X
        feetPos.x += playerVelocity.x * deltaTime;
        if (checkCollision(chunk0, feetPos)) {
            feetPos.x -= playerVelocity.x * deltaTime;
            playerVelocity.x = 0;
        }

        // Move & Collide Z
        feetPos.z += playerVelocity.z * deltaTime;
        if (checkCollision(chunk0, feetPos)) {
            feetPos.z -= playerVelocity.z * deltaTime;
            playerVelocity.z = 0;
        }

        // Move & Collide Y
        feetPos.y += playerVelocity.y * deltaTime;
        isGrounded = false;
        if (checkCollision(chunk0, feetPos)) {
            if (playerVelocity.y < 0) isGrounded = true;
            feetPos.y -= playerVelocity.y * deltaTime;
            playerVelocity.y = 0;
        }

        // Update Camera
        camera.Position = feetPos;
        camera.Position.y += EYE_LEVEL;
        
        // Atualizar os dados da câmera
        CameraData cameraData;
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
        
        cameraData.invProjection = glm::inverse(projection);
        cameraData.invView = glm::inverse(view);
        cameraData.cameraPos = glm::vec4(camera.Position, 1.0f);
        
        // --- INTERACTION LOGIC (BUILD/DESTROY) ---
        static glm::ivec3 highlightedVoxel(-1);
        static bool leftWasDown = false;
        static bool rightWasDown = false;

        // 1. Raycast to find what we are looking at
        Ray ray;
        ray.origin = {camera.Position.x, camera.Position.y, camera.Position.z};
        ray.direction = {camera.Front.x, camera.Front.y, camera.Front.z};
        Octree* hitNode = octree_ray_cast(chunk0, ray, {0,0,0}, {(float)WORLD_SIZE_X, (float)WORLD_SIZE_Y, (float)WORLD_SIZE_Z});

        if (hitNode && hitNode->has_voxel) {
            highlightedVoxel = {hitNode->voxel.coord.x, hitNode->voxel.coord.y, hitNode->voxel.coord.z};
        } else {
            highlightedVoxel = glm::ivec3(-1);
        }

        // 2. Handle Clicks
        // LEFT CLICK: DESTROY
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftWasDown) {
            if (highlightedVoxel.x != -1) {
                // IMPORTANT: This calls the function you haven't implemented yet.
                // It will crash if octree_remove isn't linked, so I added a stub at top.
                IVector3 target = {highlightedVoxel.x, highlightedVoxel.y, highlightedVoxel.z};
                octree_remove(chunk0, target); 
                worldDirty = true;
            }
        }
        leftWasDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

        // RIGHT CLICK: BUILD
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !rightWasDown) {
            if (highlightedVoxel.x != -1) {
                std::cout << "coord: " << highlightedVoxel.x << " " << highlightedVoxel.y << " " << highlightedVoxel.z << std::endl;
                printf("color: %d %d %d\n\n", get_red_rgba(hitNode->voxel.color), get_blue_rgba(hitNode->voxel.color), get_green_rgba(hitNode->voxel.color));
                // Find WHERE to place (Neighbor)
                //glm::ivec3 placeCoord = get_placement_coord(camera.Position, camera.Front, highlightedVoxel);
                glm::ivec3 placeCoord = highlightedVoxel + glm::ivec3{0, 1, 0};

                // Don't place inside player
                glm::vec3 pPos = camera.Position; pPos.y -= EYE_LEVEL;
                bool insidePlayer = (placeCoord.x >= floor(pPos.x - PLAYER_WIDTH/2) && placeCoord.x <= floor(pPos.x + PLAYER_WIDTH/2) &&
                                     placeCoord.y >= floor(pPos.y) && placeCoord.y <= floor(pPos.y + PLAYER_HEIGHT) &&
                                     placeCoord.z >= floor(pPos.z - PLAYER_WIDTH/2) && placeCoord.z <= floor(pPos.z + PLAYER_WIDTH/2));

                if (!insidePlayer) {
                    Voxel_Object newVoxel = VoxelObjCreate(voxels[selectedMaterialIndex], voxelColors[selectedMaterialIndex], 
                        {placeCoord.x, placeCoord.y, placeCoord.z});
                    octree_insert(chunk0, newVoxel);
                    worldDirty = true;
                }
            }
        }
        rightWasDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

        // 3. Update GPU if dirty
        if (worldDirty) {
            updateGPUTexture(chunk0);
            worldDirty = false;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &cameraData);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Executar o Compute Shader
        glUseProgram(computeProgram);
        
        // Vincular recursos
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, textureID);
        glUniform1i(texDimLoc, (GLint)tex_dim);
        glUniform1f(voxScaleLoc, (GLfloat)voxelScale);
        glUniform3iv(minBoundsLoc, 1, (const GLint*)&min_bounds);
        glUniform3iv(maxBoundsLoc, 1, (const GLint*)&max_bounds);
        glUniform4fv(globalLightLoc, 1, (const GLfloat*)&global_light);
        glUniform3fv(lightDirLoc, 1, (const GLfloat*)&light_dir);

        // Dispara os threads do compute shader.
        // Dividimos o tamanho da tela pelo tamanho do grupo de trabalho definido no shader.
        // Se o tamanho do grupo for 8x8, por exemplo:
        glDispatchCompute(screenWidth / 8, screenHeight / 8, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Desenhar o quad na tela
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(quadProgram);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadEBO);
    glDeleteBuffers(1, &pboID);
    glDeleteTextures(1, &textureID);
    glDeleteTextures(1, &outputTexture);
    glDeleteShader(compute_raytracing);
    glDeleteProgram(computeProgram);
    glDeleteShader(quad_vertex_shader);
    glDeleteShader(quad_fragment_shader);
    glDeleteProgram(quadProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}