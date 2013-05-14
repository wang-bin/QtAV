#/bin/bash

IS_MINGW=`uname -a |grep -i mingw`
IS_MINGW=`test -n "$IS_MINGW"`
IS_MSYS=`uname -a |grep -i msys`
IS_MSYS=`test -n "$IS_MSYS"`

type -a yasm &>/dev/null || echo "You'd better to install yasm!"

DXVA="--enable-dxva2  --enable-hwaccel=h264_dxva2 --enable-hwaccel=mpeg2_dxva2 --enable-hwaccel=vc1_dxva2 --enable-hwaccel=wmv3_dxva2"

CONFIGURE="./configure --disable-static --enable-shared --enable-runtime-cpudetect --enable-memalign-hack --disable-avdevice --enable-avfilter --enable-avresample --disable-postproc --disable-ffplay --disable-ffserver --enable-ffprobe --disable-muxers --disable-encoders --enable-pthreads --disable-iconv --disable-bzlib  --enable-hwaccels  $DXVA --extra-cflags=\"-O3  -ftree-vectorize -Wundef -Wdisabled-optimization -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -foptimize-sibling-calls -fstrength-reduce -frerun-loop-opt -frename-registers -ffast-math -fomit-frame-pointer\""

echo $CONFIGURE

time eval $CONFIGURE

# --enable-pic is default  --enable-lto
#http://cmzx3444.iteye.com/blog/1447366
#--enable-openssl  --enable-hardcoded-tables  --enable-librtmp --enable-zlib