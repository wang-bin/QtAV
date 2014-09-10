# [QtAV](http://wang-bin.github.io/QtAV)  [![Build Status](https://travis-ci.org/wang-bin/QtAV.svg)](https://travis-ci.org/wang-bin/QtAV)

QtAV is a multimedia playback library based on Qt and FFmpeg. It can help you to write a player
with less effort than ever before.

QtAV has been added to FFmpeg projects page [http://ffmpeg.org/projects.html](http://ffmpeg.org/projects.html)

**QtAV is free software licensed under the term of LGPL v2.1. The player example is licensed under GPL v3.  If you use QtAV or its constituent libraries,
you must adhere to the terms of the license in question.**


#### [Web site](http://wang-bin.github.io/QtAV), [Download binaries from sourceforge](https://sourceforge.net/projects/qtav) , [Source code on github](https://github.com/wang-bin/QtAV)

### Features

QtAV can meet your most demands

- Hardware decoding suppprt: DXVA2, VAAPI, VDA, CedarX, CUDA(the 1st player support CUDA on linux?)
- OpenGL and ES2 support for Hi10P and other 16-bit YUV videos (The 1st player/library support in ES2? VLC, XBMC, mplayer does not support now)
- Video capture in rgb and yuv format
- OSD and custom filters
- filters in libavfilter, for example stero3d, blur
- Subtitle
- Transform video using GraphicsItemRenderer. (rotate, shear, etc)
- Playing frame by frame (currently support forward playing)
- Playback speed control. At any speed.
- Variant streams: locale file, http, rtsp, etc.
- Choose audio channel
- Choose media stream, e.g. play a desired audio track
- Multiple render engine support. Currently supports QPainter, GDI+, Direct2D, XV and OpenGL(and ES2).
- Dynamically change render engine when playing.
- Multiple video outputs for 1 player
- Region of interest(ROI), i.e. video cropping
- Video eq: brightness, contrast, saturation, hue
- QML support as a plugin. Most playback APIs are compatible with QtMultiMedia module
- Compatiblity: QtAV can be built with both Qt4 and Qt5. QtAV supports
  both FFmpeg(>=1.0) and [Libav](http://libav.org). Latest FFmpeg release is recommended. 


### Extensible Framework

  Some components in QtAV are designed to be extensible. For example, you can write your decoder, audio output for particular platform. [Here is a very good example to add cedar hardware accelerated decoder for A13-OLinuXino](https://github.com/mireq/QtAV/commit/d7b428c1dae66b2a85b7a6bfa7b253980b5b963c)


# For Developers

#### Requirements

[![Qt](http://qt-project.org/images/qt13a/Qt-logo.png "Qt4.8 or Qt5")](http://qt-project.org)
[![FFmpeg](http://ffmpeg.org/ffmpeg-logo.png "(>=1.0)Latest version is recommanded")](http://ffmpeg.org)
![OpenAL](http://upload.wikimedia.org/wikipedia/zh/2/28/OpenAL_logo.png "OpenAL or OpenAL soft") 
[![Libav](http://libav.org/libav-logo-text.png ">=9.0")](http://libav.org)
[![PortAudio](http://www.portaudio.com/images/portaudio_logotext.png)](http://www.portaudio.com)
 
Latest FFmpeg, Qt5 and OpenAL releases are preferred.

**The required development files for MinGW can be found in sourceforge
page: [depends](https://sourceforge.net/projects/qtav/files/depends)**

#### Build

See the wiki [Build QtAV](https://github.com/wang-bin/QtAV/wiki/Build-QtAV) and [QtAV Build Configurations](https://github.com/wang-bin/QtAV/wiki/QtAV-Build-Configurations)


#### How To Write a Player

Wrtie a media player using QtAV is quite easy.

    GLWidgetRenderer renderer;
    renderer.show();
    AVPlayer player;
    player.setRenderer(&renderer);
    player.play("test.avi");

For more detail to using QtAV, see the wiki [Use QtAV In Your Project](https://github.com/wang-bin/QtAV/wiki/Use-QtAV-In-Your-Projects) or examples.


QtAV can also be used in **Qml**

    import QtQuick 2.0
    import QtAV 1.3
    Item {
        VideoOutput {
            anchors.fill: parent
            source: player
        }
        AVPlayer { //or MediaPlayer
            id: player
            source: "test.mp4"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: player.play()
        }
    }

### How To Contribute

- [Fork](https://github.com/wang-bin/QtAV/fork) QtAV project on github and make a branch. Commit in that branch, and push, then create a pull request to be reviewed and merged.
- [Create an issue](https://github.com/wang-bin/QtAV/issues/new) if you have any problem when using QtAV or you find a bug, etc.
- What you can do: translation, write document, wiki, find or fix bugs, give your idea for this project etc.

#### Contributors

- Wang Bin(Lucas Wang) <wbsecg1@gmail.com>: creator, maintainer
- Alexander
- skaman: aspect ratio from stream
- Stefan Ladage <sladage@gmail.com>: QIODevice support. Wiki about build QtAV for iOS. Let OpenAL work on OSX and iOS
- Miroslav Bendik <miroslav.bendik@gmail.com>: Cedarv support. Better qmlvideofx appearance
- Dimitri E. Prado <dprado@e3c.com.br>: issue 70
- theoribeiro <theo@fictix.com.br>: initial QML support
- Vito Covito <vito.covito@selcomsrl.eu>: interrupt callback

For End Users
-------------

#### Player Usage

An simple player can be found in examples. The command line options is

    player [-ao null] [-vo qt|gl|d2d|gdi|xv] [-vd "dxva[cuda[;vaapi[;vda[;ffmpeg]]]]"] [--no-ffmpeg-log] [url|path|pipe:]

To disable audio output, add `-ao null`

Choose a render engine with _-vo_ option(default is OpenGL). For example, in windows that support Direct2D, you can run

    player -vo d2d filename

To select decoder, use `-vd` option. Value can be _dxva_, _vaapi_ and _ffmpeg_, or a list separated by `;` in priority order. For example:

    player -vd "dxva;ffmpeg" filename

will use dxva if dxva can decode, otherwise ffmpeg will be used.


#### Default Shortcuts

- Double click: fullscreen switch
- Ctrl+O: open a file
- Space: pause/continue
- F: fullscreen on/off
- I: switch display quality
- T: stays on top on/off
- N: show next frame. Continue the playing by pressing "Space"
- O: OSD
- P: replay
- Q/ESC: quit
- S: stop
- R: switch aspect ratio
- M: mute on/off
- Up / Down: volume + / -
- Ctrl+Up/Down: speed + / -
- -> / <-: seek forward / backward
- Crtl+Wheel: zoom in/out
- Drag and drop a media file to player


# TODO

Read https://github.com/wang-bin/QtAV/wiki/TODO for detail.

Screenshots
----------

Use QtAV in QML with OpenGL shaders(example is from qtmultimedia. But qtmultimedia is replaced by QtAV)

![Alt text](https://sourceforge.net/p/qtav/screenshot/QtAV-QML-Shader.jpg "QtAV QML Shaders")

QtAV on Mac OS X

![Alt text](https://sourceforge.net/p/qtav/screenshot/mac.jpg "player on OSX")

QMLPlayer on ubuntu

![QMLPlayer](https://sourceforge.net/p/qtav/screenshot/QMLPlayer%2BQtAV.jpg "QMLPlayer")

Video Wall

![Alt text](https://sourceforge.net/p/qtav/screenshot/videowall.png "video wall")



***
### Donate 资助

[PayPal ![Paypal](http://www.paypal.com/en_US/i/btn/btn_donate_LG.gif)](http://wang-bin.github.io/qtav.org#donate)
[![Support via Gittip](https://rawgithub.com/twolfson/gittip-badge/0.1.0/dist/gittip.png)](https://www.gittip.com/wang-bin)
[Gittip ![Gittip](https://www.gittip.com/assets/10.1.51/logo.png)](https://www.gittip.com/wang-bin)

- - -



> Copyright &copy; Wang Bin wbsecg1@gmail.com

> Shanghai University->S3 Graphics, Shanghai, China

> 2013-01-21