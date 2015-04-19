# QtAV installer creator.

[ $# -lt 1 ] && {
  echo "need QtAV build dir as a parameter"
  exit 1
}


BUILD=$1
QTBIN=`grep QT_BIN $BUILD/.qmake.cache |head -n 1 |cut -d "=" -f 2 | tr -d ' '` 
# for windows
QTBIN=`echo /$QTBIN|sed 's,:,,'`
export PATH=$QTBIN:$PATH
echo "QTBIN=$QTBIN"
ARCH=`grep TARGET_ARCH $BUILD/.qmake.cache |grep -v TARGET_ARCH_SUB |cut -d "=" -f 2`
ARCH=`echo $ARCH` #trim
echo "$ARCH"
MKSPEC=`grep mkspecs_cached $BUILD/.qmake.cache |cut -d "=" -f 2`
MKSPEC=`echo $MKSPEC`

TARGET=${MKSPEC}-${ARCH}
mkdir -p $TARGET
# grep -m 1 is not supported for msys
QTAV_VER_MAJOR=`grep QTAV_MAJOR_VERSION ../../.qmake.conf |head -n 1 |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER_MINOR=`grep QTAV_MINOR_VERSION ../../.qmake.conf |head -n 1 |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER_PATCH=`grep QTAV_PATCH_VERSION ../../.qmake.conf |head -n 1 |cut -d "=" -f 2 | tr -d ' '`
QTAV_VER=${QTAV_VER_MAJOR}.${QTAV_VER_MINOR}.${QTAV_VER_PATCH}
echo "QtAV $QTAV_VER"

host_is() {
  local name=$1
#TODO: osx=>darwin
  local line=`uname -a |grep -i $name`
  test -n "$line" && return 0 || return 1
}

lib_name() {
  local lib_base=$1
  local lib_ver=$2
    if host_is Darwin; then
      echo lib${lib_base}.${lib_ver}.dylib
    elif host_is MinGW; then
      echo ${lib_base}${lib_ver}.dll
    elif host_is MSYS; then
      echo ${lib_base}${lib_ver}.dll
    else
      echo lib${lib_base}.so.${lib_ver}
    fi
}
qt5lib_name() {
  local m=$1
    if host_is Darwin; then
      echo libQt5${m}.${lib_ver}.dylib
    elif host_is MinGW || host_is MSYS; then
      echo Qt5${m}.dll
    else
      echo libQt5${m}.so.5
    fi
}

EXE=
host_is MinGW || host_is MSYS && EXE=.exe


LIBDIR=`find $BUILD/lib* -name "*Qt*AV.prl"`
LIBDIR=${LIBDIR%/*}
echo "LIBDIR=$LIBDIR"

echo "creating directories..."
cp -af packages $TARGET
mkdir -p $TARGET/packages/com.qtav.product.runtime/data/bin/{plugins,qml}
mkdir -p $TARGET/packages/com.qtav.product.player/data/bin
mkdir -p $TARGET/packages/com.qtav.product.examples/data/bin
mkdir -p $TARGET/packages/com.qtav.product.dev/data/include
mkdir -p $TARGET/packages/com.qtav.product.dev/data/lib
mkdir -p $TARGET/packages/com.qtav.product/data/


### runtime
echo "coping runtime files..."
RT_DIR=$TARGET/packages/com.qtav.product.runtime
cat > $RT_DIR/data/bin/qt.conf <<EOF
[Paths]
Prefix=.
EOF
declare -a QTMODULES=(Core Gui OpenGL Widgets Qml Quick Network Svg) #TODO: use readelf, objdump or otool to get depends
host_is Linux && QTMODULES+=(DBus)
cp -af $BUILD/bin/* $RT_DIR/data/bin
QTRT=`qmake -query QT_INSTALL_LIBS`
host_is MinGW || host_is MSYS && QTRT=`$QTBIN/qmake -query QT_INSTALL_BINS`
for m in ${QTMODULES[@]}; do
  RT_DLL=`qt5lib_name ${m}`
  test -L $RT_DLL/data/bin/$RT_DLL && rm -rf $RT_DLL/data/bin/$RT_DLL
  test -f $RT_DLL/data/bin/$RT_DLL || cp -Lf $QTRT/$RT_DLL $RT_DIR/data/bin
done

QTPLUGIN=`qmake -query QT_INSTALL_PLUGINS`
QTQML=`qmake -query QT_INSTALL_QML`
cp -af $QTPLUGIN/{imageformats,platform*} $RT_DIR/data/bin/plugins
cp -af $QTQML/{Qt,QtQml,QtQuick,QtQuick.2} $RT_DIR/data/bin/qml
rm -f $RT_DIR/data/bin/plugins/platforms/{*mini*,*offscreen*,*eglfs*,*linuxfb*,*kms*}
rm -f $RT_DIR/data/bin/plugins/imageformats/{*dds*,*icns*,*tga*,*tiff*,*wbmp*,*webp*}
##ffmpegs
## QtAV, Qt5AV
LIBQTAV=$LIBDIR/`lib_name "Qt*AV" 1`
echo "LIBQTAV=$LIBQTAV"
cp -Lf $LIBQTAV $RT_DIR/data/bin
LIBQTAVWIDGETS=$LIBDIR/`lib_name "Qt*AVWidgets" 1`
echo "LIBQTAVWIDGETS=$LIBQTAVWIDGETS"
cp -Lf $LIBQTAVWIDGETS $RT_DIR/data/bin

# delete prl to avoid link error for sdk user
find $RT_DIR -name "QtAV*.prl" -exec rm -f {} \;


### dev
echo "coping development files..."
cp -af ../../src/QtAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af ../../widgets/QtAVWidgets $TARGET/packages/com.qtav.product.dev/data/include
#cp -af ../../qml/QmlAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af $BUILD/tools/install_sdk/mkspecs $TARGET/packages/com.qtav.product.dev/data
cp -af ../../{README.md,lgpl-2.1.txt,gpl-3.0.txt,doc} $TARGET/packages/com.qtav.product/data

# copy qml files for simplify deployment sdk
cd $TARGET/packages/com.qtav.product.dev/data
mkdir -p qml
if [ -d $RT_DIR/data/bin/QtAV ]; then
  cp -af $RT_DIR/data/bin/QtAV qml
elif  [ -d $BUILD/bin/QtAV ]; then
  cp -af $BUILD/bin/QtAV qml
elif [ -d $RT_DIR/data/bin/qml/QtAV ]; then
  cp -af $RT_DIR/data/bin/qml/QtAV qml
fi
cd -

#mingw: libQt5AV1.a
cd $TARGET/packages/com.qtav.product.dev/data/lib
cp -af $LIBDIR/*Qt*AV* .
[ -f $LIBDIR/Qt5AV${QTAV_VER_MAJOR}.lib ] && cp -af $LIBDIR/libQt5AV${QTAV_VER_MAJOR}.lib Qt5AV.lib
[ -f $LIBDIR/libQt5AV${QTAV_VER_MAJOR}.a ] && cp -af $LIBDIR/libQt5AV${QTAV_VER_MAJOR}.a libQt5AV.a
[ -f $LIBDIR/Qt5AVWidgets${QTAV_VER_MAJOR}.lib ] && cp -af $LIBDIR/libQt5AVWidgets${QTAV_VER_MAJOR}.lib Qt5AVWidgets.lib
[ -f $LIBDIR/libQt5AVWidgets${QTAV_VER_MAJOR}.a ] && cp -af $LIBDIR/libQt5AVWidgets${QTAV_VER_MAJOR}.a libQt5AVWidgets.a
rm -f {*.dll,*.so.*,*.prl}
# no copy, just link to real file in bin for linux development
if [ -f $LIBDIR/libQt5AV.so ]; then
  ln -sf ../bin/libQt5AV.so.${QTAV_VER_MAJOR} libQt5AV.so
  ln -sf ../bin/libQt5AVWidgets.so.${QTAV_VER_MAJOR} libQt5AVWidgets.so  
elif [ -f $LIBDIR/libQtAV.so ]; then
  ln -sf ../bin/libQtAV.so.${QTAV_VER_MAJOR} libQtAV.so
  ln -sf ../bin/libQtAVWidgets.so.${QTAV_VER_MAJOR} libQtAVWidgets.so
fi
cd -

### player
echo "coping player files..."

mv $RT_DIR/data/bin/{player${EXE},QMLPlayer${EXE}} $TARGET/packages/com.qtav.product.player/data/bin

LIBCOMMON=$RT_DIR/data/bin/`lib_name common 1`
if [ -f $LIBCOMMON ]; then
    echo "moving $LIBCOMMON ..."
  mv $LIBCOMMON $TARGET/packages/com.qtav.product.player/data/bin
else
  LIBCOMMON=$LIBDIR/`lib_name common 1`
    echo "coping $LIBCOMMON ..."
  cp -Lf $LIBCOMMON $TARGET/packages/com.qtav.product.player/data/bin
fi

### examples
echo "coping examples..."
EXAMPLE_DIR=$TARGET/packages/com.qtav.product.examples
mv `find $RT_DIR/data/bin/* -maxdepth 0 -type f |grep -v "\.so" |grep -v "\.dylib" |grep -v "\.conf" |grep -v "\.dll"` $EXAMPLE_DIR/data/bin


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
if host_is MinGW || host_is MSYS; then
  echo "default install dir is 'C:\QtAV'"
  cp config/config.xml $TARGET
else
  echo "default install dir is '$HOME/QtAV'"
  sed 's,rootDir,homeDir,g' config/config.xml >$TARGET/config.xml
fi
cp config/control.js $TARGET

type -p binarycreator || {
  echo "Can not create installer. Make sure Qt Installer Framework tools can be found in \$PATH"
  exit 0
}
INSTALLER=QtAV${QTAV_VER}-${TARGET}
binarycreator -c $TARGET/config.xml -p $TARGET/packages --ignore-translations -v $INSTALLER
du -h $INSTALLER
