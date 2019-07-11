# [QtAV](http://www.qtav.org)  [![Build Status](https://travis-ci.org/wang-bin/QtAV.svg)](https://travis-ci.org/wang-bin/QtAV) [![Appveyor](https://ci.appveyor.com/api/projects/status/github/wang-bin/qtav?svg=true&passingText=windows%20-%20OK)](https://ci.appveyor.com/project/wang-bin/qtav)

[![Join the chat at https://gitter.im/QtAV/Lobby](https://badges.gitter.im/QtAV/Lobby.svg)](https://gitter.im/QtAV/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

QtAV is a multimedia playback library based on Qt and FFmpeg. It can help you to write a player
with less effort than ever before.

QtAV has been added to FFmpeg projects page [http://ffmpeg.org/projects.html](http://ffmpeg.org/projects.html)

**QtAV is free software licensed under the term of LGPL v2.1. The player example is licensed under GPL v3.  If you use QtAV or its constituent libraries,
you must adhere to the terms of the license in question.**


#### [Home page](http://www.qtav.org)

### Features

QtAV can meet your most demands

- Hardware decoding suppprt: DXVA2, VAAPI, VDA/VideoToolbox, CedarX, CUDA(the 1st player support CUDA on linux?)
- OpenGL and ES2 support for almost all formats including Hi10P videos (The 1st player/library support 10bit in ES2? VLC, XBMC, mplayer does not support now)
- Real time preview
- Video capture in rgb and yuv format
- OSD and custom filters
- Filters in libavfilter, for example stero3d, blur
- Subtitle track select. Dynamic change FFmpeg and libass engine
- Play frame by frame
- Playback speed control
- Variant streams: locale file, http, rtsp etc. and your custom streams
- Audio channel, tracks and external audio tracks
- Dynamically change render engine when playing.
- Dynamically change video decoder
- Multiple video outputs for 1 player
- Video eq(software and OpenGL): brightness, contrast, saturation, hue
- QML support. Most playback APIs are compatible with QtMultimedia module
- Compatibility: QtAV can be built with both Qt4 and Qt5, FFmpeg(>=1.0) and [Libav](http://libav.org) (>=9.0). Latest FFmpeg release is recommended.


### Extensible Framework

  Some components in QtAV are designed to be extensible. For example, you can write your decoder, audio output for particular platform. [Here is a very good example to add cedar hardware accelerated decoder for A13-OLinuXino](https://github.com/mireq/QtAV/commit/d7b428c1dae66b2a85b7a6bfa7b253980b5b963c)


# For Developers

#### Requirements

[![Qt](http://upload.wikimedia.org/wikipedia/commons/thumb/9/94/Qt_logo.svg/64px-Qt_logo.svg.png "Qt4.8 or Qt5")](http://www.qt.io)
[![FFmpeg](http://ffmpeg.org/ffmpeg-logo.png "(>=1.0)Latest version is recommanded")](http://ffmpeg.org)
[![Libav](http://libav.org/libav-logo-text.png ">=9.0")](http://libav.org)
![OpenAL](http://upload.wikimedia.org/wikipedia/zh/2/28/OpenAL_logo.png "OpenAL or OpenAL soft")

**The required development files to build QtAV can be found in sourceforge
page: [depends](https://sourceforge.net/projects/qtav/files/depends)**

#### Build

See the wiki [Build QtAV](https://github.com/wang-bin/QtAV/wiki/Build-QtAV) and [QtAV Build Configurations](https://github.com/wang-bin/QtAV/wiki/QtAV-Build-Configurations)


#### How To Write a Player

Write a media player using QtAV is quite easy.

    GLWidgetRenderer2 renderer;
    renderer.show();
    AVPlayer player;
    player.setRenderer(&renderer);
    player.play("test.avi");

For more detail to using QtAV, see the wiki [Use QtAV In Your Project](https://github.com/wang-bin/QtAV/wiki/Use-QtAV-In-Your-Projects) or examples.


QtAV can also be used in **Qml**

    import QtQuick 2.0
    import QtAV 1.6
    Item {
        Video {
            id: video
            source: "test.mp4"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: video.play()
        }
    }

### How To Contribute

- [Fork](https://github.com/wang-bin/QtAV/fork) QtAV project on github and make a branch. Commit in that branch, and push, then create a pull request to be reviewed and merged.
- [Create an issue](https://github.com/wang-bin/QtAV/issues/new) if you have any problem when using QtAV or you find a bug, etc.
- What you can do: translation, write document, wiki, find or fix bugs, give your idea for this project etc.

#### Contributors

- Wang Bin(Lucas Wang): creator, maintainer
- Gianluigi Tiesi(sherpya): avdevice input support
- Stefan Ladage: QIODevice support. Wiki about build QtAV for iOS. Let OpenAL work on OSX and iOS
- Miroslav Bendik: Cedarv support. Better qmlvideofx appearance
- theoribeiro: initial QML support
- Vito Covito: interrupt callback
- Alexander, Marius Wachtler, Petar Koretić, Sandro Cavazzoni(skaman), Dimitri E. Prado, karlox ...

For End Users
-------------

#### Player Commandline Options

Run `player -h`


#### Default Shortcuts

- Double click: fullscreen switch
- Ctrl+O: open a file
- Space: pause/continue
- F: fullscreen on/off
- T: stays on top on/off
- N/B: show the next/previous frame. Continue the playing by pressing "Space"
- O: OSD
- P: replay
- Q/ESC: quit
- S: stop
- A: switch aspect ratio
- R: rotate 90
- M: mute on/off
- Up / Down: volume + / -
- Ctrl+Up/Down: speed + / -
- -> / <-: seek forward / backward
- Crtl+Wheel: zoom in/out
- Drag and drop a media file to player


Screenshots
----------

Use QtAV in QML with OpenGL shaders(example is from qtmultimedia. But qtmultimedia is replaced by QtAV)

![Alt text](https://sourceforge.net/p/qtav/screenshot/QtAV-QML-Shader.jpg "QtAV QML Shaders")

![Alt text](http://www.qtav.org/screenshots/player-OSX.jpg "player on OSX")

![QMLPlayer](http://www.qtav.org/screenshots/QMLPlayer-preview-ubuntu.jpg "QMLPlayer")

![Alt text](http://www.qtav.org/screenshots/videowall.png "video wall")



***
### [Donate 捐赠](http://www.qtav.org/donate.html)




> Copyright &copy; Wang Bin wbsecg1@gmail.com

> 2013-01-21
