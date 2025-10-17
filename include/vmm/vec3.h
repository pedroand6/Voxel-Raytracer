#ifndef _VEC3_h
#define _VEC3_h

#include <stdbool.h>

typedef union _vec3 {
    struct {
        float x;
        float y;
        float z;
    };
    struct {
        float r;
        float g;
        float b;
    };
} Vector3;

#include "vec4.h"
#include "ivec3.h"

Vector3 vec3_float(float x, float y, float z);
Vector3 vec3_vec3(Vector3 vec);
Vector3 vec3_vec4(Vector4 vec);
Vector3 vec3_ivec3(IVector3 vec);
Vector3 vec3_zero(void);
Vector3 vec3_one(void);
Vector3 vec3_left(void);
Vector3 vec3_right(void);
Vector3 vec3_back(void);
Vector3 vec3_forward(void);
Vector3 vec3_down(void);
Vector3 vec3_up(void);

bool vec3_equal_vec(Vector3 a, Vector3 b);
Vector3 vec3_min(Vector3 a, Vector3 b);
Vector3 vec3_max(Vector3 a, Vector3 b);
Vector3 vec3_abs(Vector3 a);
Vector3 vec3_sign(Vector3 a);

Vector3 vec3_scalar_add(Vector3 in, float scalar);
Vector3 vec3_scalar_sub(Vector3 in, float scalar);
Vector3 vec3_scalar_mul(Vector3 in, float scalar);
Vector3 vec3_scalar_div(Vector3 in, float scalar);

Vector3 vec3_negate(Vector3 in);
Vector3 vec3_add(Vector3 a, Vector3 b);
Vector3 vec3_sub(Vector3 a, Vector3 b);
Vector3 vec3_mul(Vector3 a, Vector3 b);
Vector3 vec3_div(Vector3 a, Vector3 b);

float vec3_dot(Vector3 a, Vector3 b);
float vec3_angle(Vector3 a, Vector3 b);
Vector3 vec3_cross(Vector3 a, Vector3 b);
float vec3_len(Vector3 in);

Vector3 vec3_normalize(Vector3 in);

#endif
