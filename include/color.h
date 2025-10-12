#ifndef _COLOR_H
#define _COLOR_H

#include <stdint.h>
#include <vmm/vec3.h>
#include <vmm/vec4.h>

//interpreting color as four byte RGBA value;
typedef uint32_t ColorRGBA;
typedef uint32_t ColorRGB;

#define COLOR_CLEAR 0x00000000
#define COLOR_WHITEA 0xffffffff
#define COLOR_BLACKA 0x000000ff
#define COLOR_DGRAYA 0x333333ff
#define COLOR_GRAYA 0x999999ff
#define COLOR_LGRAYA 0xccccccff
#define COLOR_REDA 0xff0000ff
#define COLOR_GREENA 0x00ff00ff
#define COLOR_BLUEA 0x0000ffff
#define COLOR_PURPLEA 0xff00ffff

#define COLOR_WHITE 0xffffff
#define COLOR_BLACK 0x000000
#define COLOR_DGRAY 0x333333
#define COLOR_GRAY 0x999999
#define COLOR_LGRAY 0xcccccc
#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define COLOR_BLUE 0x0000ff
#define COLOR_PURPLE 0xff00ff

ColorRGB make_color_rgb(uint8_t red, uint8_t green, uint8_t blue);
ColorRGBA make_color_rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
ColorRGB get_color_rgba(ColorRGBA color);
ColorRGBA get_color_rgb(ColorRGB color);
uint8_t get_red_rgb(ColorRGB color);
uint8_t get_red_rgba(ColorRGBA color);
uint8_t get_green_rgb(ColorRGB color);
uint8_t get_green_rgba(ColorRGBA color);
uint8_t get_blue_rgb(ColorRGB color);
uint8_t get_blue_rgba(ColorRGBA color);
uint8_t get_alpha_rgba(ColorRGBA color);

Vector3 get_rgb_vec3(ColorRGB color);
Vector4 get_rgba_vec4(ColorRGBA color);

#endif