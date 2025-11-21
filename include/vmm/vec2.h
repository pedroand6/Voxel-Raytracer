#ifndef _VEC2_H
#define _VEC2_H

#include <stdbool.h>

typedef union _vec2 {
    struct {
        float x;
        float y;
    };
    struct {
        float u;
        float v;
    };
    struct {
        float a;
        float b;
    };
} Vector2;

#include "ivec2.h"

Vector2 vec2_float(float x, float y);
Vector2 vec2_vec2(Vector2 vec);
Vector2 vec2_ivec2(IVector2 vec);
Vector2 vec2_zero(void);
Vector2 vec2_one(void);
Vector2 vec2_left(void);
Vector2 vec2_right(void);
Vector2 vec2_down(void);
Vector2 vec2_up(void);

bool vec2_equal_vec(Vector2 a, Vector2 b);
Vector2 vec2_min(Vector2 a, Vector2 b);
Vector2 vec2_max(Vector2 a, Vector2 b);
Vector2 vec2_abs(Vector2 a);
Vector2 vec2_sign(Vector2 a);

Vector2 vec2_scalar_add(Vector2 in, float scalar);
Vector2 vec2_scalar_sub(Vector2 in, float scalar);
Vector2 vec2_scalar_mul(Vector2 in, float scalar);
Vector2 vec2_scalar_div(Vector2 in, float scalar);

Vector2 vec2_negate(Vector2 in);
Vector2 vec2_add(Vector2 a, Vector2 b);
Vector2 vec2_sub(Vector2 a, Vector2 b);
Vector2 vec2_mul(Vector2 a, Vector2 b);
Vector2 vec2_div(Vector2 a, Vector2 b);

float vec2_dot(Vector2 a, Vector2 b);
float vec2_angle(Vector2 a, Vector2 b);
float vec2_len(Vector2 in);

Vector2 vec2_normalize(Vector2 in);

#endif
