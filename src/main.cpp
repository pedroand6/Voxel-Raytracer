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

#define WORLD_VOLUME WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z

std::string computeSrc = ShaderUtils::readShaderFile("shaders/raytracing.comp");
std::string quadVsSrc  = ShaderUtils::readShaderFile("shaders/quad.vert");
std::string quadFsSrc  = ShaderUtils::readShaderFile("shaders/quad.frag");

const char* compute = computeSrc.c_str();
const char* quad_vs = quadVsSrc.c_str();
const char* quad_fs = quadFsSrc.c_str();

const int screenWidth = 1280;
const int screenHeight = 720;

Camera camera(glm::vec3(32.0f, 32.0f, 20.0f));

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

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Libera o cursor
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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
    glfwSwapInterval(1);

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

    glm::vec3 min_bounds(0.0f, 0.0f, 0.0f);
    glm::vec3 max_bounds(WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);
    Octree* chunk0 = octree_create(NULL, {0,0,0}, {WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z});

    // Lista de todos os tipos de voxels possívels
    Voxel voxels[] = {
        {3.0f, 0.0f, 0.0f}, // VOX_GRASS
        {3.0f, 0.0f, 0.0f}, // VOX_DIRT
        {3.0f, 0.0f, 0.0f}, // VOX_WOOD
        {3.0f, 0.0f, 0.0f}, // VOX_LEAVES
        {1.33f, 0.0f, 0.0f}, // VOX_WATER
        {3.0f, 0.0f, 0.0f},  // VOX_STONE
        {1.5f, 0.0f, 0.0f},  // VOX_GLASS
        {2.42f, 0.0f, 0.0f},  // VOX_DIAMOND
        {1.38f, 0.0f, 0.0f}  // VOX_JELLY
    };

    Voxel_Type VOX_GRASS = 0;
    Voxel_Type VOX_DIRT = 1;
    Voxel_Type VOX_WOOD = 2;
    Voxel_Type VOX_LEAVES = 3;
    Voxel_Type VOX_WATER = 4;
    Voxel_Type VOX_STONE = 5;
    Voxel_Type VOX_GLASS = 6;
    Voxel_Type VOX_DIAMOND = 7;
    Voxel_Type VOX_JELLY = 8;

    // Room parameters (positioned near the center of the world)
    int roomMinX = 12;
    int roomMaxX = 51;
    int roomMinZ = 12;
    int roomMaxZ = 51;
    int floorY = 20;
    int wallHeight = 20;

    // Create a grass floor
    for (int x = roomMinX; x <= roomMaxX; ++x) {
        for (int z = roomMinZ; z <= roomMaxZ; ++z) {
            int index = x + floorY * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_GRASS], make_color_rgba(100, 200, 80, 255), {x, floorY, z});
            octree_insert(chunk0, voxel);
        }
    }

    // Build walls: 3 wood walls and 1 glass wall (glass will be on the +X side)
    for (int y = floorY + 1; y <= floorY + wallHeight; ++y) {
        // North wall (z = roomMinZ) - WOOD
        for (int x = roomMinX; x <= roomMaxX; ++x) {
            //int index = x + y * WORLD_SIZE_X + roomMinZ * WORLD_SIZE_X * WORLD_SIZE_Y;
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_WOOD], make_color_rgba(140, 90, 50, 255), {x, y, roomMinZ});
            octree_insert(chunk0, voxel);
        }

        // South wall (z = roomMaxZ) - WOOD
        for (int x = roomMinX; x <= roomMaxX; ++x) {
            //int index = x + y * WORLD_SIZE_X + roomMaxZ * WORLD_SIZE_X * WORLD_SIZE_Y;
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_WOOD], make_color_rgba(140, 90, 50, 255), {x, y, roomMaxZ});
            octree_insert(chunk0, voxel);
        }

        // West wall (x = roomMinX) - WOOD
        for (int z = roomMinZ; z <= roomMaxZ; ++z) {
            //int index = roomMinX + y * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_WOOD], make_color_rgba(140, 90, 50, 255), {roomMinX, y, z});
            octree_insert(chunk0, voxel);
        }

        // East wall (x = roomMaxX) - GLASS
        for (int z = roomMinZ; z <= roomMaxZ; ++z) {
            //int index = roomMaxX + y * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
            Voxel_Object voxel = VoxelObjCreate(voxels[VOX_GLASS], make_color_rgba(100, 100, 230, 40), {roomMaxX, y, z});
            octree_insert(chunk0, voxel);
        }
    }

    // Create a red jelly sphere inside the room
    int cx = (roomMinX + roomMaxX) / 2;
    int cz = (roomMinZ + roomMaxZ) / 2;
    int cy = floorY + 6;
    int radius = 5;
    for (int x = cx - radius; x <= cx + radius; ++x) {
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int z = cz - radius; z <= cz + radius; ++z) {
                if (x < 0 || x >= WORLD_SIZE_X || y < 0 || y >= WORLD_SIZE_Y || z < 0 || z >= WORLD_SIZE_Z) continue;
                int dx = x - cx; int dy = y - cy; int dz = z - cz;
                if (dx*dx + dy*dy + dz*dz <= radius*radius) {
                    //int index = x + y * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
                    Voxel_Object voxel = VoxelObjCreate(voxels[VOX_JELLY], make_color_rgba(240, 100, 100, 100), {x, y, z});
                    octree_insert(chunk0, voxel);
                }
            }
        }
    }

    // Create Octree texture
    size_t arr_size;
    uint8_t* texture_data = octree_texture(chunk0, &arr_size);

    // O tex_dim que você calculou
    size_t tex_dim = (size_t)ceil(cbrt((double)_octree_texel_size(chunk0)));
    if (tex_dim == 0) tex_dim = 1;

    // --- Carga para a GPU ---
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID); // <-- Informa que é 3D

    // --- ADICIONE ESTA LINHA ---
    // Informa ao OpenGL que seus dados de pixel estão compactados.
    // (O alinhamento é de 1 byte, não o padrão de 4).
    // Isso impede que o OpenGL leia mal os seus dados.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // --- FIM DA CORREÇÃO ---

    // Agora chame o glTexImage3D (estou assumindo que você está
    // usando a versão síncrona, sem PBO)
    glTexImage3D(
        GL_TEXTURE_3D,       // Alvo
        0,                   // Nível de Mipmap
        GL_RGBA8,            // Formato interno na GPU
        (GLsizei)tex_dim,    // LARGURA
        (GLsizei)tex_dim,    // ALTURA
        (GLsizei)tex_dim,    // PROFUNDIDADE
        0,                   // Borda
        GL_RGBA,             // Formato dos seus dados (R,G,B,A)
        GL_UNSIGNED_BYTE,    // Tipo dos seus dados (uint8_t)
        NULL                 // O ponteiro para o seu array 1D
    );
    
    // (Opcional, mas boa prática) Redefina o alinhamento para o padrão
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // Use GL_NEAREST...
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Create PBO
    GLuint pboID;
    glGenBuffers(1, &pboID);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboID);

    // Allocate PBO and upload voxels
    // 1. Calcule o tamanho total do *container* da textura em texels
    size_t voxel_count_in_cube = tex_dim * tex_dim * tex_dim;
    
    // 2. Calcule o tamanho total do container em bytes
    // (Assumindo 4 bytes por texel, como em octree.cpp)
    size_t buffer_size_bytes = voxel_count_in_cube * 4;

    // 3. Aloque o PBO com o tamanho total do CONTAINER
    glBufferData(GL_PIXEL_UNPACK_BUFFER, buffer_size_bytes, NULL, GL_STREAM_DRAW);
    
    // 4. Mapeie o buffer (como antes)
    void* gpu_pointer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    
    // 5. Copie APENAS os dados válidos (arr_size) para o início do buffer
    memcpy(gpu_pointer, texture_data, arr_size);
    
    // 6. Desmapeie (como antes)
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    // Schedule the asynchronous transfer from PBO to texture
    // The 'NULL' here tells OpenGL to use the bound GL_PIXEL_UNPACK_BUFFER
    glTexSubImage3D(GL_TEXTURE_3D, 
                    0,          // Mipmap level
                    0,0,0,          // offset
                    (GLsizei)tex_dim, (GLsizei)tex_dim, (GLsizei)tex_dim,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    NULL);

    // Unbind the PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);



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
    GLint minBoundsLoc = glGetUniformLocation(computeProgram, "u_worldBoundsMin");
    GLint maxBoundsLoc = glGetUniformLocation(computeProgram, "u_worldBoundsMax");

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

    //std::cout << (int)get_green_rgba(octree_find(chunk0, {roomMinX, floorY, roomMinZ}).color) << std::endl;

    float now, lastTime = 0.0f;
    // Dentro do seu game loop
    while (!glfwWindowShouldClose(window)) {
        now = (float)glfwGetTime();
        std::cout << "FPS: " << 1.0f / (now - lastTime) << "\n";
        lastTime = now;
        
        // Lógica do deltaTime
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Processar input do teclado
        processInput(window);
        
        // Atualizar os dados da câmera
        CameraData cameraData;
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
        
        cameraData.invProjection = glm::inverse(projection);
        cameraData.invView = glm::inverse(view);
        cameraData.cameraPos = glm::vec4(camera.Position, 1.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &cameraData);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Executar o Compute Shader
        glUseProgram(computeProgram);
        
        // Vincular recursos
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, textureID);
        glUniform1f(texDimLoc, (GLfloat)tex_dim);
        glUniform3fv(minBoundsLoc, 1, (const GLfloat*)&min_bounds);
        glUniform3fv(maxBoundsLoc, 1, (const GLfloat*)&max_bounds);

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