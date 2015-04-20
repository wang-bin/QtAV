#/bin/bash
#TODO: test both x86 x64. cc test then build with gcc -m32 -m64 or --enable-cross-compile --target-os=linux --arch=x86
# Author: wbsecg1@gmail.com 2013-2015
echo "This is part of QtAV project. Get the latest script from https://github.com/wang-bin/QtAV/tree/master/scripts"

# Put this script in ffmpeg source dir. Make sure your build environment is correct. Then run "./build_ffmpeg.sh"
# To build ffmpeg for android, run "./build_ffmpeg android". default is armv7-a.

PLATFORMS="ios|android|maemo5|maemo6|vc|x86"
echo "Put this script in ffmpeg source dir. Make sure your build environment is correct."
echo "usage: ./build_ffmpeg.sh [${PLATFORMS}]"
echo "(optional) set var in config-xxx.sh, xxx is ${PLATFORMS//\|/, }"
echo "var can be: INSTALL_DIR, NDK_ROOT, MAEMO5_SYSROOT, MAEMO6_SYSROOT"
echo "Author: wbsecg1@gmail.com 2013-2015"
# TODO: PLATFORM=xxx TARGET=ooo TOOLCHAIN=ttt ./build_ffmpeg.sh

TAGET_FLAG=$1
TAGET_ARCH_FLAG=$2 #${2:-$1}

if [ -n "$TAGET_FLAG" ]; then
  USER_CONFIG=config-${TAGET_FLAG}.sh
  test -f $USER_CONFIG &&  . $USER_CONFIG
fi

: ${INSTALL_DIR:=sdk}
# set NDK_ROOT if compile for android
: ${NDK_ROOT:="/devel/android/android-ndk-r8e"}
: ${MAEMO5_SYSROOT:=/opt/QtSDK/Maemo/4.6.2/sysroots/fremantle-arm-sysroot-20.2010.36-2-slim}
: ${MAEMO6_SYSROOT:=/opt/QtSDK/Madde/sysroots/harmattan_sysroot_10.2011.34-1_slim}
: ${LIB_OPT:="--enable-shared"}
: ${MISC_OPT="--enable-hwaccels"}#--enable-gpl --enable-version3
CONFIGURE=configure


toupper(){
    echo "$@" | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ
}

tolower(){
    echo "$@" | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
}

#host_is
platform_is() {
  local name=$1
#TODO: osx=>darwin
  local line=`uname -a |grep -i $name`
  test -n "$line" && return 0 || return 1
}
target_is() {
  test "$TAGET_FLAG" = "$1" && return 0 || return 1
}
target_arch_is() {
  test "$TAGET_ARCH_FLAG" = "$1" && return 0 || return 1
}
is_libav() {
  test "${PWD/libav*/}" = "$PWD" && return 1 || return 0
}

enable_opt() {
  local OPT=$1
  # grep -m1
  grep "\-\-enable\-$OPT" configure && eval ${OPT}_opt="--enable-$OPT" &>/dev/null
}
#CPU_FLAGS=-mmmx -msse -mfpmath=sse
#ffmpeg 1.2 autodetect dxva, vaapi, vdpau. manually enable vda before 2.3
enable_opt dxva2
enable_opt vaapi
enable_opt vdpau
enable_opt vda

# clock_gettime in librt instead of glibc>=2.17
grep "LIBRT" configure &>/dev/null && {
  # TODO: cc test
  platform_is Linux && ! target_is android && EXTRALIBS="$EXTRALIBS -lrt"
}
#avr >= ffmpeg0.11
#FFMAJOR=`pwd |sed 's,.*-\(.*\)\..*\..*,\1,'`
#FFMINOR=`pwd |sed 's,.*\.\(.*\)\..*,\1,'`
# n1.2.8, 2.5.1, 2.5
FFMAJOR=`./version.sh |sed 's,[a-zA-Z]*\([0-9]*\)\..*,\1,'`
FFMINOR=`./version.sh |sed 's,[a-zA-Z]*[0-9]*\.\([0-9]*\).*,\1,'`
echo "FFmpeg/Libav version: $FFMAJOR.$FFMINOR"

setup_vc_env() {
# http://ffmpeg.org/platform.html#Microsoft-Visual-C_002b_002b-or-Intel-C_002b_002b-Compiler-for-Windows
  #TOOLCHAIN_OPT=
  test -n "$dxva2_opt" && PLATFORM_OPT="$PLATFORM_OPT $dxva2_opt"
  PLATFORM_OPT="$PLATFORM_OPT --toolchain=msvc"
  CL_INFO=`cl 2>&1 |grep -i Microsoft`
  CL_VER=`echo $CL_INFO |sed 's,.* \([0-9]*\)\.[0-9]*\..*,\1,g'`
  echo "cl version: $CL_VER"
  if [ -n "`echo $CL_INFO |grep -i x86`" ]; then
    echo "vc x86"
    test $CL_VER -gt 16 && echo "adding windows xp compatible link flags..." && PLATFORM_OPT="$PLATFORM_OPT --extra-ldflags=\"-SUBSYSTEM:CONSOLE,5.01\""
  elif [ -n "`echo $CL_INFO |grep -i x64`" ]; then
    echo "vc x64"
    test $CL_VER -gt 16 && echo "adding windows xp compatible link flags..." && PLATFORM_OPT="$PLATFORM_OPT --extra-ldflags=\"-SUBSYSTEM:CONSOLE,5.02\""
  elif [ -n "`echo $CL_INFO |grep -i arm`" ]; then
    echo "vc arm"
    # http://www.cnblogs.com/zjjcy/p/3384517.html  http://www.cnblogs.com/zjjcy/p/3499848.html
    # armasm: http://www.cnblogs.com/zcmmwbd/p/windows-phone-8-armasm-guide.html#2842650
    # TODO: use a wrapper function to deal with the parameters passed to armasm
    PLATFORM_OPT="--extra-cflags=\"-D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE -D_M_ARM -DWINAPI_FAMILY=WINAPI_FAMILY_APP\" --extra-ldflags=\"-MACHINE:ARM\" $PLATFORM_OPT --enable-cross-compile --arch=arm --cpu=armv7 --target-os=win32 --as=armasm --disable-yasm --disable-inline-asm"
  fi
}

setup_icc_env() {
  #TOOLCHAIN_OPT=
  PLATFORM_OPT="--toolchain=icl"
}

setup_wince_env() {
  WINCEOPT="--enable-cross-compile --cross-prefix=arm-mingw32ce- --target-os=mingw32ce --arch=arm --cpu=arm"
  PLATFORM_OPT="$WINCEOPT"
  MISC_OPT=
  INSTALL_DIR=sdk-wince
}

setup_android_env() {
  ANDROID_ARCH=$1
  test -n "$ANDROID_ARCH" || ANDROID_ARCH=arm
  ANDROID_TOOLCHAIN_PREFIX="arm-linux-androideabi"
  CROSS_PREFIX=arm-linux-androideabi-
  FFARCH=$ANDROID_ARCH
  if [ "$ANDROID_ARCH" = "x86" ]; then
    #ANDROID_ARCH="i686"
    ANDROID_TOOLCHAIN_PREFIX="x86"
    CROSS_PREFIX=i686-linux-android-
  elif [ "$ANDROID_ARCH" = "mipsel" ]; then
    ANDROID_TOOLCHAIN_PREFIX="mipsel-linux-android"
    CROSS_PREFIX=mipsel-linux-android-
  else
    FFARCH=armv7-a
  fi
  ANDROID_TOOLCHAIN_DIR="/tmp/ndk-toolchain-${ANDROID_ARCH}" #$NDK_ROOT/toolchains/arm-linux-androideabi-4.7/prebuilt/linux-x86_64/bin
  echo "ANDROID_TOOLCHAIN_DIR=${ANDROID_TOOLCHAIN_DIR}"
  ANDROID_SYSROOT="$ANDROID_TOOLCHAIN_DIR/sysroot" #"$NDK_ROOT/platforms/android-14/arch-arm"
# --enable-libstagefright-h264
  ANDROIDOPT="--enable-cross-compile --cross-prefix=$CROSS_PREFIX --sysroot=$ANDROID_SYSROOT --target-os=android --arch=${FFARCH}"
  test -d $ANDROID_TOOLCHAIN_DIR || $NDK_ROOT/build/tools/make-standalone-toolchain.sh --platform=android-19 --toolchain=${ANDROID_TOOLCHAIN_PREFIX}-4.8 --install-dir=$ANDROID_TOOLCHAIN_DIR #--system=linux-x86_64
  export PATH=$ANDROID_TOOLCHAIN_DIR/bin:$PATH
  rm -rf $ANDROID_SYSROOT/usr/include/{libsw*,libav*}
  rm -rf $ANDROID_SYSROOT/usr/lib/{libsw*,libav*}
:<<RM
  CONFIGURE=configure-android
  cat configure |sed s,SLIBNAME_WITH_MAJOR=\'$\(SLIBNAME\)\.$\(LIBMAJOR\)\',SLIBNAME_WITH_MAJOR=\'$\(SLIBPREF\)$\(FULLNAME\)\-$\(LIBMAJOR\)$\(SLIBSUF\)\', |sed s,SLIB_INSTALL_NAME=\'\$\(SLIBNAME_WITH_VERSION\)\',SLIB_INSTALL_NAME=\'\$\(SLIBNAME_WITH_MAJOR\)\', |sed s,SLIB_INSTALL_LINKS=\'\$\(SLIBNAME_WITH_MAJOR\)\ \$\(SLIBNAME\)\',SLIB_INSTALL_LINKS=\'\$\(SLIBNAME\)\', >$CONFIGURE
  chmod +x $CONFIGURE
RM
  MISC_OPT=--disable-avdevice
  PLATFORM_OPT="$ANDROIDOPT"
  EXTRA_CFLAGS="$ANDROID_CFLAGS $ANDROID_CLFAGS_ARMV7"
  INSTALL_DIR=sdk-android-$ANDROID_ARCH
}

setup_ios_env() {
#iphoneos iphonesimulator i386
  PLATFORM_OPT='--enable-cross-compile --arch=arm --target-os=darwin --cc="clang -arch armv7" --sysroot=$(xcrun --sdk iphoneos --show-sdk-path) --cpu=cortex-a8 --enable-pic'
  LIB_OPT="--enable-static"
  MISC_OPT=--disable-avdevice
  INSTALL_DIR=sdk-ios
}

setup_ios_simulator_env() {
#iphoneos iphonesimulator i386
  PLATFORM_OPT='--enable-cross-compile --arch=i386 --cpu=i386 --target-os=darwin --cc="clang -arch i386" --sysroot=$(xcrun --sdk iphonesimulator --show-sdk-path) --enable-pic'
  LIB_OPT="--enable-static"
  MISC_OPT=--disable-avdevice
  INSTALL_DIR=sdk-ios-i386
}

setup_maemo5_env() {
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
  MISC_OPT=--disable-avdevice
  INSTALL_DIR=sdk-maemo5
}
setup_maemo6_env() {
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
  MISC_OPT=--disable-avdevice
  INSTALL_DIR=sdk-maemo6
}

if target_is android; then
  setup_android_env $TAGET_ARCH_FLAG
elif target_is ios; then
  setup_ios_env
elif target_is ios_simulator; then
  setup_ios_simulator_env
elif target_is maemo5; then
  setup_maemo5_env
elif target_is maemo6; then
  setup_maemo6_env
elif target_is x86; then
  if [ "`uname -m`" = "x86_64" ]; then
    #TOOLCHAIN_OPT="$TOOLCHAIN_OPT --enable-cross-compile --target-os=$(tolower $(uname -s)) --arch=x86"
    ARCH_FLAGS=-m32
    INSTALL_DIR=sdk-x86
  fi
else
  if platform_is Sailfish; then
    echo "Build in Sailfish SDK"
    MISC_OPT=--disable-avdevice
    INSTALL_DIR=sdk-sailfish
  elif platform_is Linux; then
    test -n "$vaapi_opt" && PLATFORM_OPT="$PLATFORM_OPT $vaapi_opt"
    test -n "$vdpau_opt" && PLATFORM_OPT="$PLATFORM_OPT $vdpau_opt"
  elif platform_is Darwin; then
    test -n "$vda_opt" && PLATFORM_OPT="$PLATFORM_OPT $vda_opt"
    EXTRA_CFLAGS=-mmacosx-version-min=10.6
  fi
fi

if target_is vc; then
  setup_vc_env
else
  TOOLCHAIN_OPT="$TOOLCHAIN_OPT --extra-cflags=\"$ARCH_FLAGS -O3 $CLANG_CFLAGS $EXTRA_CFLAGS\""
  platform_is MinGW || platform_is MSYS && {
    test -n "$dxva2_opt" && PLATFORM_OPT="$PLATFORM_OPT $dxva2_opt"
    TOOLCHAIN_OPT="$dxva2_opt --disable-iconv $TOOLCHAIN_OPT --extra-ldflags=\"-static-libgcc -Wl,-Bstatic\""
  }
  test -n "$ARCH_FLAGS" && platform_is Linux && TOOLCHAIN_OPT="$TOOLCHAIN_OPT --extra-ldflags=\"-m32\""
fi
test -n "$EXTRALIBS" && TOOLCHAIN_OPT="$TOOLCHAIN_OPT --extra-libs=\"$EXTRALIBS\""
echo $LIB_OPT
is_libav || MISC_OPT="$MISC_OPT --disable-postproc"
CONFIGURE="./$CONFIGURE --extra-version=QtAV $LIB_OPT --enable-pic --enable-runtime-cpudetect $MISC_OPT --disable-muxers --disable-encoders $PLATFORM_OPT $TOOLCHAIN_OPT"
CONFIGURE=`echo $CONFIGURE |tr -s ' '`
# http://ffmpeg.org/platform.html
# static: --enable-pic --extra-ldflags="-Wl,-Bsymbolic" --extra-ldexeflags="-pie"
# ios: https://github.com/FFmpeg/gas-preprocessor

echo $CONFIGURE

time eval $CONFIGURE
if [ $? -eq 0 ]; then
  time (make -j8 install prefix="$PWD/$INSTALL_DIR")
fi

# --enable-pic is default  --enable-lto
# http://cmzx3444.iteye.com/blog/1447366
# --enable-openssl  --enable-hardcoded-tables  --enable-librtmp --enable-zlib
