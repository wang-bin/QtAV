#     Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

if [ $# -lt 1 ]; then
  echo "${0##*/} QtAV_build_dir"
  exit 1
fi

BUILD_DIR=$1
QTDIR=


cd $BUILD_DIR
QTCORE=`otool -L bin/player.app/Contents/MacOS/player |awk '{print $1}' |grep QtCore`
QTDIR=${QTCORE//\/lib\/QtCore*/}
export PATH=$QTDIR/bin:$PATH
echo "QTDIR: $QTDIR"

fix_qtav_framework() {
  local QTAV_FRAMEWORK=$1
  local QTAV_FRAMEWORK_DIR=${QTAV_FRAMEWORK%/Versions*}
  local QTAV_FRAMEWORK_IN=${QTAV_FRAMEWORK##*QtAV.framework/}
  echo install_name_tool -id "@executable_path/../Frameworks/QtAV.framework/$QTAV_FRAMEWORK_IN" $QTAV_FRAMEWORK
  install_name_tool -id "@executable_path/../Frameworks/QtAV.framework/$QTAV_FRAMEWORK_IN" $QTAV_FRAMEWORK
  for ff in libportaudio libavutil libavcodec libavformat libavfilter libavdevice libavresample libswscale libswresample libpostproc
  do
    local FFPATH=`otool -L $QTAV_FRAMEWORK |awk '{print $1}' |grep -v : |grep $ff`
    local FFNAME=${FFPATH##*/}
    echo install_name_tool -change "$FFPATH" "@executable_path/../Frameworks/$FFNAME" $QTAV_FRAMEWORK
    install_name_tool -change "$FFPATH" "@executable_path/../Frameworks/$FFNAME" $QTAV_FRAMEWORK
  done
  for qt in QtCore QtGui QtOpenGL QtWidgets
  do
    local QTPATH=`otool -L $QTAV_FRAMEWORK |awk '{print $1}' |grep -v : |grep $qt`
    local QT_IN=${QTPATH//*\/Versions\//Versions\/}
    echo install_name_tool -change "$QTPATH" "@executable_path/../Frameworks/${qt}.framework/$QT_IN" $QTAV_FRAMEWORK
    install_name_tool -change "$QTPATH" "@executable_path/../Frameworks/${qt}.framework/$QT_IN" $QTAV_FRAMEWORK
  done
}

fix_qtav_dylib() {
  echo "not implemented"
}

deploy() {
  local APP=$1
  local FRAMEWORK_DIR=bin/${APP}.app/Contents/Frameworks
  local EXE=bin/${APP}.app/Contents/MacOS/$APP
  mkdir -p $FRAMEWORK_DIR
  local LIBQTAV=`otool -L $EXE |awk '{print $1}' |grep libQtAV`
  local QTAV_FRAMEWORK=`otool -L $EXE |awk '{print $1}' |grep QtAV\.framework`
  local QTAV_FRAMEWORK_DIR=${QTAV_FRAMEWORK%/Versions*}
  local QTAV_FRAMEWORK_IN=${QTAV_FRAMEWORK##*QtAV.framework/}
  local LIBCOMMON=`otool -L $EXE |awk '{print $1}' |grep libcommon`
  cp -Lf $LIBQTAV $LIBCOMMON $FRAMEWORK_DIR

  local LIB_PATH=$FRAMEWORK_DIR/${LIBCOMMON/*\//}
  local LIBCOMMON_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libcommon`
  install_name_tool -id "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $LIB_PATH
  install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $EXE

  if [ "x$QTAV_FRAMEWORK_DIR" = "x" ]; then
    LIB_PATH=$FRAMEWORK_DIR/${LIBQTAV/*\//}
    local LIBQTAV_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libQtAV`
    install_name_tool -id "@executable_path/../Frameworks/${LIBQTAV/*\//}" $LIB_PATH
    install_name_tool -change "$LIBQTAV_SELF" "@executable_path/../Frameworks/${LIBQTAV/*\//}" $EXE
  else
    cp -af $QTAV_FRAMEWORK_DIR $FRAMEWORK_DIR
    fix_qtav_framework $FRAMEWORK_DIR/QtAV.framework/$QTAV_FRAMEWORK_IN
    install_name_tool -change "$QTAV_FRAMEWORK" "@executable_path/../Frameworks/QtAV.framework/$QTAV_FRAMEWORK_IN" $EXE
  fi
  echo "macdeplyqt"
  cd bin
  macdeployqt ${APP}.app -dmg -verbose=0
  cd -
}


deploy player
deploy QMLPlayer
