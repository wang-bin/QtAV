/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

uniform sampler2D u_Texture0;
varying vec2 v_TexCoords0;
uniform mat4 u_colorMatrix;
uniform float u_opacity;
uniform mat4 u_c;
/***User header code***%userHeader%***/
/***User sampling function here***%userSample%***/
#ifndef USER_SAMPLER
vec4 sample2d(sampler2D tex, vec2 pos, int plane)
{
    return texture(tex, pos);
}
#endif

void main() {
    vec4 c = sample2d(u_Texture0, v_TexCoords0, 0);
    c = u_c * c;
#ifndef HAS_ALPHA
    c.a = 1.0; // before color mat transform!
#endif //HAS_ALPHA
#ifdef XYZ_GAMMA
    c.rgb = pow(c.rgb, vec3(2.6));
#endif // XYZ_GAMMA
    c = u_colorMatrix * c;
#ifdef XYZ_GAMMA
    c.rgb = pow(c.rgb, vec3(1.0/2.2));
#endif //XYZ_GAMMA
    gl_FragColor = clamp(c, 0.0, 1.0) * u_opacity;
    /***User post processing here***%userPostProcess%***/
}
