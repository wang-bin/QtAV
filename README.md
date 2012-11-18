QtAV
==============

QtAV is a media playing library based on Qt and FFmpeg.


Shortcut
--------
Space: pause/continue

F: fullscreen on/off

T: stays on top on/off

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
  BUILD_DIR=/your/build/dir qmake

  or  qmake   (BUILD_DIR is $PWD/out)

  or  cd build_dir ; qmake /path/to/QtAV.pro

  make -j4

  The binaries is in $BUILD_DIR/bin


Screenshot
-------

![Alt text](https://github.com/downloads/wang-bin/QtAV/screenshot.png "screenshot")
