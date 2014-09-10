#!/bin/bash

# http://docs.travis-ci.com/user/installing-dependencies/
# http://docs.travis-ci.com/user/customizing-the-build/
# http://docs.travis-ci.com/user/build-configuration/
#   - sudo add-apt-repository ppa:wsnipex/vaapi
set -ev

tail -n 27 /proc/cpuinfo
uname -a
cat /etc/issue

echo "QtAV build script for travis-ci"

WORK_DIR=$PWD
echo "WORK_DIR=$WORK_DIR"
mkdir -p build
cd build

#ffmpeg 2.3 libavutil clock_gettime need -lrt if glibc < 2.17
FFMPEG=$1
: ${FFMPEG:=ffmpeg-2.2.5}
FFVER=(1.0.9 1.1.12 1.2.7 2.0.5 2.1.5 2.2.5)
MKSPEC=(linux-clang)

export QT_PKG=Qt5.3.1-Linux64
export FFMPEG_PKG=${FFMPEG}-linux-x86+x64

:<<EOF
function get_qt() {
    test -d ${QT_PKG} || {
      echo "---------get ${QT_PKG}----------"
      test -f ${QT_PKG}.tar.xz || wget http://sourceforge.net/projects/buildqt/files/release/5.3.1/${QT_PKG}.tar.xz/download -O ${QT_PKG}.tar.xz
      tar -I xz -xf ${QT_PKG}.tar.xz
    }
}

function get_ffmpeg() {
    test -d ${FFMPEG_PKG} || {
    echo "---------get ${FFMPEG_PKG}----------"
      test -f ${FFMPEG_PKG}.tar.xz || wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/linux/${FFMPEG_PKG}.tar.xz/download -O ${FFMPEG_PKG}.tar.xz
      tar -I xz -xf ${FFMPEG_PKG}.tar.xz
    }
}

export -f get_qt
export -f get_ffmpeg

#parallel ::: get_qt get_ffmpeg
get_qt
get_ffmpeg


export CPATH=$PWD/${FFMPEG_PKG}/include
export LIBRARY_PATH=$PWD/${FFMPEG_PKG}/lib/x64
export LD_LIBRARY_PATH=$LIBRARY_PATH

echo "CPATH=$CPATH"
echo "LIBRARY_PATH=$LIBRARY_PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

EOF

export PATH=$PWD/${QT_PKG}/bin:$PATH
echo "PATH=$PATH"

cat > Makefile.download <<EOF
PKG=\$(addprefix ffmpeg-,${FFVER[@]}) $QT_PKG

all: \$(PKG)
	echo \$^

ffmpeg-%:
	test -f \$@-linux-x86+x64.tar.xz || wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/linux/\$@-linux-x86+x64.tar.xz/download -O \$@-linux-x86+x64.tar.xz
	tar Jxf \$@-linux-x86+x64.tar.xz

Qt5%:
	test -f \$@.tar.xz || wget http://sourceforge.net/projects/buildqt/files/release/5.3.1/\$@.tar.xz/download -O \$@.tar.xz
	tar -I xz -xf \$@.tar.xz

EOF

time make -f Makefile.download -j $((${#FFVER[@]}+1))


function build_spec() {
    local spec=$1
    echo "building with qmake spec $spec ..."
    local cpu_cores=`cat /proc/cpuinfo |grep 'cpu cores' |wc -l`
    local jobs=$(($cpu_cores * 2))
    echo "jobs=$jobs"
    mkdir -p $spec
    cd $spec
    rm -f build.log
    qmake -r $WORK_DIR -spec $spec -config silent
    time make -j${jobs} 2>&1 |tee build.log
    cd $WORK_DIR/build
}

buildqtav() {
    local FF=$1
    local VER=$2
    local FFSDK=$WORK_DIR/build/$FF-$VER-linux-x86+x64
    export CPATH=$FFSDK/include
    export LIBRARY_PATH=$FFSDK/lib/x64
    export LD_LIBRARY_PATH=$FFSDK/lib/x64
    echo $CPATH
    echo "building... with $FFSDK"
    OUT=qtav_$FF$VER
    mkdir -p $OUT
    cd $OUT
    for spec in ${MKSPEC[@]}; do
      build_spec $spec
      cd $OUT
      ln -sf $FFSDK/lib/x64/* $spec/bin
    done
    cd $WORK_DIR/build
}

time (
for V in ${FFVER[@]}; do
    buildqtav ffmpeg $V
done
)
