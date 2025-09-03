#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <ShaderUtils.hpp>
#include <Camera.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

std::string computeSrc = ShaderUtils::readShaderFile("shaders/raytracing.comp");
std::string quadVsSrc  = ShaderUtils::readShaderFile("shaders/quad.vert");
std::string quadFsSrc  = ShaderUtils::readShaderFile("shaders/quad.frag");

const char* compute = computeSrc.c_str();
const char* quad_vs = quadVsSrc.c_str();
const char* quad_fs = quadFsSrc.c_str();

const int screenWidth = 1280;
const int screenHeight = 720;

Camera camera(glm::vec3(32.0f, 32.0f, 100.0f));

// Variáveis para o mouse
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

// Variáveis para o tempo (deltaTime)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Protótipos das novas funções
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

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
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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

// CÓDIGO NOVO: Callback para o movimento do mouse
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
    float yoffset = lastY - ypos; // invertido, pois as coordenadas y vão de cima para baixo

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

    // Para que o compute shader possa escrever nela, a vinculamos como uma "image unit".
    // O 'binding = 0' corresponde ao 'layout(binding = 0)' no shader.
    // GL_WRITE_ONLY indica que só vamos escrever.
    glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Suponha que você tenha um mundo de 64x64x64 voxels.
    const int WORLD_SIZE_X = 64;
    const int WORLD_SIZE_Y = 64;
    const int WORLD_SIZE_Z = 64;

    // Cada voxel pode ser um vec4 (R, G, B, tipo/ID). 0.0 para alfa significa ar.
    std::vector<glm::vec4> voxelData(WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z);

    // ... Preencha seu voxelData com os dados do mundo ...
    for (int y = 0; y < WORLD_SIZE_Y; ++y) {
        int index = WORLD_SIZE_X / 2 + y * WORLD_SIZE_X + WORLD_SIZE_X / 2 * WORLD_SIZE_X * WORLD_SIZE_Y;
        voxelData[index] = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Verde
    }

    // Por exemplo, para criar um chão vermelho:
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int z = 0; z < WORLD_SIZE_Z; ++z) {
            int index = x + 0 * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
            voxelData[index] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Vermelho
        }
    }

    GLuint ssbo_voxels;
    glGenBuffers(1, &ssbo_voxels);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_voxels);
    glBufferData(GL_SHADER_STORAGE_BUFFER, voxelData.size() * sizeof(glm::vec4), voxelData.data(), GL_STATIC_DRAW);

    // Vincula o buffer ao ponto de ligação 2 (corresponderá ao binding no shader)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_voxels);

    // UBO para a câmera
    struct CameraData {
        glm::mat4 invProjection; // Matriz de projeção inversa
        glm::mat4 invView;       // Matriz de visão inversa
        glm::vec3 cameraPos;
    };

    GLuint uboCamera; // Variável para guardar o ID do buffer

    // 1. Gerar o buffer
    glGenBuffers(1, &uboCamera);

    // 2. Vincular o buffer ao alvo GL_UNIFORM_BUFFER
    glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);

    // 3. Alocar a memória para o buffer.
    // Nós alocamos o espaço, mas não enviamos dados ainda (por isso o 'NULL').
    // GL_DYNAMIC_DRAW é uma dica para o OpenGL de que vamos atualizar estes dados frequentemente.
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_DYNAMIC_DRAW);

    // 4. Vincular o buffer ao ponto de ligação (binding point) 1.
    // Este '1' DEVE ser o mesmo que `binding = 1` no seu shader!
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboCamera);

    // 5. Desvincular o buffer do alvo principal para boa prática
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    GLuint quadVAO, quadVBO, quadEBO;

    // 1. Gerar o Vertex Array Object (VAO)
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO); // Ativar o VAO

    // 2. Gerar e configurar o Vertex Buffer Object (VBO) para as posições
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 3. Gerar e configurar o Element Buffer Object (EBO) para os índices
    glGenBuffers(1, &quadEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // 4. Configurar os atributos dos vértices (como interpretar os dados do VBO)
    // Posição do atributo 'layout (location = 0)' no vertex shader
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0); // Ativar o atributo

    // 5. Desvincular o VAO para não modificar acidentalmente
    glBindVertexArray(0);



    const GLuint compute_raytracing = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_raytracing, 1, &compute, NULL);
    glCompileShader(compute_raytracing);
    checkShaderCompilation(compute_raytracing);

    const GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, compute_raytracing);
    glLinkProgram(computeProgram);
    checkProgramLinking(computeProgram);


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

    // Dentro do seu game loop
    while (!glfwWindowShouldClose(window)) {
        // CÓDIGO NOVO: Lógica do deltaTime
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // CÓDIGO NOVO: Processar input do teclado
        processInput(window);
        
        // --- 1. ATUALIZAR DADOS (Câmera) ---
        CameraData cameraData;

        // LINHA MODIFICADA: A matriz view agora vem da nossa classe Camera
        glm::mat4 view = camera.GetViewMatrix();

        // LINHAS MODIFICADAS: Usar as constantes screenWidth e screenHeight
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
        
        cameraData.invProjection = glm::inverse(projection);
        cameraData.invView = glm::inverse(view);
        
        // LINHA MODIFICADA: A posição da câmera também vem da classe
        cameraData.cameraPos = glm::vec4(camera.Position, 1.0f);

        // Enviar os dados para a GPU
        glBindBuffer(GL_UNIFORM_BUFFER, uboCamera); // Vincular nosso UBO
        // Atualizar o conteúdo do buffer com os novos dados da struct
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &cameraData);
        glBindBuffer(GL_UNIFORM_BUFFER, 0); // Desvincular

        // 2. EXECUTAR O COMPUTE SHADER
        glUseProgram(computeProgram);
        
        // Vincular recursos
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_voxels);
        // ... vincular UBO da câmera ...

        // Dispara os threads do compute shader.
        // Dividimos o tamanho da tela pelo tamanho do grupo de trabalho definido no shader.
        // Se o tamanho do grupo for 8x8, por exemplo:
        glDispatchCompute(screenWidth / 8, screenHeight / 8, 1);

        // 3. BARREIRA DE MEMÓRIA
        // Essencial! Garante que o compute shader terminou de escrever na textura
        // antes que a etapa de rasterização tente ler dela.
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // 4. DESENHAR O QUAD NA TELA
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

        // Trocar buffers e processar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadEBO);
    glDeleteBuffers(1, &ssbo_voxels);
    glDeleteTextures(1, &outputTexture);
    glDeleteShader(compute_raytracing);
    glDeleteProgram(computeProgram);
    glDeleteShader(quad_vertex_shader);
    glDeleteShader(quad_fragment_shader);
    glDeleteProgram(quadProgram);
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}