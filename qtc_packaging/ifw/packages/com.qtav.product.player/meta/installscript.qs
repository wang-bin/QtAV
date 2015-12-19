/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
**************************************************************************/

function Component()
{
    // constructor
}

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}

Component.prototype.createOperations = function()
{
    try {
        // call the base create operations function
        component.createOperations();
    } catch (e) {
        print(e);
    }

    if (installer.value("os") === "win") {
        component.addOperation("CreateShortcut", "@TargetDir@/bin/player.exe", "@StartMenuDir@/QtAV.Player.lnk",
            "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/player.exe",
            "iconId=0");
        component.addOperation("CreateShortcut", "@TargetDir@/bin/QMLPlayer.exe", "@StartMenuDir@/QtAV.Player.QML.lnk",
            "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/QMLPlayer.exe",
            "iconId=0");
        component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe", "@StartMenuDir@/QtAV.Uninstall.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/maintenancetool.exe",
            "iconId=0");

        //component.addOperation("Settings", "formate=native", //"path=HKEY_CURRENT_USER/SOFTWARE/Classes/*/shell/OpenWithQtAV/command", "method=set", "key=Default", "value=@TargetDir@/bin/player.exe -vo  gl -f %%1");

        component.addOperation("GlobalConfig", "HKEY_CURRENT_USER\\SOFTWARE\\Classes\\*\\shell\\Open With QtAV\\command", "Default", "\"@TargetDir@/bin/player.exe\" -vo gl -f \"%1\"");
        component.addOperation("GlobalConfig", "HKEY_CURRENT_USER\\SOFTWARE\\Classes\\*\\shell\\Open With QtAV QML\\command", "Default", "\"@TargetDir@/bin/QMLPlayer.exe\" -f \"%1\"");
    }
    if (installer.value("os") === "x11") {
        var d = "Type=Application\nComment=A media playing framework based on Qt and FFmpeg\nKeywords=movie;video;player;multimedia;\nIcon=QtAV\nTerminal=false\nHomepage=https://github.com/wang-bin/QtAV\nCategories=Qt;AudioVideo;Player;Video;Development;\nMimeType=application/mxf;application/ogg;application/ram;application/sdp;application/smil;application/smil+xml;application/vnd.ms-wpl;application/vnd.rn-realmedia;application/x-extension-m4a;application/x-extension-mp4;application/x-flac;application/x-flash-video;application/x-matroska;application/x-netshow-channel;application/x-ogg;application/x-quicktime-media-link;application/x-quicktimeplayer;application/x-shorten;application/x-smil;application/xspf+xml;audio/3gpp;audio/ac3;audio/AMR;audio/AMR-WB;audio/basic;audio/midi;audio/mp2;audio/mp4;audio/mpeg;audio/mpegurl;audio/ogg;audio/prs.sid;audio/vnd.rn-realaudio;audio/x-aiff;audio/x-ape;audio/x-flac;audio/x-gsm;audio/x-it;audio/x-m4a;audio/x-matroska;audio/x-mod;audio/x-mp3;audio/x-mpeg;audio/x-mpegurl;audio/x-ms-asf;audio/x-ms-asx;audio/x-ms-wax;audio/x-ms-wma;audio/x-musepack;audio/x-pn-aiff;audio/x-pn-au;audio/x-pn-realaudio;audio/x-pn-realaudio-plugin;audio/x-pn-wav;audio/x-pn-windows-acm;audio/x-realaudio;audio/x-real-audio;audio/x-sbc;audio/x-scpls;audio/x-speex;audio/x-tta;audio/x-wav;audio/x-wavpack;audio/x-vorbis;audio/x-vorbis+ogg;audio/x-xm;image/vnd.rn-realpix;image/x-pict;misc/ultravox;text/google-video-pointer;text/x-google-video-pointer;video/3gpp;video/dv;video/fli;video/flv;video/mp2t;video/mp4;video/mp4v-es;video/mpeg;video/msvideo;video/ogg;video/quicktime;video/vivo;video/vnd.divx;video/vnd.rn-realvideo;video/vnd.vivo;video/webm;video/x-anim;video/x-avi;video/x-flc;video/x-fli;video/x-flic;video/x-flv;video/x-m4v;video/x-matroska;video/x-mpeg;video/x-ms-asf;video/x-ms-asx;video/x-msvideo;video/x-ms-wm;video/x-ms-wmv;video/x-ms-wmx;video/x-ms-wvx;video/x-nsv;video/x-ogm+ogg;video/x-theora+ogg;video/x-totem-stream;x-content/video-dvd;x-content/video-vcd;x-content/video-svcd;x-scheme-handler/pnm;x-scheme-handler/mms;x-scheme-handler/net;x-scheme-handler/rtp;x-scheme-handler/rtsp;x-scheme-handler/mmsh;x-scheme-handler/uvox;x-scheme-handler/icy;x-scheme-handler/icyx;\nKeywords=Player;DVD;Audio;Video;";
        var player = "Name=QtAV Player\nGenericName=QtAV Player\nTryExec=@TargetDir@/bin/player.sh\nExec=@TargetDir@/bin/player.sh -f %U\n" + d;
        var qmlplayer = "Name=QtAV QMLPlayer\nGenericName=QtAV QMLPlayer\nTryExec=@TargetDir@/bin/QMLPlayer.sh\nExec=@TargetDir@/bin/QMLPlayer.sh -f %U\n" + d;
        component.addOperation("CreateDesktopEntry", "QtAV.Player.desktop", player);
        component.addOperation("CreateDesktopEntry", "QtAV.QMLPlayer.desktop", qmlplayer);
        component.addOperation("CreateDesktopEntry", "QtAV.Uninstall.desktop", "Name=QtAV Uninstall\nGenericName=QtAV Uninstall\nTryExec=@TargetDir@/uninstall\nExec=@TargetDir@/uninstall\nType=Application\nIcon=QtAV\nTerminal=false");
    }
}
