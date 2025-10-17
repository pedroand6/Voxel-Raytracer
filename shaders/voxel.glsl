#version 450 core

// Conversão das cores para o sistema de vector4
uint get_red_rgba(uint color) {
    uint red = (color >> 24) & 0xff;
    return red;
}

uint get_green_rgba(uint color) {
    uint green = (color >> 16) & 0xff;
    return green;
}

uint get_blue_rgba(uint color) {
    uint blue = (color >> 8) & 0xff;
    return blue;
}

uint get_alpha_rgba(uint color) {
    uint alpha = color & 0xff;
    return alpha;
}

vec4 rgbaColor(uint color) {
    return vec4(
        float(get_red_rgba(color))/ float(0xff),
        float(get_green_rgba(color))/ float(0xff),
        float(get_blue_rgba(color))/ float(0xff),
        float(get_alpha_rgba(color))/ float(0xff)
    );
}


// Enum for Voxel Types
const uint VOX_AIR = 0;
const uint VOX_GRASS = 1;
const uint VOX_DIRT = 2;
const uint VOX_STONE = 3;
const uint VOX_WATER = 4;

struct Voxel {
    uint type;
    float refraction;
};

struct VoxelObj {
    ivec3 coord;
    uint color; // uint32
    Voxel voxel;

    uint padding[2];
};