/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
attribute vec4 a_Position;
attribute vec2 a_TexCoords0;
uniform mat4 u_Matrix;
varying vec2 v_TexCoords0;
#ifdef MULTI_COORD
attribute vec2 a_TexCoords1;
attribute vec2 a_TexCoords2;
varying vec2 v_TexCoords1;
varying vec2 v_TexCoords2;
#ifdef HAS_ALPHA
attribute vec2 a_TexCoords3;
varying vec2 v_TexCoords3;
#endif
#endif //MULTI_COORD
/***User header code***%userHeader%***/

void main() {
    gl_Position = u_Matrix * a_Position;
    v_TexCoords0 = a_TexCoords0;
#ifdef MULTI_COORD
    v_TexCoords1 = a_TexCoords1;
    v_TexCoords2 = a_TexCoords2;
#ifdef HAS_ALPHA
    v_TexCoords3 = a_TexCoords3;
#endif
#endif //MULTI_COORD
}
