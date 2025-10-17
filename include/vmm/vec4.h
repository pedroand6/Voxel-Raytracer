#ifndef _VEC4_H
#define _VEC4_H

#include <stdbool.h>

typedef union _vec4 {
    struct {
        float x;
        float y;
        float z;
        float w;
    };
    struct {
        float r;
        float g;
        float b;
        float a;
    };
} Vector4;

Vector4 vec4_float(float x, float y, float z, float w);
Vector4 vec4_vec4(Vector4 vec);
Vector4 vec4_zero(void);
Vector4 vec4_one(void);

bool vec4_equal_vec(Vector4 a, Vector4 b);
Vector4 vec4_max(Vector4 a, Vector4 b);
Vector4 vec4_abs(Vector4 a);
Vector4 vec4_sign(Vector4 a);

Vector4 vec4_scalar_add(Vector4 in, float scalar);
Vector4 vec4_scalar_sub(Vector4 in, float scalar);
Vector4 vec4_scalar_mul(Vector4 in, float scalar);
Vector4 vec4_scalar_div(Vector4 in, float scalar);

Vector4 vec4_negate(Vector4 in);
Vector4 vec4_add(Vector4 a, Vector4 b);
Vector4 vec4_sub(Vector4 a, Vector4 b);
Vector4 vec4_mul(Vector4 a, Vector4 b);
Vector4 vec4_div(Vector4 a, Vector4 b);

float vec4_dot(Vector4 a, Vector4 b);
float vec4_angle(Vector4 a, Vector4 b);
float vec4_len(Vector4 in);

Vector4 vec4_normalize(Vector4 in);

#endif
