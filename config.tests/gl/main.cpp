/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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
#include <qgl.h> //check type conflicting for gl.h and GLES2/gl2.h
//gl.h is auto included by qt build with gl(not gles)
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif //__APPLE__

int main()
{
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glPushMatrix();
    glLoadIdentity();
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, +1.0);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glMatrixMode(GL_MODELVIEW);

	return 0;
}
