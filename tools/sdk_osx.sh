THIS_DIR=`dirname "${BASH_SOURCE[0]}"`
ARGV=($@)
ARGC=${#ARGV[@]}
echo $THIS_DIR |grep "\.app" && test -d $THIS_DIR/Contents/Frameworks/QtCore.framework && IS_BOUNDLE=0
[ $ARGC -gt 0 ] && QT_LIBS=${ARGV[ARGC-1]}
if [ -n "$IS_BOUNDLE" ]; then
  if [ ! -e $QT_LIBS/QtCore.framework ]; then
    QT_VER=`otool -L $THIS_DIR/Contents/Frameworks/QtCore.framework/QtCore | grep -v ":" |grep "QtCore\.framework" |sed 's,.*current version \(.*\)\.[0-9]),\1,'`
    echo "Input the absolute path of Qt$QT_VER installed lib dir. For example: /Users/xxx/Qt5.4.1/5.4/clang_64/lib"
    read QT_LIBS
  fi
fi
if [ "$1" = "-uninstall" ]; then
  rm -rfv $QT_LIBS/QtAV* $QT_LIBS/../qml/QtAV $QT_LIBS/../include/QtAV*
  rm -fv $QT_LIBS/../mkspecs/modules/qt_lib_av* $QT_LIBS/../mkspecs/features/av*.prf
  exit 0
fi

test -e $QT_LIBS/QtCore.framework || {
  echo "./${0##*/} paths_of_qtav_frameworks \$QTDIR/lib"
  echo "foo.app/${0##*/} \$QTDIR/lib"
  exit 0
}
# TODO: read from stdin if no arguments

fix_qt() {
  local lib=$1
  echo fixing $lib...
  local list=`otool -L $lib`
  for l in $list; do
    local name=`echo $l | grep -E "[^@].*/Qt[^\.]+\.framework.*[^:]$"`
    if [ $name ]; then
      local fw="$QT_LIBS/"`echo $name | sed -E "s/^.*(Qt[^\.]+.framework.*Qt.*)$/\1/"`
      install_name_tool -change $name $fw $lib
    fi
  done
}

echo "deploying frameworks..."

unset ARGV[$ARGC-1]
AV_FRAMEWORKS=(${ARGV[@]})
#which ${BASH_SOURCE[0]}
echo $THIS_DIR |grep "\.app" && test -d $THIS_DIR/Contents/Frameworks/QtAV.framework && {
  find . -name "QtAV*.framework"
  AV_FRAMEWORKS=(`find $THIS_DIR -name "QtAV*.framework"`)
}
echo "AV_FRAMEWORKS: ${AV_FRAMEWORKS[@]}"
for av in ${AV_FRAMEWORKS[@]}; do
  test -f $av/QtAV && {
    QTAV_VER=`otool -L $av/QtAV | grep -v ":" |grep "QtAV\.framework" |sed 's,.*current version \(.*\)),\1,'`
    QTAV_VER_MAJOR=`echo $QTAV_VER |cut -d "." -f 1`
    QTAV_VER_MINOR=`echo $QTAV_VER |cut -d "." -f 2`
    QTAV_VER_PATCH=`echo $QTAV_VER |cut -d "." -f 3`
    break
  }
done
echo QtAV version: $QTAV_VER
for av in ${AV_FRAMEWORKS[@]}; do
  F=${av##*/}
  name=${F%\.framework}
  rm -rf $QT_LIBS/$F
  cp -af $av $QT_LIBS
  find $QT_LIBS/$F -name "*.prl" -delete
  install_name_tool -id $QT_LIBS/$F/Versions/$QTAV_VER_MAJOR/$name $QT_LIBS/$F/Versions/$QTAV_VER_MAJOR/$name
  fix_qt $QT_LIBS/$F/Versions/$QTAV_VER_MAJOR/$name
done

if [ -d $THIS_DIR/Contents/Resources/qml/QtAV ]; then
  echo "deploying QtAV qml module..."
  cp -af $THIS_DIR/Contents/Resources/qml/QtAV $QT_LIBS/../qml
  install_name_tool -id $QT_LIBS/../qml/QtAV/libQmlAV.dylib $QT_LIBS/../qml/QtAV/libQmlAV.dylib
fi

echo "deploying mkspecs..."
tolower(){
  echo "$@" | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
}

write_module_pri() {
  local M=$1
  local m=`tolower $M`
  local dep=$2
  echo $QT_LIBS/../mkspecs/modules/qt_lib_${m}.pri
cat >$QT_LIBS/../mkspecs/modules/qt_lib_${m}.pri<<EOF
QT.${m}.VERSION = ${QTAV_VER}
QT.${m}.MAJOR_VERSION = ${QTAV_VER_MAJOR}
QT.${m}.MINOR_VERSION = ${QTAV_VER_MINOR}
QT.${m}.PATCH_VERSION = ${QTAV_VER_PATCH}
QT.${m}.name = Qt${M}
QT.${m}.libs = \$\$QT_MODULE_LIB_BASE
QT.${m}.rpath = $QT_LIBS
QT.${m}.includes = \$\$QT_MODULE_INCLUDE_BASE \$\$QT_MODULE_INCLUDE_BASE/Qt${M}
QT.${m}.bins = \$\$QT_MODULE_BIN_BASE
QT.${m}.libexecs = \$\$QT_MODULE_LIBEXEC_BASE
QT.${m}.plugins = \$\$QT_MODULE_PLUGIN_BASE
QT.${m}.imports = \$\$QT_MODULE_IMPORT_BASE
QT.${m}.qml = \$\$QT_MODULE_QML_BASE
QT.${m}.depends = $dep
QT.${m}.module_config = lib_bundle
QT.${m}.CONFIG = ${m}
QT.${m}.DEFINES = QT_AV_LIB
QT_MODULES += ${m}
EOF
}
write_module_private_pri() {
  local M=$1
  local m=`tolower $M`
  local dep=$2
  echo $QT_LIBS/../mkspecs/modules/qt_lib_${m}_private.pri
cat >$QT_LIBS/../mkspecs/modules/qt_lib_${m}_private.pri<<EOF
QT.${m}_private.VERSION = ${QTAV_VER}
QT.${m}_private.MAJOR_VERSION = ${QTAV_VER_MAJOR}
QT.${m}_private.MINOR_VERSION = ${QTAV_VER_MINOR}
QT.${m}_private.PATCH_VERSION = ${QTAV_VER_PATCH}
QT.${m}_private.name = Qt${M}
QT.${m}_private.libs = \$\$QT_MODULE_LIB_BASE
QT.${m}_private.includes = \$\$QT_MODULE_INCLUDE_BASE/QtAV/${QTAV_VER} \$\$QT_MODULE_INCLUDE_BASE/Qt${M}/${QTAV_VER}/QtAV
QT.${m}_private.depends = $dep
QT.${m}_private.module_config = lib_bundle internal_module no_link
EOF
}
write_module_prf() {
  local M=$1
  local m=`tolower $M`
  echo "$QT_LIBS/../mkspecs/features/${m}.prf"
cat>$QT_LIBS/../mkspecs/features/${m}.prf<<EOF
android: QMAKE_LFLAGS += -lOpenSLES
!contains(QT, ${m}): QT *= ${m}
INCLUDEPATH *= $QT_LIBS/Qt${M}.framework/Headers/Qt${M}
EOF
}

write_module_pri AV "core gui"
write_module_pri AVWidgets "av opengl"
write_module_private_pri AV "av"
write_module_private_pri AVWidgets "avwidgets"
write_module_prf AV
write_module_prf AVWidgets
