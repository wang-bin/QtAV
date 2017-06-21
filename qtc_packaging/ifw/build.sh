# QtAV installer creator.

[ $# -lt 1 ] && {
  echo "need QtAV build dir as a parameter"
  exit 1
}


BUILD=$1
QTBIN=`grep QT_BIN $BUILD/.qmake.cache |head -n 1 |cut -d "=" -f 2 | tr -d ' '`
# for windows
QTBIN=`echo /$QTBIN|sed 's,:,,'`
export PATH=$QTBIN:$PWD/../../tools:$PATH
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
libd_name() {
  local lib_base=$1
  local lib_ver=$2
    if host_is Darwin; then
      echo lib${lib_base}_debug.${lib_ver}.dylib
    elif host_is MinGW; then
      echo ${lib_base}d${lib_ver}.dll
    elif host_is MSYS; then
      echo ${lib_base}d${lib_ver}.dll
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
mkdir -p $TARGET/packages/com.qtav.product.dev/data/bin/QtAV
mkdir -p $TARGET/packages/com.qtav.product.dev/data/include
mkdir -p $TARGET/packages/com.qtav.product.dev/data/lib
mkdir -p $TARGET/packages/com.qtav.product/data/


### runtime
echo "coping runtime files..."
RT_DIR=$TARGET/packages/com.qtav.product.runtime
declare -a QTMODULES=(Core Gui OpenGL Widgets Qml Quick Network Svg Sql) #TODO: use readelf, objdump or otool to get depends
host_is Linux && QTMODULES+=(DBus XcbQpa X11Extras)
cp -af $BUILD/bin/* $RT_DIR/data/bin
rm -rf $RT_DIR/data/bin/QtAV*d${QTAV_VER_MAJOR}.*
cat > $RT_DIR/data/bin/qt.conf <<EOF
[Paths]
Prefix=.
EOF

QTRT=`qmake -query QT_INSTALL_LIBS`
host_is MinGW || host_is MSYS && {
  QTRT=`$QTBIN/qmake -query QT_INSTALL_BINS`
  test -f $QTRT/libEGL.dll && cp -af $QTRT/libEGL.dll $RT_DIR/data/bin
  test -f $QTRT/libGLESv2.dll && cp -af $QTRT/libGLESv2.dll $RT_DIR/data/bin
  test -f $QTRT/d3dcompiler_47.dll && cp -af $QTRT/d3dcompiler_47.dll $RT_DIR/data/bin
}

for m in ${QTMODULES[@]}; do
  RT_DLL=`qt5lib_name ${m}`
  test -L $RT_DLL/data/bin/$RT_DLL && rm -rf $RT_DLL/data/bin/$RT_DLL
  test -f $RT_DLL/data/bin/$RT_DLL || cp -Lf $QTRT/$RT_DLL $RT_DIR/data/bin
done

QTPLUGIN=`qmake -query QT_INSTALL_PLUGINS`
QTQML=`qmake -query QT_INSTALL_QML`
for qplugin in imageformats platforms platforminputcontexts platformthemes xcbglintegrations iconengines egldeviceintegrations generic sqldrivers; do
  test -d $QTPLUGIN/$qplugin && cp -af $QTPLUGIN/$qplugin $RT_DIR/data/bin/plugins
done
# find dir -name "*d.dll" -o -name "*.pdb" -delete fail, why?
cp -af $QTQML/{Qt,QtQml,QtQuick,QtQuick.2} $RT_DIR/data/bin/qml
rm -f $RT_DIR/data/bin/plugins/platforms/{*mini*,*offscreen*,*eglfs*,*linuxfb*,*kms*}
rm -f $RT_DIR/data/bin/plugins/imageformats/{*dds*,*icns*,*tga*,*tiff*,*wbmp*,*webp*}
find $RT_DIR/data/bin/plugins -name "*.pdb" -delete
find $RT_DIR/data/bin/qml -name "*.pdb" -delete
find $RT_DIR/data/bin/qml -name "*plugind.dll" -delete
find $RT_DIR/data/bin/plugins -name "*d.dll" -delete #qml: *plugind.dll
find $RT_DIR/data/bin -name "*.exp" -delete
find $RT_DIR/data/bin -name "*.lib" -delete

##ffmpegs
## QtAV, Qt5AV. TODO: debug
LIBQTAV=$LIBDIR/`lib_name "Qt*AV" ${QTAV_VER_MAJOR}`
echo "LIBQTAV=$LIBQTAV"
cp -Lf $LIBQTAV $RT_DIR/data/bin
LIBQTAVWIDGETS=$LIBDIR/`lib_name "Qt*AVWidgets" ${QTAV_VER_MAJOR}`
echo "LIBQTAVWIDGETS=$LIBQTAVWIDGETS"
cp -Lf $LIBQTAVWIDGETS $RT_DIR/data/bin
ls $RT_DIR/data/bin/QtAV*
# delete prl to avoid link error for sdk user
find $RT_DIR -name "QtAV*.prl" -exec rm -f {} \;


### dev
echo "coping development files..."
cp -aLf $LIBDIR/../bin/QtAV*d${QTAV_VER_MAJOR}.* $TARGET/packages/com.qtav.product.dev/data/bin
cp -af ../../src/QtAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af ../../widgets/QtAVWidgets $TARGET/packages/com.qtav.product.dev/data/include
#cp -af ../../qml/QmlAV $TARGET/packages/com.qtav.product.dev/data/include
cp -af $BUILD/tools/install_sdk/mkspecs $TARGET/packages/com.qtav.product.dev/data
cp -af ../../{README.md,lgpl-2.1.txt,doc} $TARGET/packages/com.qtav.product/data

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
rm -rf $RT_DIR/data/bin/QtAV/QmlAVd*

pwd
echo "coping libs..."
#mingw: libQt5AV1.a
cd $TARGET/packages/com.qtav.product.dev/data/lib
pwd
cp -af $LIBDIR/*Qt*AV* .
[ -f Qt5AV${QTAV_VER_MAJOR}.lib ] && cp -af Qt5AV${QTAV_VER_MAJOR}.lib Qt5AV.lib
[ -f libQt5AV${QTAV_VER_MAJOR}.a ] && ln -sf libQt5AV${QTAV_VER_MAJOR}.a libQt5AV.a
[ -f Qt5AVWidgets${QTAV_VER_MAJOR}.lib ] && cp -af Qt5AVWidgets${QTAV_VER_MAJOR}.lib Qt5AVWidgets.lib
[ -f libQt5AVWidgets${QTAV_VER_MAJOR}.a ] && ln -sf libQt5AVWidgets${QTAV_VER_MAJOR}.a libQt5AVWidgets.a
[ -f QtAV${QTAV_VER_MAJOR}.lib ] && cp -af QtAV${QTAV_VER_MAJOR}.lib Qt5AV.lib
[ -f libQtAV${QTAV_VER_MAJOR}.a ] && ln -sf libQtAV${QTAV_VER_MAJOR}.a libQt5AV.a
[ -f QtAVWidgets${QTAV_VER_MAJOR}.lib ] && cp -af QtAVWidgets${QTAV_VER_MAJOR}.lib Qt5AVWidgets.lib
[ -f libQtAVWidgets${QTAV_VER_MAJOR}.a ] && ln -sf libQtAVWidgets${QTAV_VER_MAJOR}.a libQt5AVWidgets.a
[ -f QtAVd${QTAV_VER_MAJOR}.lib ] && cp -af QtAVd${QTAV_VER_MAJOR}.lib Qt5AVd.lib
[ -f libQtAVd${QTAV_VER_MAJOR}.a ] && ln -sf libQtAVd${QTAV_VER_MAJOR}.a libQt5AVd.a
[ -f QtAVWidgetsd${QTAV_VER_MAJOR}.lib ] && cp -af QtAVWidgetsd${QTAV_VER_MAJOR}.lib Qt5AVWidgetsd.lib
[ -f libQtAVWidgetsd${QTAV_VER_MAJOR}.a ] && ln -sf libQtAVWidgetsd${QTAV_VER_MAJOR}.a libQt5AVWidgetsd.a
rm -f {*.dll,*.prl,*.exp}
[ -f libQtAV.so ] && ln -sf libQtAV.so libQt5AV.so
[ -f libQtAVWidgets.so ] && ln -sf libQtAVWidgets.so libQt5AVWidgets.so
cd -

# link runtime qtav and overwrite the old so
cd $RT_DIR/data/bin
if host_is Linux; then
echo "linking qtav runtime so..."
  ln -sf ../lib/libQt5AV.so.${QTAV_VER_MAJOR} .
  ln -sf ../lib/libQt5AVWidgets.so.${QTAV_VER_MAJOR} .
  ln -sf ../lib/libQtAV.so.${QTAV_VER_MAJOR} .
  ln -sf ../lib/libQtAVWidgets.so.${QTAV_VER_MAJOR} .
fi
cd -

### player
echo "coping player files..."
mv $RT_DIR/data/bin/{Player${EXE},QMLPlayer${EXE}} $TARGET/packages/com.qtav.product.player/data/bin

### examples
echo "coping examples..."
EXAMPLE_DIR=$TARGET/packages/com.qtav.product.examples
mv `find $RT_DIR/data/bin/* -maxdepth 0 -type f |grep -v "\.so" |grep -v "\.dylib" |grep -v "\.conf" |grep -v "\.dll" |grep -v "\.pdb"` $EXAMPLE_DIR/data/bin

if host_is Linux; then
echo "patch elf runpath..."
  patchelf.sh $TARGET/packages/com.qtav.product.runtime/data/bin
  patchelf.sh $TARGET/packages/com.qtav.product.player/data/bin
  patchelf.sh $TARGET/packages/com.qtav.product.examples/data/bin
fi

find $TARGET -name "log*.txt" -delete
find $TARGET -name "*.manifest" -delete

# copy simpleplayer source code
mkdir -p $EXAMPLE_DIR/data/src
cp -af ../../examples/simpleplayer $EXAMPLE_DIR/data/src
[ -f $EXAMPLE_DIR/data/src/simpleplayer/simpleplayer_sdk.pro ] && rm -rf $EXAMPLE_DIR/data/src/simpleplayer/simpleplayer.pro

echo "creating installer..."
if host_is MinGW || host_is MSYS; then
  echo "default install dir is 'C:\QtAV'"
  echo cp config/config.xml $TARGET
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
binarycreator --offline-only -c $TARGET/config.xml -p $TARGET/packages --ignore-translations -v $INSTALLER
du -h $INSTALLER
