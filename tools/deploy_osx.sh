#     Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

if [ $# -lt 1 ]; then
  echo "${0##*/} QtAV_build_dir"
  exit 1
fi
#set -ev
THIS_DIR=$PWD
BUILD_DIR=$1
QTBIN=`grep -m 1 QT_BIN $BUILD_DIR/.qmake.cache |cut -d "=" -f 2 | tr -d ' '` #TODO: use last line
QTDIR=$QTBIN/..
echo "BUILD_DIR: $BUILD_DIR"
echo "QTDIR: $QTDIR"
export PATH=$QTBIN:$PATH

fix_qt() {
  local dir=bin/$1.app/Contents
  local all=`find $dir/Frameworks -name 'Qt*' -type f | grep -E '/Qt[^/.]+$' |grep -v Headers`
  all+=" "`find $dir -name '*.dylib'`
  all+=" $dir/MacOS/$1"

  for one in $all; do
    echo fixing $one...
    list=`otool -L $one`
    for l in $list; do
      local name=`echo $l | grep -E "[^@].*/Qt[^\.]+\.framework.*[^:]$"`
      if [ $name ]; then
        local fw="@executable_path/../Frameworks/"`echo $name | sed -E "s/^.*(Qt[^\.]+.framework.*Qt.*)$/\1/"`
        install_name_tool -change $name $fw $one
      fi
    done
  done
}
#TODO: fix_av

deploy() {
  cd $BUILD_DIR
  local APP=$1
  local FRAMEWORK_DIR=bin/${APP}.app/Contents/Frameworks
  local EXE=bin/${APP}.app/Contents/MacOS/$APP
  [ -f $THIS_DIR/sdk_osx.sh ] && cp -Lf $THIS_DIR/sdk_osx.sh bin/${APP}.app/
  mkdir -p $FRAMEWORK_DIR

  local LIBCOMMON=`otool -L $EXE |awk '{print $1}' |grep libcommon`
  [ ! -f $LIBCOMMON ] && LIBCOMMON=lib_*/libcommon.1.dylib
  if [ -f $LIBCOMMON ]; then
    cp -Lf $LIBCOMMON $FRAMEWORK_DIR

    local LIB_PATH=$FRAMEWORK_DIR/${LIBCOMMON/*\//}
    local LIBCOMMON_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libcommon`
    install_name_tool -id "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $LIB_PATH
    install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $EXE
  fi
  local QT_LIBS=`$QTBIN/qmake -query QT_INSTALL_LIBS`
  local QT_PLUGINS=`$QTBIN/qmake -query QT_INSTALL_PLUGINS`
  local QT_QML=`$QTBIN/qmake -query QT_INSTALL_QML`
  #cocoa depends on QtPrintSupport and DBus(5.5), QtSVG & QtPrintSupport depends on QtWidgets
  QTMODULES="QtCore QtGui QtSvg QtPrintSupport QtWidgets QtDBus QtSql"
  otool -L $EXE |grep QtOpenGL && QTMODULES+=" QtOpenGL"
  otool -L $EXE |grep QtQuick && QML="Qt QtQml QtQuick QtQuick.2" && QTMODULES+=" QtQuick QtQml QtNetwork"
  for q in $QTMODULES; do
    cp -af $QT_LIBS/${q}.framework $FRAMEWORK_DIR  &>/dev/null
    find $FRAMEWORK_DIR/${q}.framework -name Headers -exec rm -rf {} \; &>/dev/null
    local Q=$FRAMEWORK_DIR/${q}.framework/Versions/5/${q}
    test -f $Q && install_name_tool -id @executable_path/../Frameworks/${q}.framework/Versions/5/${q} $Q
  done
  mkdir -p $FRAMEWORK_DIR/../PlugIns/{platforms,imageformats,iconengines,sqldrivers}
  for q in platforms/libqcocoa.dylib imageformats/libqsvg.dylib imageformats/libqjpeg.dylib iconengines/libqsvgicon.dylib sqldrivers/libqsqlite.dylib; do
    cp -aLf $QT_PLUGINS/${q} $FRAMEWORK_DIR/../PlugIns/${q}
  done
  mkdir -p $FRAMEWORK_DIR/../Resources/qml
  for q in $QML; do
    cp -aLf $QT_QML/$q $FRAMEWORK_DIR/../Resources/qml
  done
  cp -aLf bin/QtAV $FRAMEWORK_DIR/../Resources/qml

  local AVMODULES="QtAV"
  otool -L $EXE |grep QtWidgets && AVMODULES+=" QtAVWidgets"
  for module in $AVMODULES; do
    rm -rf $FRAMEWORK_DIR/${module}.framework
    cp -af lib*/${module}.framework $FRAMEWORK_DIR
    local MODULE_LIB=`otool -D $FRAMEWORK_DIR/${module}.framework/${module} |grep -v ":"`
    local MODULE_LIB_REL=${MODULE_LIB/*${module}.framework/${module}.framework}
    if [ "MODULE_LIB" = "MODULE_LIB_REL" ]; then
      MODULE_LIB_REL=${MODULE_LIB##*/} #a dylib
    fi
    echo MODULE_LIB_REL=$MODULE_LIB_REL
    local LIB_PATH=$FRAMEWORK_DIR/${MODULE_LIB/*\//}
    install_name_tool -id "@executable_path/../Frameworks/${MODULE_LIB_REL}" $LIB_PATH
    local EXE_DEP=`otool -L $EXE |awk '{print $1}' |grep -v : |grep "${MODULE_LIB_REL}"`
    install_name_tool -change "$EXE_DEP" "@executable_path/../Frameworks/${MODULE_LIB_REL}" $EXE
  done
:<<EOC
  for ff in libavutil libavcodec libavformat libavfilter libavdevice libavresample libswscale libswresample libpostproc
  do
    local FFPATH=`otool -L $MODULE_LIB |awk '{print $1}' |grep -v : |grep $ff`
    if [ -f "$FRAMEWORK_DIR/${FFPATH##*/}" ]; then
      install_name_tool -id "@executable_path/../Frameworks/${FFPATH##*/}" $FRAMEWORK_DIR/${FFPATH##*/}
      install_name_tool -change "$FFPATH" "@executable_path/../Frameworks/${FFPATH##*/}" $MODULE_LIB
    fi
  done
EOC
  find bin/${APP}.app -name "*_debug*" -delete
  find bin/${APP}.app -name "log*.txt" -delete
  find bin/${APP}.app -name "*.prl" -delete

  fix_qt $APP
cat>bin/${APP}.app/Contents/Resources/qt.conf<<EOF
[Paths]
Plugins = PlugIns
Qml2Imports = Resources/qml
EOF
  cd -
  if [ -n "$TRAVIS_BUILD_DIR" ]; then
    test -f ./create-dmg/create-dmg && ./create-dmg/create-dmg  --window-size 500 300  --icon-size 96 --volname "QtAV $APP"  --app-drop-link 240 200 QtAV-$APP.dmg $BUILD_DIR/bin/${APP}.app
  else
    test -f ./create-dmg/create-dmg && ./create-dmg/create-dmg  --window-size 500 300  --icon-size 96 --volname "QtAV $APP"  --app-drop-link 240 200 QtAV-$APP.dmg $BUILD_DIR/bin/${APP}.app &
  fi
}

deploy Player
deploy QMLPlayer
