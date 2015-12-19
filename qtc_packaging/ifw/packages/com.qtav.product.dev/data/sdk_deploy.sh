# QtAV SDK deployment tool for installer

THIS=$0
IN=$PWD

help() {
  echo "$THIS -install|-uninstall qmake_path"
  exit 1
}
check_qmake() {
  [ -n "$QMAKE" ] || {
    echo "Input your absolute path of qmake. Make sure the major and minor version and target architecture. That path MUST have subdir bin, include, lib, mkspecs etc."
    echo "example: $HOME/Qt5.6.0/5.6/gcc_64/bin/qmake"
    read QMAKE
  }
  [ ! -d $QMAKE -a -x $QMAKE ] || {
    echo "qmake is not available ($QMAKE)"
    help
  }
  QT_INSTALL_VER=`$QMAKE -query QT_VERSION`
  [ "$ACT" != "0" ] && return
  echo "Checking Qt version..."
  QTAV_MAJOR=`grep -m1 QTAV_MAJOR include/QtAV/version.h |sed 's,.*QTAV_MAJOR \([0-9]*\).*,\1,'`
  QTAV_MAJOR=${QTAV_MAJOR:-1}
  QT_BUILD_VER=`strings $IN/bin/libQt*AV.so.$QTAV_MAJOR |grep "Qt-.* licensee: "|sed 's,Qt-\(.*\) licensee: ,\1,'`
  echo QT_BUILD_VER=$QT_BUILD_VER
  #install: skip version check if QT_BUILD_VER is empty
  if [ -n "$QT_BUILD_VER" -a "${QT_BUILD_VER:0:4}" != "${QT_INSTALL_VER:0:4}" ]; then
    echo "Qt runtime version ($QT_INSTALL_VER) mismatches. QtAV is built with $QT_BUILD_VER"
    echo "Major and minor version MUST be the same"
    help
  fi
}
ACT=0 #0 iinstall, 1 uninstall
  if [ "$1" = "-install" ]; then
    ACT=0
  elif [ "$1" = "-uninstall" ]; then
    ACT=1
  elif [ "${THIS##*-}" = "install" ]; then
    ACT=0
  elif [ "${THIS##*-}" = "uninstall" ]; then
    ACT=1
  else
    echo "Select action: 0 Install(default), 1 uninstall"
    read ACT
    if [ "$ACT" = "1" -o "$ACT" = "uninstall" ]; then
      ACT=1
    fi
  fi
if [ $# -gt 1 ]; then
  QMAKE=$2
fi
check_qmake

echo $QMAKE

QT_INSTALL_BINS=`$QMAKE -query QT_INSTALL_BINS`
QT_INSTALL_HEADERS=`$QMAKE -query QT_INSTALL_HEADERS`
QT_INSTALL_LIBS=`$QMAKE -query QT_INSTALL_LIBS`
QT_INSTALL_QML=`$QMAKE -query QT_INSTALL_QML`

if [ "$ACT" = "1" ]; then
  rm -rvf $QT_INSTALL_BINS/*Qt*AV*
  rm -rvf $QT_INSTALL_HEADERS/QtAV*
  rm -rvf $QT_INSTALL_LIBS/*Qt*AV*
  rm -rvf $QT_INSTALL_QML/QtAV
  rm -rvf $QT_INSTALL_BINS/../mkspecs/modules/qt_lib_av*.pri
  rm -rvf $QT_INSTALL_BINS/../mkspecs/features/av*.prf
  echo "QtAV uninstalled"
  exit 0
fi

cp -avf $IN/bin/*Qt*AV*.so* $QT_INSTALL_BINS
cp -avf $IN/include/* `$QMAKE -query QT_INSTALL_HEADERS`
cp -avf $IN/lib/* $QT_INSTALL_LIBS
cp -avf $IN/qml/QtAV `$QMAKE -query QT_INSTALL_QML`
[ -d $QT_INSTALL_BINS/../mkspecs ] && cp -avf $IN/mkspecs/* $QT_INSTALL_BINS/../mkspecs || echo "Warning: mkspecs not found"
test -f $QT_INSTALL_LIBS/libQt5AV.so || test -f $QT_INSTALL_LIBS/libQtAV.so || {
  cd $QT_INSTALL_LIBS
  if [ -f $QT_INSTALL_BINS/libQt5AV.so.$QTAV_MAJOR ]; then
    ln -sf $QT_INSTALL_BINS/libQt5AV.so.$QTAV_MAJOR libQt5AV.so
    ln -sf $QT_INSTALL_BINS/libQt5AVWidgets.so.$QTAV_MAJOR libQt5AVWidgets.so
  elif [ -f $QT_INSTALL_BINS/libQtAV.so.$QTAV_MAJOR ]; then
    ln -sf $QT_INSTALL_BINS/libQtAV.so.$QTAV_MAJOR libQtAV.so
    ln -sf $QT_INSTALL_BINS/libQtAVWidgets.so.$QTAV_MAJOR libQtAVWidgets.so
  fi
  cd -
}

echo "QtAV installed"
