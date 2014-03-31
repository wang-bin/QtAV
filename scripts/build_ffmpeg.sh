#/bin/bash

# wbsecg1@gmail.com

# Put this script in ffmpeg source dir. Make sure your build environment is correct. Then run "./build_ffmpeg.sh"
# To build ffmpeg for android, run "./build_ffmpeg android". default is armv7.
INSTALL_DIR=sdk
# set NDK_ROOT if compile for android
NDK_ROOT="/devel/android/android-ndk-r8e"


TAGET_FLAG=$1

#host_is
function platform_is() {
  local name=$1
#TODO: osx=>darwin
  local line=`uname -a |grep -i $name`
  test -n "$line" && return 0 || return 1
}
function target_is() {
  test "$TAGET_FLAG" = "$1" && return 0 || return 1
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

function setup_wince_env() {
  WINCEOPT="--enable-cross-compile --cross-prefix=arm-mingw32ce- --target-os=mingw32ce --arch=arm --cpu=arm"
  PLATFORM_OPT="$WINCEOPT"
  INSTALL_DIR=sdk-wince
}

function setup_android_env() {
# flags are from https://github.com/yixia/FFmpeg-Android
  ANDROID_TOOLCHAIN="/tmp/ndk-toolchain" #$NDK_ROOT/toolchains/arm-linux-androideabi-4.7/prebuilt/linux-x86_64/bin
  ANDROID_SYSROOT="$ANDROID_TOOLCHAIN/sysroot" #"$NDK_ROOT/platforms/android-14/arch-arm"

  ANDROID_CFLAGS="-O3 -Wall -mthumb -pipe -fpic -fasm \
  -fstrict-aliasing -Werror=strict-aliasing \
  -fmodulo-sched -fmodulo-sched-allow-regmoves \
  -Wno-psabi -Wa,--noexecstack \
  -D__ARM_ARCH_5__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5TE__ \
  -DANDROID -DNDEBUG"
  ANDROID_CLFAGS_NEON="-march=armv7-a -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad"
  ANDROID_CFLAGS_ARMV7="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp"
  ANDROID_CFLAGS_VFP="-march=armv6 -mfpu=vfp -mfloat-abi=softfp"
  ANDROID_CFLAGS_ARMV6="-march=armv6"
  ANDROID_LDFLAGS_ARMV7="-Wl,--fix-cortex-a8"
  ANDROIDOPT="--enable-cross-compile --cross-prefix=arm-linux-androideabi- --target-os=linux --arch=arm --sysroot=$ANDROID_SYSROOT --extra-ldflags=\"$ANDROID_LDFLAGS_ARMV7\""

  $NDK_ROOT/build/tools/make-standalone-toolchain.sh --platform=android-14 --install-dir=$ANDROID_TOOLCHAIN --system=linux-x86_64
  export PATH=$ANDROID_TOOLCHAIN/bin:$PATH
  rm -r $ANDROID_SYSROOT/usr/include/{libsw*,libav*}
  rm -r $ANDROID_SYSROOT/usr/lib/{libsw*,libav*}

  PLATFORM_OPT="$ANDROIDOPT"
  EXTRA_CFLAGS="$ANDROID_CFLAGS $ANDROID_CLFAGS_ARMV7"
  INSTALL_DIR=sdk-android
}

function setup_maemo5_env() {
  MAEMO5_SYSROOT=/opt/QtSDK/Maemo/4.6.2/sysroots/fremantle-arm-sysroot-20.2010.36-2-slim
#--arch=armv7l --cpu=armv7l
#CLANG=clang
  if [ -n "$CLANG" ]; then
    CLANG_CFLAGS="-target arm-none-linux-gnueabi"
    CLANG_LFLAGS="-target arm-none-linux-gnueabi"
    HOSTCC=clang
    MAEMO_OPT="--host-cc=$HOSTCC --cc=$HOSTCC --enable-cross-compile  --target-os=linux --arch=armv7-a --sysroot=$MAEMO5_SYSROOT"
  else
    HOSTCC=gcc
    MAEMO_OPT="--host-cc=$HOSTCC --cross-prefix=arm-none-linux-gnueabi- --enable-cross-compile --target-os=linux --arch=armv7-a --sysroot=$MAEMO5_SYSROOT"
  fi
  PLATFORM_OPT="$MAEMO_OPT"
  INSTALL_DIR=sdk-maemo5
}
function setup_maemo6_env() {
  MAEMO6_SYSROOT=/opt/QtSDK/Madde/sysroots/harmattan_sysroot_10.2011.34-1_slim
#--arch=armv7l --cpu=armv7l
#CLANG=clang
  if [ -n "$CLANG" ]; then
    CLANG_CFLAGS="-target arm-none-linux-gnueabi"
    CLANG_LFLAGS="-target arm-none-linux-gnueabi"
    HOSTCC=clang
    MAEMO_OPT="--host-cc=$HOSTCC --cc=$HOSTCC --enable-cross-compile  --target-os=linux --arch=armv7-a --sysroot=$MAEMO6_SYSROOT"
  else
    HOSTCC=gcc
    MAEMO_OPT="--host-cc=$HOSTCC --cross-prefix=arm-none-linux-gnueabi- --enable-cross-compile --target-os=linux --arch=armv7-a --sysroot=$MAEMO6_SYSROOT"
  fi
  PLATFORM_OPT="$MAEMO_OPT"
  INSTALL_DIR=sdk-maemo6
}

platform_is Darwin && PLATFORM_OPT="$VDA --cc=clang --cxx=clang++"
platform_is Linux && PLATFORM_OPT="$VAAPI $VDPAU"
platform_is MinGW && PLATFORM_OPT="$DXVA"
platform_is MSYS && PLATFORM_OPT="$DXVA"

target_is android && setup_android_env
target_is maemo5 && setup_maemo5_env
target_is maemo6 && setup_maemo6_env

CONFIGURE="./configure --disable-static --enable-shared --enable-runtime-cpudetect --enable-memalign-hack --enable-avfilter --enable-avresample --disable-muxers --disable-encoders --enable-pthreads --disable-iconv --disable-bzlib --enable-hwaccels $PLATFORM_OPT --extra-cflags=\"-O3 -ftree-vectorize -ffast-math $CLANG_CFLAGS $EXTRA_CFLAGS\""

echo $CONFIGURE

time eval $CONFIGURE
if [ $? -eq 0 ]; then
  time (make -j4 install prefix="$INSTALL_DIR")
fi
  
  

# --enable-pic is default  --enable-lto
#http://cmzx3444.iteye.com/blog/1447366
#--enable-openssl  --enable-hardcoded-tables  --enable-librtmp --enable-zlib
