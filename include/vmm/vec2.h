#ifndef _VEC2_H
#define _VEC2_H

#include <stdbool.h>

typedef struct _vec2 {
    float x;
    float y;
} Vector2;

bool vec2_equal_vec(Vector2 a, Vector2 b);

Vector2 vec2_scalar_add(Vector2 in, float scalar);
Vector2 vec2_scalar_sub(Vector2 in, float scalar);
Vector2 vec2_scalar_mul(Vector2 in, float scalar);
Vector2 vec2_scalar_div(Vector2 in, float scalar);

Vector2 vec2_negate(Vector2 in);
Vector2 vec2_add(Vector2 a, Vector2 b);
Vector2 vec2_sub(Vector2 a, Vector2 b);

float vec2_dot(Vector2 a, Vector2 b);
float vec2_angle(Vector2 a, Vector2 b);
float vec2_len(Vector2 in);

Vector2 vec2_normalize(Vector2 in);

#endif
