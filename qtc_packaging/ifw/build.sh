
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
QTAV_VER_MAJOR=`grep -m 1 QTAV_MAJOR_VERSION ../../.qmake.conf |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER_MINOR=`grep -m 1 QTAV_MINOR_VERSION ../../.qmake.conf |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER_PATCH=`grep -m 1 QTAV_PATCH_VERSION ../../.qmake.conf |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER=${QTAV_VER_MAJOR}.${QTAV_VER_MINOR}.${QTAV_VER_PATCH}
echo "QtAV $QTAV_VER"

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

# TODO: QT_INSTALL_LIBS. .qmake.cache stores QT_INSTALL_BINS, run qmake here
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
rm -f $TARGET/packages/com.qtav.product.runtime/data/bin/plugins/platforms/{*mini*,*offscreen*,*eglfs*,*linuxfb*,*kms*}
rm -f $TARGET/packages/com.qtav.product.runtime/data/bin/plugins/imageformats/{*dds*,*icns*,*tga*,*tiff*,*wbmp*,*webp*}
##ffmpegs
## QtAV, Qt5AV
LIBQTAV=$LIBDIR/`lib_name "Qt*AV" 1`
echo "LIBQTAV=$LIBQTAV"
cp -Lf $LIBQTAV $TARGET/packages/com.qtav.product.runtime/data/bin
LIBQTAVWIDGETS=$LIBDIR/`lib_name "Qt*AVWidgets" 1`
echo "LIBQTAVWIDGETS=$LIBQTAVWIDGETS"
cp -Lf $LIBQTAVWIDGETS $TARGET/packages/com.qtav.product.runtime/data/bin

# dev
echo "coping development files..."
cp -af ../../src/QtAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af ../../widgets/QtAVWidgets $TARGET/packages/com.qtav.product.dev/data/include
#cp -af ../../qml/QmlAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af $BUILD/tools/install_sdk/mkspecs $TARGET/packages/com.qtav.product.dev/data
cp -af ../../{README.md,lgpl-2.1.txt,gpl-3.0.txt,doc} $TARGET/packages/com.qtav.product/data

#mingw: libQt5AV1.a
cp -af $LIBDIR/*Qt*AV* $TARGET/packages/com.qtav.product.dev/data/lib
[ -f $LIBDIR/Qt5AV1.lib ] && cp -af $LIBDIR/libQt5AV1.lib $TARGET/packages/com.qtav.product.dev/data/lib/Qt5AV.lib
[ -f $LIBDIR/libQt5AV1.a ] && cp -af $LIBDIR/libQt5AV1.a $TARGET/packages/com.qtav.product.dev/data/lib/libQt5AV.a
[ -f $LIBDIR/libQt5AV1.so ] && cp -af $LIBDIR/libQt5AV1.so $TARGET/packages/com.qtav.product.dev/data/lib/libQt5AV.so
[ -f $LIBDIR/Qt5AVWidgets1.lib ] && cp -af $LIBDIR/libQt5AVWidgets1.lib $TARGET/packages/com.qtav.product.dev/data/lib/Qt5AVWidgets.lib
[ -f $LIBDIR/libQt5AVWidgets1.a ] && cp -af $LIBDIR/libQt5AVWidgets1.a $TARGET/packages/com.qtav.product.dev/data/lib/libQt5AVWidgets.a
[ -f $LIBDIR/libQt5AVWidgets1.so ] && cp -af $LIBDIR/libQt5AVWidgets1.so $TARGET/packages/com.qtav.product.dev/data/lib/libQt5AVWidgets.so
rm -f $TARGET/packages/com.qtav.product.dev/data/lib/{*.dll,*.so.*}

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
EXAMPLE_DIR=$TARGET/packages/com.qtav.product.examples
mv `find $TARGET/packages/com.qtav.product.runtime/data/bin/* -maxdepth 0 -type f |grep -v "\.so" |grep -v "\.dylib" |grep -v "\.conf" |grep -v "\.dll"` $EXAMPLE_DIR/data/bin


find $TARGET -name log.txt -exec rm -f {} \;
find $TARGET -name "*.manifest" -exec rm -f {} \;
# copy simpleplayer source code
mkdir -p $EXAMPLE_DIR/data/src
cp -af ../../examples/simpleplayer $EXAMPLE_DIR/data/src
cat >$EXAMPLE_DIR/data/src/simpleplayer/simpleplayer.pro<<EOF
TEMPLATE = app
CONFIG -= app_bundle
QT += avwidgets av
HEADERS = playerwindow.h
SOURCES = playerwindow.cpp main.cpp
EOF

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
