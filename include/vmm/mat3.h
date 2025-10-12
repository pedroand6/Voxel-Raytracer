#ifndef _MAT3_H
#define _MAT3_H

#include "vec3.h"

typedef struct _mat3 {
    float m[9];
} Mat3;

Mat3 mat3(float val);

Mat3 mat3_scalar(Mat3 in, float scalar);

Mat3 mat3_add(Mat3 a, Mat3 b);
Mat3 mat3_sub(Mat3 a, Mat3 b);

Mat3 mat3_mul(Mat3 a, Mat3 b);
Vector3 mat3_vec3(Mat3 a, Vector3 b);

Mat3 mat3_scale(float s1, float s2);
Mat3 mat3_translation(float t1, float t2);
Mat3 mat3_rotation(float r1);

#endif
