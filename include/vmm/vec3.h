#ifndef _VEC3_h
#define _VEC3_h

#include <stdbool.h>

typedef struct _vec3 {
    float x;
    float y;
    float z;
} Vector3;

bool vec3_equal_vec(Vector3 a, Vector3 b);

Vector3 vec3_scalar_add(Vector3 in, float scalar);
Vector3 vec3_scalar_sub(Vector3 in, float scalar);
Vector3 vec3_scalar_mul(Vector3 in, float scalar);
Vector3 vec3_scalar_div(Vector3 in, float scalar);

Vector3 vec3_negate(Vector3 in);
Vector3 vec3_add(Vector3 a, Vector3 b);
Vector3 vec3_sub(Vector3 a, Vector3 b);

float vec3_dot(Vector3 a, Vector3 b);
float vec3_angle(Vector3 a, Vector3 b);
Vector3 vec3_cross(Vector3 a, Vector3 b);
float vec3_len(Vector3 in);

Vector3 vec3_normalize(Vector3 in);

#endif
