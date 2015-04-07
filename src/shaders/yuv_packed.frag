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

uniform sampler2D u_Texture0;
varying lowp vec2 v_TexCoords;
uniform float u_opacity;
uniform mat4 u_colorMatrix;
uniform vec4 u_c0; // yuyv: (0.5, 0, 0.5, 0)
uniform vec4 u_c1; // yuyv: (0, 1, 0, 0)
uniform vec4 u_c2; // yuyv: (0, 0, 0, 1)

void main(void)
{
    gl_FragColor = clamp(u_colorMatrix
                         * vec4(
                             dot(texture2D(u_Texture0, v_TexCoords), u_c0),
                             dot(texture2D(u_Texture0, v_TexCoords), u_c1),
                             dot(texture2D(u_Texture0, v_TexCoords), u_c2),
                             1)
                         , 0.0, 1.0) * u_opacity;
}

