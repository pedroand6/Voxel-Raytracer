#ifndef _ivec3_h
#define _ivec3_h

#include <stdint.h>
#include <stdbool.h>

typedef struct _ivec3 {
    int32_t x;
    int32_t y;
    int32_t z;
} IVector3;

bool ivec3_equal_vec(IVector3 a, IVector3 b);

IVector3 ivec3_scalar_add(IVector3 in, int scalar);
IVector3 ivec3_scalar_sub(IVector3 in, int scalar);
IVector3 ivec3_scalar_mul(IVector3 in, int scalar);
IVector3 ivec3_scalar_div(IVector3 in, int scalar);

IVector3 ivec3_negate(IVector3 in);
IVector3 ivec3_add(IVector3 a, IVector3 b);
IVector3 ivec3_sub(IVector3 a, IVector3 b);

float ivec3_dot(IVector3 a, IVector3 b);
float ivec3_angle(IVector3 a, IVector3 b);
IVector3 ivec3_cross(IVector3 a, IVector3 b);
float ivec3_len(IVector3 in);

IVector3 ivec3_normalize(IVector3 in);

#endif
