#/bin/bash

# wbsecg1@gmail.com

function platform_is() {
  local name=$1
#TODO: osx=>darwin  
  local line=`uname -a |grep -i $name`
  test -n "$line" && return 0 || return 1
}
function is_libav() {
  test "${PWD/libav*/}" = "$PWD" && return 1 || return 0
}

#CPU_FLAGS=-mmmx -msse -mfpmath=sse
#ffmpeg 2.x autodetect dxva, vaapi, vdpau. manually enable vda
DXVA="--enable-dxva2" #  --enable-hwaccel=h264_dxva2 --enable-hwaccel=mpeg2_dxva2 --enable-hwaccel=vc1_dxva2 --enable-hwaccel=wmv3_dxva2"
VAAPI="--enable-vaapi" # --enable-hwaccel=h263_vaapi --enable-hwaccel=h264_vaapi --enable-hwaccel=mpeg2_vaapi --enable-hwaccel=mpeg4_vaapi --enable-hwaccel=vc1_vaapi --enable-hwaccel=wmv3_vaapi"
VDPAU="--enable-vdpau" # --enable-hwaccel=h263_vdpau --enable-hwaccel=h264_vdpau --enable-hwaccel=mpeg1_vdpau --enable-hwaccel=mpeg2_vdpau --enable-hwaccel=mpeg4_vdpau --enable-hwaccel=vc1_vdpau --enable-hwaccel=wmv3_vdpau"
VDA="--enable-vda" # --enable-hwaccel=h264_vda"

platform_is Darwin && PLATFORM_OPT="$VDA --cc=clang --cxx=clang++"
platform_is Linux && PLATFORM_OPT="$VAAPI $VDPAU"
platform_is MinGW && PLATFORM_OPT="$DXVA"
platform_is MSYS && PLATFORM_OPT="$DXVA"
#SYS_ROOT=/opt/QtSDK/Maemo/4.6.2/sysroots/fremantle-arm-sysroot-20.2010.36-2-slim
#--arch=armv7l --cpu=armv7l 
#CLANG=clang
if [ -n "$CLANG" ]; then
  CLANG_CFLAGS="-target arm-none-linux-gnueabi"
  CLANG_LFLAGS="-target arm-none-linux-gnueabi"
  HOSTCC=clang
  MAEMO5OPT="--host-cc=$HOSTCC --cc=$HOSTCC --enable-gpl --enable-version3 --enable-nonfree --enable-cross-compile  --target-os=linux --arch=armv7-a --sysroot=$SYS_ROOT"
else
  HOSTCC=gcc
  MAEMO5OPT="--host-cc=$HOSTCC --enable-gpl --enable-version3 --enable-nonfree --enable-cross-compile --cross-prefix=arm-none-linux-gnueabi- --target-os=linux --arch=armv7-a --sysroot=$SYS_ROOT"
fi

CONFIGURE="./configure --disable-static --enable-shared --enable-runtime-cpudetect --enable-memalign-hack --disable-avdevice --enable-avfilter --enable-avresample --disable-postproc --disable-muxers --disable-encoders --enable-pthreads --disable-iconv --disable-bzlib --enable-hwaccels $PLATFORM_OPT --extra-cflags=\"-O3 -ftree-vectorize -ffast-math $CLANG_CFLAGS\""

echo $CONFIGURE

time eval $CONFIGURE

# --enable-pic is default  --enable-lto
#http://cmzx3444.iteye.com/blog/1447366
#--enable-openssl  --enable-hardcoded-tables  --enable-librtmp --enable-zlib
