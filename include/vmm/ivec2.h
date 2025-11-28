#ifndef _IVEC2_H
#define _IVEC2_H

#include <stdint.h>
#include <stdbool.h>

typedef union _ivec2 {
    struct {
        int32_t x;
        int32_t y;
    };
    struct {
        int32_t u;
        int32_t v;
    };
} IVector2;

#include "vec2.h"

IVector2 ivec2_int(int32_t x, int32_t y);
IVector2 ivec2_ivec2(IVector2 vec);
IVector2 ivec2_vec2(Vector2 vec);
IVector2 ivec2_zero(void);
IVector2 ivec2_one(void);
IVector2 ivec2_left(void);
IVector2 ivec2_right(void);
IVector2 ivec2_down(void);
IVector2 ivec2_up(void);

bool ivec2_equal_vec(IVector2 a, IVector2 b);
IVector2 ivec2_min(IVector2 a, IVector2 b);
IVector2 ivec2_max(IVector2 a, IVector2 b);
IVector2 ivec2_abs(IVector2 a);
IVector2 ivec2_sign(IVector2 a);

IVector2 ivec2_scalar_add(IVector2 in, int scalar);
IVector2 ivec2_scalar_sub(IVector2 in, int scalar);
IVector2 ivec2_scalar_mul(IVector2 in, int scalar);
IVector2 ivec2_scalar_div(IVector2 in, int scalar);

IVector2 ivec2_negate(IVector2 in);
IVector2 ivec2_add(IVector2 a, IVector2 b);
IVector2 ivec2_sub(IVector2 a, IVector2 b);
IVector2 ivec2_mul(IVector2 a, IVector2 b);
IVector2 ivec2_div(IVector2 a, IVector2 b);

float ivec2_dot(IVector2 a, IVector2 b);
float ivec2_angle(IVector2 a, IVector2 b);
IVector2 ivec2_cross(IVector2 a, IVector2 b);
float ivec2_len(IVector2 in);

IVector2 ivec2_normalize(IVector2 in);

#endif
