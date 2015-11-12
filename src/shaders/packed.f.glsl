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
// >=1.40: texture(sampler2DRect,...). 'texture' is define in header
#if __VERSION__ < 130
#ifndef texture
#define texture texture2D
#endif
#endif

uniform sampler2D u_Texture0;
varying vec2 v_TexCoords0;
uniform mat4 u_colorMatrix;
uniform float u_opacity;
uniform mat4 u_c;

/***User Sampler code here***%1***/
#ifndef USER_SAMPLER
vec4 sample(sampler2D tex, vec2 pos)
{
    return texture(tex, pos);
}
#endif

void main() {
    vec4 c = sample(u_Texture0, v_TexCoords0);
    c = u_c * c;
#ifndef HAS_ALPHA
    c.a = 1.0;
#endif //HAS_ALPHA
    gl_FragColor = clamp(u_colorMatrix * c, 0.0, 1.0) * u_opacity;
}
