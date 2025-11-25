#version 450 core

// Saída do fragment shader: a cor final do píxel
out vec4 FragColor;

// Entrada vinda do vertex shader: as coordenadas da textura
in vec2 texCoords;

// A textura gerada pelo compute shader (o resultado ruidoso)
layout(binding = 0) uniform sampler2D screenTexture;

// Textura de IDs + Distância
// Canal R: ID do Voxel (com Face ID embutido)
// Canal G: Distância do Voxel em unidades inteiras
layout(binding = 1) uniform isampler2D voxelIdTexture;

// Configurações de Qualidade
const int MAX_RADIUS = 20; // Limite máximo para não matar a performance em close-ups
const int MIN_RADIUS = 1;  // Mínimo para voxels distantes
const float BLUR_FACTOR = 200.0; // Quanto maior, mais forte o blur para objetos próximos

void main()
{
    // Obtém o tamanho da textura para converter coordenadas UV (0.0-1.0) em Pixels Inteiros
    ivec2 texSize = textureSize(screenTexture, 0);
    ivec2 pixelPos = ivec2(texCoords * texSize);

    // 1. Lê os dados do pixel central
    // Canal R = ID, Canal G = Distância
    ivec4 centerData = texelFetch(voxelIdTexture, pixelPos, 0);
    int centerID = centerData.r;
    int centerDist = centerData.g;

    // Se o ID for negativo (fundo/céu), não faz média, apenas repassa a cor.
    if (centerID < 0) {
        FragColor = texelFetch(screenTexture, pixelPos, 0);
        return;
    }

    // 2. Calcula o Raio Dinâmico baseado na distância
    // Voxels perto (dist pequena) -> Raio grande -> Média de mais pixels -> Menos ruído
    // Voxels longe (dist grande) -> Raio pequeno -> Preserva detalhes
    // Usamos max(1, ...) para evitar divisão por zero
    float calculatedRadius = BLUR_FACTOR / sqrt(float(max(1, centerDist)));
    
    // Grampeia o raio para segurança (1 até MAX_RADIUS)
    int RADIUS = clamp(int(calculatedRadius), MIN_RADIUS, MAX_RADIUS);

    vec3 colorSum = vec3(0.0);
    float count = 0.0;

    // 3. Loop pelos vizinhos (Kernel de Caixa)
    for (int y = -RADIUS; y <= RADIUS; ++y) {
        for (int x = -RADIUS; x <= RADIUS; ++x) {
            
            ivec2 neighborPos = pixelPos + ivec2(x, y);

            // Verificação de borda da tela
            if (neighborPos.x < 0 || neighborPos.x >= texSize.x || 
                neighborPos.y < 0 || neighborPos.y >= texSize.y) {
                continue;
            }

            // 4. A Lógica Smart: Só misturamos se for o MESMO voxel (Mesmo ID)
            // Isso preserva as arestas vivas do cubo.
            int neighborID = texelFetch(voxelIdTexture, neighborPos, 0).r;

            if (neighborID == centerID) {
                vec3 neighborColor = texelFetch(screenTexture, neighborPos, 0).rgb;
                colorSum += neighborColor;
                count += 1.0;
            }
        }
    }

    // 5. Média Final
    vec3 finalColor = colorSum / max(count, 1.0);

    // DICA DE DEBUG: Se quiser ver o raio variando, descomente a linha abaixo:
    // FragColor = vec4(vec3(float(RADIUS) / float(MAX_RADIUS)), 1.0); return;

    FragColor = vec4(finalColor, 1.0);
}