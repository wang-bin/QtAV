#!/bin/bash
set -ev
brew update
brew install ffmpeg qt5 uchardet

ls /usr/local/bin
export QTAV_OUT=QtAV-OSX-${TRAVIS_COMMIT:0:7}
mkdir -p $QTAV_OUT
export PATH=/usr/local/opt/qt5/bin:$PATH
## /usr/local/include is fine too
export CPATH=/usr/local/opt/ffmpeg/include:/usr/local/opt/libass/include
export LIBRARY_PATH=/usr/local/opt/ffmpeg/lib:/usr/local/opt/libass/lib
