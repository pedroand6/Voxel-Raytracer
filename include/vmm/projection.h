#ifndef _PROJ_H
#define _PROJ_H

#include "mat4.h"

Mat4 mat4_orthogonal(float left, float right, float bottom, float top, float zNear, float zFar);
Mat4 mat4_perspective(float fov, float aspect, float zNear, float zFar);

#endif
