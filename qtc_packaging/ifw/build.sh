
# QtAV installer creator.

[ $# -lt 1 ] && {
  echo "need QtAV build dir as a parameter"
  exit 1
}


BUILD=$1
ARCH=`grep TARGET_ARCH $BUILD/.qmake.cache |grep -v TARGET_ARCH_SUB |cut -d "=" -f 2`
ARCH=`echo $ARCH` #trim
echo "$ARCH"
MKSPEC=`grep mkspecs_cached $BUILD/.qmake.cache |cut -d "=" -f 2`
MKSPEC=`echo $MKSPEC`

TARGET=${MKSPEC}-${ARCH}
QTAV_VER=`grep PACKAGE_VERSION ../../QtAV.pro |cut -d "=" -f 2`
QTAV_VER=`echo $QTAV_VER`

function platform_is() {
  local name=$1
#TODO: osx=>darwin
  local line=`uname -a |grep -i $name`
  test -n "$line" && return 0 || return 1
}

lib_name() {
  local lib_base=$1
  local lib_ver=$2
    if platform_is Darwin; then
      echo lib${lib_base}.${lib_ver}.dylib
    elif platform_is MinGW; then
      echo ${lib_base}${lib_ver}.dll
    elif platform_is MSYS; then
      echo ${lib_base}${lib_ver}.dll
    else
      echo lib${lib_base}.so.${lib_ver}
    fi
}
qt5lib_name() {
  local m=$1
    if platform_is Darwin; then
      echo libQt5${m}.${lib_ver}.dylib
    elif platform_is MinGW || platform_is MSYS; then
      echo Qt5${m}.dll
    else
      echo libQt5${m}.so.5
    fi
}

EXE=
platform_is MinGW || platform_is MSYS && EXE=.exe

QTDIR=$(grep include $BUILD/sdk_uninstall.* |head -n 1 | sed 's,\\,\/,g')
QTDIR=${QTDIR%include*}
QTDIR=${QTDIR//* /}
echo "QTDIR=$QTDIR"

LIBDIR=`find $BUILD/lib* -name "*Qt*AV.prl"`
LIBDIR=${LIBDIR%/*}
mkdir -p $TARGET
echo "LIBDIR=$LIBDIR"

echo "creating directories..."
cp -af packages $TARGET
mkdir -p $TARGET/packages/com.qtav.product.runtime/data/bin/{plugins,qml}
mkdir -p $TARGET/packages/com.qtav.product.player/data/bin
mkdir -p $TARGET/packages/com.qtav.product.examples/data/bin
mkdir -p $TARGET/packages/com.qtav.product.dev/data/include
mkdir -p $TARGET/packages/com.qtav.product.dev/data/lib
mkdir -p $TARGET/packages/com.qtav.product/data/


# runtime
echo "coping runtime files..."
echo "[Paths]" > $TARGET/packages/com.qtav.product.runtime/data/bin/qt.conf
echo "Prefix=." >> $TARGET/packages/com.qtav.product.runtime/data/bin/qt.conf
QTMODULES=(Core Gui OpenGL Widgets Qml Quick Network Svg)
platform_is Linux && QTMODULES+=(DBus)
cp -af $BUILD/bin/* $TARGET/packages/com.qtav.product.runtime/data/bin
for m in ${QTMODULES[@]}; do
  test -f $TARGET/packages/com.qtav.product.runtime/data/bin/`qt5lib_name ${m}` || cp -Lf $QTDIR/lib/`qt5lib_name ${m}` $TARGET/packages/com.qtav.product.runtime/data/bin
done
cp -af $QTDIR/plugins/{imageformats,platform*} $TARGET/packages/com.qtav.product.runtime/data/bin/plugins
cp -af $QTDIR/qml/{Qt,QtQml,QtQuick,QtQuick.2} $TARGET/packages/com.qtav.product.runtime/data/bin/qml
##ffmpeg
## QtAV, Qt5AV
LIBQTAV=$LIBDIR/`lib_name "Qt*AV" 1`
echo "LIBQTAV=$LIBQTAV"
cp -Lf $LIBQTAV $TARGET/packages/com.qtav.product.runtime/data/bin

# dev
echo "coping development files..."
cp -af ../../src/QtAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af ../../qml/QmlAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af ../../{README.md,lgpl-2.1.txt,gpl-3.0.txt,doc} $TARGET/packages/com.qtav.product/data

#mingw: libQt5AV1.a
cp -af $LIBDIR/*Qt*AV* $TARGET/packages/com.qtav.product.dev/data/lib



# player
echo "coping player files..."

mv $TARGET/packages/com.qtav.product.runtime/data/bin/{player${EXE},QMLPlayer${EXE}} $TARGET/packages/com.qtav.product.player/data/bin

LIBCOMMON=$TARGET/packages/com.qtav.product.runtime/data/bin/`lib_name common 1`
if [ -f $LIBCOMMON ]; then
    echo "moving $LIBCOMMON ..."
  mv $LIBCOMMON $TARGET/packages/com.qtav.product.player/data/bin
else
  LIBCOMMON=$LIBDIR/`lib_name common 1`
    echo "coping $LIBCOMMON ..."
  cp -Lf $LIBCOMMON $TARGET/packages/com.qtav.product.player/data/bin
fi

#examples
echo "coping examples..."
mv `find $TARGET/packages/com.qtav.product.runtime/data/bin/* -maxdepth 0 -type f |grep -v "\.so" |grep -v "\.dylib" |grep -v "\.conf" |grep -v "\.dll"` $TARGET/packages/com.qtav.product.examples/data/bin


find $TARGET -name log.txt -exec rm -f {} \;
find $TARGET -name "*.manifest" -exec rm -f {} \;


echo "creating installer..."
if platform_is MinGW || platform_is MSYS; then
  echo "default install dir is 'C:\QtAV'"
  cp config/config.xml $TARGET
else
  echo "default install dir is '$HOME/QtAV'"
  sed 's,rootDir,homeDir,g' config/config.xml >$TARGET/config.xml
fi

INSTALLER=QtAV${QTAV_VER}-${TARGET}
binarycreator -c $TARGET/config.xml -p $TARGET/packages --ignore-translations -v $INSTALLER
du -h $INSTALLER
