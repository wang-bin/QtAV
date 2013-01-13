QtAV
==============

QtAV is a media playing library based on Qt and FFmpeg.


Shortcut
--------
Space: pause/continue

F: fullscreen on/off

T: stays on top on/off

N: show next frame. Continue the playing by pressing "Space"

O: open a file

P: replay

S: stop

M: mute on/off

Up: volume +

Down: volume -

->: seek forward

<-: seek backward


Build
------

  qmake -r "BUILD_DIR=/your/build/dir" [path/to/pro]

  make -j4

  The binaries is in $BUILD_DIR/bin

NOTE: If you are using QtCreator to build the project, you should go to Projects->Build Steps->qmake->Additional arguments, add "BUILD_DIR=your/buid/dir"


Screenshot
-------

QtAV on Win8
![Alt text](https://github.com/downloads/wang-bin/QtAV/screenshot.png "screenshot")

<div>
IP camera using QtAV. OS: Fedora 18 (some developers from Italy)
<img src="https://sourceforge.net/projects/qtav/screenshots/ip_camera.jpg"/>
</div>
