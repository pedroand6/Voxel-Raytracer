#include <stdint.h>
#include <color.h>

ColorRGB make_color_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    ColorRGB ret = (red << 16) | (green << 8) | blue;
    return ret;
}

ColorRGBA make_color_rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    ColorRGBA ret = (red << 24) | (green << 16) | (blue << 8) | alpha;
    return ret;
}

ColorRGB get_color_rgba(ColorRGBA color) {
    ColorRGB col = color >> 8;
    return col;
}

ColorRGBA get_color_rgb(ColorRGB color) {
    ColorRGBA col = (color << 8) | 0xff;
    return col;
}

uint8_t get_red_rgb(ColorRGB color) {
    uint8_t red = (color >> 16) & 0xff;
    return red;
}

uint8_t get_red_rgba(ColorRGBA color) {
    uint8_t red = (color >> 24) & 0xff;
    return red;
}

uint8_t get_green_rgb(ColorRGB color) {
    uint8_t green = (color >> 8) & 0xff;
    return green;
}

uint8_t get_green_rgba(ColorRGBA color) {
    uint8_t green = (color >> 16) & 0xff;
    return green;
}

uint8_t get_blue_rgb(ColorRGB color) {
    uint8_t blue = color & 0xff;
    return blue;
}

uint8_t get_blue_rgba(ColorRGBA color) {
    uint8_t blue = (color >> 8) & 0xff;
    return blue;
}

uint8_t get_alpha_rgba(ColorRGBA color) {
    uint8_t alpha = color & 0xff;
    return alpha;
}

Vector3 get_rgb_vec3(ColorRGB color) {
    Vector3 vec = {
        (float)get_red_rgb(color)/ (float)0xff,
        (float)get_green_rgb(color)/ (float)0xff,
        (float)get_blue_rgb(color)/ (float)0xff
    };
    return vec;
}

Vector4 get_rgba_vec4(ColorRGBA color) {
    Vector4 vec = {
        (float)get_red_rgba(color)/ (float)0xff,
        (float)get_green_rgba(color)/ (float)0xff,
        (float)get_blue_rgba(color)/ (float)0xff,
        (float)get_alpha_rgba(color)/ (float)0xff
    };
    return vec;
}
