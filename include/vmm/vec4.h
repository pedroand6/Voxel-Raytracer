#ifndef _VEC4_H
#define _VEC4_H

#include <stdbool.h>

typedef struct _vec4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

bool vec4_equal_vec(Vector4 a, Vector4 b);

Vector4 vec4_scalar_add(Vector4 in, float scalar);
Vector4 vec4_scalar_sub(Vector4 in, float scalar);
Vector4 vec4_scalar_mul(Vector4 in, float scalar);
Vector4 vec4_scalar_div(Vector4 in, float scalar);

Vector4 vec4_negate(Vector4 in);
Vector4 vec4_add(Vector4 a, Vector4 b);
Vector4 vec4_sub(Vector4 a, Vector4 b);

float vec4_dot(Vector4 a, Vector4 b);
float vec4_angle(Vector4 a, Vector4 b);
float vec4_len(Vector4 in);

Vector4 vec4_normalize(Vector4 in);

#endif
