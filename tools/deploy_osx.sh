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
  local MODULE_FRAMEWORK=$1
  local MODULE_NAME=${MODULE_FRAMEWORK##*/}
  local MODULE_FRAMEWORK_DIR=${MODULE_FRAMEWORK%/Versions*}
  local MODULE_FRAMEWORK_IN=${MODULE_FRAMEWORK##*${MODULE_NAME}.framework/}
  echo install_name_tool -id "@executable_path/../Frameworks/${MODULE_NAME}.framework/$MODULE_FRAMEWORK_IN" $MODULE_FRAMEWORK
  install_name_tool -id "@executable_path/../Frameworks/${MODULE_NAME}.framework/$MODULE_FRAMEWORK_IN" $MODULE_FRAMEWORK
  for ff in libportaudio libavutil libavcodec libavformat libavfilter libavdevice libavresample libswscale libswresample libpostproc
  do
    local FFPATH=`otool -L $MODULE_FRAMEWORK |awk '{print $1}' |grep -v : |grep $ff`
    local FFNAME=${FFPATH##*/}
    #echo install_name_tool -change "$FFPATH" "@executable_path/../Frameworks/$FFNAME" $MODULE_FRAMEWORK
    install_name_tool -change "$FFPATH" "@executable_path/../Frameworks/$FFNAME" $MODULE_FRAMEWORK
  done
  for qt in QtCore QtGui QtOpenGL QtWidgets
  do
    local QTPATH=`otool -L $MODULE_FRAMEWORK |awk '{print $1}' |grep -v : |grep $qt`
    local QT_IN=${QTPATH//*\/Versions\//Versions\/}
    #echo install_name_tool -change "$QTPATH" "@executable_path/../Frameworks/${qt}.framework/$QT_IN" $MODULE_FRAMEWORK
    install_name_tool -change "$QTPATH" "@executable_path/../Frameworks/${qt}.framework/$QT_IN" $MODULE_FRAMEWORK
  done
}

fix_qtav_dylib() {
  echo "not implemented"
}

deploy_qtav_module() {
  local AVMODUlE=$1
  local APP=$2
  local FRAMEWORK_DIR=bin/${APP}.app/Contents/Frameworks
  local EXE=bin/${APP}.app/Contents/MacOS/$APP

  local MODULE_LIB=`otool -L $EXE |awk '{print $1}' |grep lib${AVMODUlE}` #MODULE_LIB
  local MODULE_FRAMEWORK=`otool -L $EXE |awk '{print $1}' |grep ${AVMODUlE}\.framework` #MODULE_FRAMEWORK
  local MODULE_FRAMEWORK_DIR=${MODULE_FRAMEWORK%/Versions*}
  local MODULE_FRAMEWORK_IN=${MODULE_FRAMEWORK##*${AVMODUlE}.framework/}
  if [ -f "$MODULE_LIB" ]; then
    cp -Lf $MODULE_LIB $FRAMEWORK_DIR
    local LIB_PATH=$FRAMEWORK_DIR/${MODULE_LIB/*\//}
    local MODULE_LIB_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep lib${AVMODUlE}`
    install_name_tool -id "@executable_path/../Frameworks/${MODULE_LIB/*\//}" $LIB_PATH
    install_name_tool -change "$MODULE_LIB_SELF" "@executable_path/../Frameworks/${MODULE_LIB/*\//}" $EXE
  else
    cp -af $MODULE_FRAMEWORK_DIR $FRAMEWORK_DIR
    fix_qtav_framework $FRAMEWORK_DIR/${AVMODUlE}.framework/$MODULE_FRAMEWORK_IN
    install_name_tool -change "$MODULE_FRAMEWORK" "@executable_path/../Frameworks/${AVMODUlE}.framework/$MODULE_FRAMEWORK_IN" $EXE
  fi
}

deploy() {
  local APP=$1
  local FRAMEWORK_DIR=bin/${APP}.app/Contents/Frameworks
  local EXE=bin/${APP}.app/Contents/MacOS/$APP
  mkdir -p $FRAMEWORK_DIR

  local LIBCOMMON=`otool -L $EXE |awk '{print $1}' |grep libcommon`
  cp -Lf $LIBCOMMON $FRAMEWORK_DIR

  local LIB_PATH=$FRAMEWORK_DIR/${LIBCOMMON/*\//}
  local LIBCOMMON_SELF=`otool -L $LIB_PATH |awk '{print $1}' |grep -v : |grep libcommon`
  install_name_tool -id "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $LIB_PATH
  install_name_tool -change "$LIBCOMMON_SELF" "@executable_path/../Frameworks/${LIBCOMMON/*\//}" $EXE

  for module in QtAV QtAVWidgets; do
    deploy_qtav_module $module $APP
  done
  echo "macdeplyqt"
  cd bin
  macdeployqt ${APP}.app -dmg -verbose=0
  cd -
}


deploy player
deploy QMLPlayer
