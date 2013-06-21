/*

	d2dbasetypes.h - Header file for the Direct2D API

	No original Microsoft headers were used in the creation of this
	file.

*/

#ifndef _D2DBASETYPES_H
#define _D2DBASETYPES_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <d3d9types.h>

typedef D3DCOLORVALUE D2D_COLOR_F;

struct D2D_MATRIX_3X2_F {
  FLOAT _11;
  FLOAT _12;
  FLOAT _21;
  FLOAT _22;
  FLOAT _31;
  FLOAT _32;
};

struct D2D_POINT_2F {
  FLOAT x;
  FLOAT y;
};

struct D2D_POINT_2U {
  UINT32 x;
  UINT32 y;
};

struct D2D_RECT_F {
  FLOAT left;
  FLOAT top;
  FLOAT right;
  FLOAT bottom;
};

struct D2D_RECT_U {
  UINT32 left;
  UINT32 top;
  UINT32 right;
  UINT32 bottom;
};

struct D2D_SIZE_F {
  FLOAT width;
  FLOAT height;
};

struct D2D_SIZE_U {
  UINT32 width;
  UINT32 height;
};

typedef D2D_COLOR_F D2D1_COLOR_F;

typedef struct D2D_POINT_2F D2D1_POINT_2F;

typedef struct D2D_POINT_2U D2D1_POINT_2U;

typedef struct D2D_RECT_F D2D1_RECT_F;

typedef struct D2D_RECT_U D2D1_RECT_U;

typedef struct D2D_SIZE_F D2D1_SIZE_F;

typedef struct D2D_SIZE_U D2D1_SIZE_U;

#endif /* _D2DBASETYPES_H */
