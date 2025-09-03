#version 450 core
// Saída do fragment shader: a cor final do píxel
out vec4 FragColor;

// Entrada vinda do vertex shader: as coordenadas da textura
in vec2 texCoords;

// A textura que foi gerada pelo compute shader.
// Esta variável precisa de ser ligada ao 'texture unit' 0 no código C++.
uniform sampler2D screenTexture;

void main()
{
    // A cor deste fragmento é simplesmente a cor lida da textura
    // na coordenada de textura correspondente.
    FragColor = texture(screenTexture, texCoords);
}