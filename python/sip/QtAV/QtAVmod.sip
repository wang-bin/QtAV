%Module(name=QtAV, keyword_arguments="Optional")

%Import QtGui/QtGuimod.sip
%Import QtWidgets/QtWidgetsmod.sip
%Import QtOpenGL/QtOpenGLmod.sip

%Timeline {QtAV_1_11_0 QtAV_1_12_0}

%Feature FULL_API 

%Copying
QtAV:  Multimedia framework based on Qt and FFmpeg
Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

This file is part of QtAV (from 2015)

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
%End

%HideNamespace QtAV

%Include global.sip
%Include VideoOutput.sip
%Include AVError.sip
%Include AVPlayer.sip

%If (FULL_API)
%Include VideoFormat.sip
%Include Frame.sip
%Include VideoFrame.sip
%Include QPainterRenderer.sip
%Include AVOutput.sip
%Include VideoRenderer.sip
%Include OpenGLRendererBase.sip
%Include OpenGLVideo.sip
%End
