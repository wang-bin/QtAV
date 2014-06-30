#!/bin/bash

# http://docs.travis-ci.com/user/installing-dependencies/
# http://docs.travis-ci.com/user/customizing-the-build/
# http://docs.travis-ci.com/user/build-configuration/
#   - sudo add-apt-repository ppa:wsnipex/vaapi
set -ev

tail -n 27 /proc/cpuinfo
uname -a

echo "QtAV build script for travis-ci"

WORK_DIR=$PWD
echo "WORK_DIR=$WORK_DIR"
mkdir -p build
cd build


QT_PKG=Qt5.3.0-Linux64
FFMPEG_PKG=ffmpeg-2.2.2-linux-x86+x64

test -d ${QT_PKG} || {
  test -f ${QT_PKG}.tar.xz || wget http://sourceforge.net/projects/buildqt/files/release/5.3.0/${QT_PKG}.tar.xz/download -O ${QT_PKG}.tar.xz
  tar Jxf ${QT_PKG}.tar.xz
}

test -d ${FFMPEG_PKG} || {
  test -f ${FFMPEG_PKG}.tar.xz || wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/linux/${FFMPEG_PKG}.tar.xz/download -O ${FFMPEG_PKG}.tar.xz
  tar Jxf ${FFMPEG_PKG}.tar.xz
}

export PATH=$PWD/${QT_PKG}/bin:$PATH
export CPATH=$PWD/${FFMPEG_PKG}/include
export LIBRARY_PATH=$PWD/${FFMPEG_PKG}/lib/x64
export LD_LIBRARY_PATH=$LIBRARY_PATH

echo "PATH=$PATH"
echo "CPATH=$CPATH"
echo "LIBRARY_PATH=$LIBRARY_PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"


function build_spec() {
    local spec=$1
    echo "building with qmake spec $spec ..."
    local cpu_cores=`cat /proc/cpuinfo |grep 'cpu cores' |wc -l`
    local jobs=$(($cpu_cores * 2))
    echo "jobs=$jobs"
    mkdir -p $spec
    cd $spec

    qmake -r $WORK_DIR -spec $spec
    time make -j${jobs}
    cd $WORK_DIR/build
}

MKSPEC=(linux-g++ linux-clang)
for spec in ${MKSPEC[@]}; do
  build_spec $spec
done


