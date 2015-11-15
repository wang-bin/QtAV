/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
// >=1.40: texture(sampler2DRect,...). 'texture' is define in header
#if __VERSION__ < 130
#ifndef texture
#define texture texture2D
#endif
#endif

// u_TextureN: yuv. use array?
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
#ifdef HAS_ALPHA
uniform sampler2D u_Texture3;
#endif //HAS_ALPHA
varying lowp vec2 v_TexCoords0;
#ifdef MULTI_COORD
varying lowp vec2 v_TexCoords1;
varying lowp vec2 v_TexCoords2;
#ifdef HAS_ALPHA
varying lowp vec2 v_TexCoords3;
#endif
#else
#define v_TexCoords1 v_TexCoords0
#define v_TexCoords2 v_TexCoords0
#define v_TexCoords3 v_TexCoords0
#endif //MULTI_COORD
uniform float u_opacity;
uniform float u_bpp;
uniform mat4 u_colorMatrix;
#ifdef CHANNEL16_TO8
uniform vec2 u_to8;
#endif
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
// matrixCompMult for convolution
/***User Sampler code here***%1***/
#ifndef USER_SAMPLER
vec4 sample(sampler2D tex, vec2 pos)
{
    return texture(tex, pos);
}
#endif

// 10, 16bit: http://msdn.microsoft.com/en-us/library/windows/desktop/bb970578%28v=vs.85%29.aspx
void main()
{
    gl_FragColor = clamp(u_colorMatrix
                         * vec4(
#ifdef CHANNEL16_TO8
#ifdef USE_RG
                             dot(sample(u_Texture0, v_TexCoords0).rg, u_to8),
                             dot(sample(u_Texture1, v_TexCoords1).rg, u_to8),
                             dot(sample(u_Texture2, v_TexCoords2).rg, u_to8),
#else
                             dot(sample(u_Texture0, v_TexCoords0).ra, u_to8),
                             dot(sample(u_Texture1, v_TexCoords1).ra, u_to8),
                             dot(sample(u_Texture2, v_TexCoords2).ra, u_to8),
#endif //USE_RG
#else
#ifdef USE_RG
                             sample(u_Texture0, v_TexCoords0).r,
                             sample(u_Texture1, v_TexCoords1).r,
#ifdef IS_BIPLANE
                             sample(u_Texture2, v_TexCoords2).g,
#else
                             sample(u_Texture2, v_TexCoords2).r,
#endif //IS_BIPLANE
#else
// use r, g, a to work for both yv12 and nv12. idea from xbmc
                             sample(u_Texture0, v_TexCoords0).r,
                             sample(u_Texture1, v_TexCoords1).g,
                             sample(u_Texture2, v_TexCoords2).a,
#endif //USE_RG
#endif //CHANNEL16_TO8
                             1)
                         , 0.0, 1.0) * u_opacity;
#ifdef HAS_ALPHA
#ifdef CHANNEL16_TO8
#ifdef USE_RG
    gl_FragColor.a *= dot(sample(u_Texture3, v_TexCoords3).rg, u_to8); //GL_RG
#else
    gl_FragColor.a *= dot(sample(u_Texture3, v_TexCoords3).ra, u_to8); //GL_LUMINANCE_ALPHA
#endif //USE_RG
#else //8bit
#ifdef USE_RG
    gl_FragColor.a *= sample(u_Texture3, v_TexCoords3).r;
#else
    gl_FragColor.a *= sample(u_Texture3, v_TexCoords3).a; //GL_ALPHA
#endif //USE_RG
#endif //CHANNEL16_TO8
#endif //HAS_ALPHA
}
