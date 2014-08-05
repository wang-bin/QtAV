if [ $# -lt 2 ]; then
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

deploy() {
  local APP=$1
  local FRAMEWORK_DIR=bin/${APP}.app/Contents/Frameworks
  local EXE=bin/${APP}.app/Contents/MacOS/$APP
  mkdir -p $FRAMEWORK_DIR
  local LIBQTAV=`otool -L $EXE |awk '{print $1}' |grep libQtAV`
  local LIBCOMMON=`otool -L $EXE |awk '{print $1}' |grep libcommon`
  cp -Lf $LIBQTAV $LIBCOMMON $FRAMEWORK_DIR

  local LIB_PATH=$FRAMEWORK_DIR/${LIBQTAV/*\//}
  local LIBQTAV_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libQtAV`
  install_name_tool -change "$LIBQTAV_SELF" "@executable_path/../Frameworks/${LIBQTAV/*\//}" $LIB_PATH
  LIB_PATH=$FRAMEWORK_DIR/${LIBCOMMON/*\//}
  local LIBCOMMON_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libcommon`
  install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $LIB_PATH

  echo install_name_tool -change "$LIBQTAV_SELF" "@executable_path/../Frameworks/${LIBQTAV/*\//}" $EXE
  echo install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $EXE

  install_name_tool -change "$LIBQTAV_SELF" "@executable_path/../Frameworks/${LIBQTAV/*\//}" $EXE
  install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $EXE

  cd bin
  macdeployqt ${APP}.app -dmg -verbose=2
  cd -
}


deploy player
deploy QMLPlayer
