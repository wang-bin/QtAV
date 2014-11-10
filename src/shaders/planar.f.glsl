/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#else
#define highp
#define mediump
#define lowp
#endif

// u_TextureN: yuv. use array?
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
#ifdef PLANE_4
uniform sampler2D u_Texture3;
#endif //PLANE_4
varying lowp vec2 v_TexCoords;
uniform float u_opacity;
uniform float u_bpp;
uniform mat4 u_colorMatrix;

#if defined(LA_16BITS_BE) || defined(LA_16BITS_LE)
#define LA_16BITS 1
#else
#define LA_16BITS 0
#endif
//#define LA_16BITS  (defined(LA_16BITS_BE) || defined(LA_16BITS_LE)) // why may error?

#if defined(YUV_MAT_GLSL)
//http://en.wikipedia.org/wiki/YUV calculation used
//http://www.fourcc.org/fccyvrgb.php
//GLSL: col first
// use bt601
#if defined(CS_BT709)
const mat4 yuv2rgbMatrix = mat4(1, 1, 1, 0,
                              0, -0.187, 1.8556, 0,
                              1.5701, -0.4664, 0, 0,
                              0, 0, 0, 1)
#else //BT601
const mat4 yuv2rgbMatrix = mat4(1, 1, 1, 0,
                              0, -0.344, 1.773, 0,
                              1.403, -0.714, 0, 0,
                              0, 0, 0, 1)
#endif
                        * mat4(1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, -0.5, -0.5, 1);
#endif

// 10, 16bit: http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578%28v=vs.85%29.aspx
void main()
{
    // FFmpeg supports 9, 10, 12, 14, 16 bits
#if LA_16BITS
    //http://stackoverflow.com/questions/22693169/opengl-es-2-0-glsl-compiling-fails-on-osx-when-using-const
    float range = exp2(u_bpp) - 1.0; // why can not be const?
#if defined(LA_16BITS_LE)
    vec2 t = vec2(1.0, 256.0)*255.0/range;
#else
    vec2 t = vec2(256.0, 1.0)*255.0/range;
#endif
#endif //LA_16BITS
    // 10p in little endian: yyyyyyyy yy000000 => (L, L, L, A)
    gl_FragColor = clamp(u_colorMatrix
                         * vec4(
#if LA_16BITS
                             dot(texture2D(u_Texture0, v_TexCoords).ra, t),
                             dot(texture2D(u_Texture1, v_TexCoords).ra, t),
                             dot(texture2D(u_Texture2, v_TexCoords).ra, t),
#else
// use r, g, a to work for both yv12 and nv12. idea from xbmc
                             texture2D(u_Texture0, v_TexCoords).r,
                             texture2D(u_Texture1, v_TexCoords).g,
                             texture2D(u_Texture2, v_TexCoords).a,
#endif //LA_16BITS
                             1)
                         , 0.0, 1.0) * u_opacity;
#ifdef PLANE_4
#if LA_16BITS
    gl_FragColor.a *= dot(texture2D(u_Texture3, v_TexCoords).ra, t); //GL_LUMINANCE_ALPHA
#else //8bit
    gl_FragColor.a *= texture2D(u_Texture3, v_TexCoords).a; //GL_ALPHA
#endif //LA_16BITS
#endif //PLANE_4
}
