#ifndef _MAT4_H
#define _MAT4_H

#include "vec3.h"
#include "vec4.h"

typedef struct _mat4 {
    float m[16];
} Mat4;

Mat4 mat4(float val);

Mat4 mat4_scalar(Mat4 in, float scalar);

Mat4 mat4_add(Mat4 a, Mat4 b);
Mat4 mat4_sub(Mat4 a, Mat4 b);

Mat4 mat4_mul(Mat4 a, Mat4 b);
Vector4 mat4_vec4(Mat4 a, Vector4 b);

Mat4 mat4_scale(float s1, float s2, float s3);
Mat4 mat4_translation(float t1, float t2, float t3);
Mat4 mat4_rotation(float r1, float r2, float r3);

Mat4 mat4_look_at(Vector3 eye, Vector3 center, Vector3 up);
Mat4 mat4_make_model(Vector3 position, Vector3 rotation, Vector3 scale);

#endif
