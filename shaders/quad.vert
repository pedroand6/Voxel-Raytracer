#version 450 core

// Atributo do vértice: a posição 2D do vértice do quadrado
// (corresponde ao 'glVertexAttribPointer' com location = 0)
layout (location = 0) in vec2 aPos;

// Saída para o fragment shader: as coordenadas da textura
out vec2 texCoords;

void main()
{
    // A posição final do vértice é a sua posição original.
    // Não precisamos de matrizes de câmera/projeção, pois o quadrado
    // já está no espaço de tela (-1 a 1).
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);

    // Converte a posição do vértice (-1 a 1) para coordenadas de textura (0 a 1)
    // e passa para o fragment shader.
    texCoords = (aPos + 1.0) / 2.0;
}