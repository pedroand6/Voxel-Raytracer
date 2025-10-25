#ifndef _IVEC3_H
#define _IVEC3_H

#include <stdint.h>
#include <stdbool.h>

typedef union _ivec3 {
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    };
    struct {
        int32_t r;
        int32_t g;
        int32_t b;
    };
} IVector3;

#include "vec3.h"

IVector3 ivec3_int(int32_t x, int32_t y, int32_t z);
IVector3 ivec3_ivec3(IVector3 vec);
IVector3 ivec3_vec3(Vector3 vec);
IVector3 ivec3_zero(void);
IVector3 ivec3_one(void);
IVector3 ivec3_left(void);
IVector3 ivec3_right(void);
IVector3 ivec3_back(void);
IVector3 ivec3_forward(void);
IVector3 ivec3_down(void);
IVector3 ivec3_up(void);

bool ivec3_equal_vec(IVector3 a, IVector3 b);
IVector3 ivec3_min(IVector3 a, IVector3 b);
IVector3 ivec3_max(IVector3 a, IVector3 b);
IVector3 ivec3_abs(IVector3 a);
IVector3 ivec3_sign(IVector3 a);

IVector3 ivec3_scalar_add(IVector3 in, int scalar);
IVector3 ivec3_scalar_sub(IVector3 in, int scalar);
IVector3 ivec3_scalar_mul(IVector3 in, int scalar);
IVector3 ivec3_scalar_div(IVector3 in, int scalar);

IVector3 ivec3_negate(IVector3 in);
IVector3 ivec3_add(IVector3 a, IVector3 b);
IVector3 ivec3_sub(IVector3 a, IVector3 b);
IVector3 ivec3_mul(IVector3 a, IVector3 b);
IVector3 ivec3_div(IVector3 a, IVector3 b);

float ivec3_dot(IVector3 a, IVector3 b);
float ivec3_angle(IVector3 a, IVector3 b);
IVector3 ivec3_cross(IVector3 a, IVector3 b);
float ivec3_len(IVector3 in);

IVector3 ivec3_normalize(IVector3 in);

#endif
