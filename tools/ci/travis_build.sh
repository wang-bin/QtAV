#!/bin/bash

# http://docs.travis-ci.com/user/installing-dependencies/
# http://docs.travis-ci.com/user/customizing-the-build/
# http://docs.travis-ci.com/user/build-configuration/
#   - sudo add-apt-repository ppa:wsnipex/vaapi
set -ev

echo "$TRAVIS_BUILD_DIR"

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
FFVER=(1.0.10 1.2.10 2.0.6 2.2.11)
QTVER=(4.8.6 5.0.2 5.1.1 5.2.1)
MKSPEC=(linux-clang)

cat > Makefile.download <<EOF
PKG=\$(addprefix ffmpeg-,${FFVER[@]}) \$(addsuffix -Linux64, \$(addprefix Qt,${QTVER[@]}))

all: \$(PKG)
	echo \$^

ffmpeg-%:
	test -f \$@-linux-x86+x64.tar.xz || wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/linux/\$@-linux-x86+x64.tar.xz/download -O \$@-linux-x86+x64.tar.xz
	tar Jxf \$@-linux-x86+x64.tar.xz

Qt%:
	test -f \$@.tar.xz || wget http://sourceforge.net/projects/buildqt/files/lite/\$@.tar.xz/download -O \$@.tar.xz
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
    local QT=$3
    local FFSDK=$WORK_DIR/build/$FF-$VER-linux-x86+x64
    export PATH=$WORK_DIR/build/${QT}-Linux64/bin:$PATH
    export CPATH=$FFSDK/include
    export LIBRARY_PATH=$FFSDK/lib/x64
    export LD_LIBRARY_PATH=$FFSDK/lib/x64
    echo "PATH=$PATH"
    echo "CPATH=$CPATH"
    echo "building... with $QT $FFSDK"
    OUT=qtav_${QT}_$FF$VER
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
for (( i=0; i<${#FFVER[@]}; i++ )); do
    buildqtav ffmpeg "${FFVER[i]}" "Qt${QTVER[i]}"
done
)
