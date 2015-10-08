

function fix_qt_path() {
  local EXE=$1
  local QTEXE=`otool -L $EXE |grep Qt |grep -v ':'`
  for Q in $QTEXE
  do
    if [ -n "`echo $Q |grep framework`" ]; then
      echo Q=$Q
      OLD=`echo $Q |cut -d ' ' -f 1`
      echo OLD=$OLD
      REL=`echo $OLD |sed 's,.*\/\(.*\)\(\.framework\/.*\),\1\2,'`
      REL_DIR=${REL%/*}
      NEW="@rpath/Frameworks/$REL"
      #NEW="@rpath/Frameworks/$REL"
      install_name_tool -change $OLD $NEW $EXE #$REL
      echo install_name_tool -change $OLD $NEW $EXE

    fi
  done
}

function copy_qt() {
  local EXE=$1
  local QTEXE=`otool -L $EXE |grep Qt |grep -v ':'`
  for Q in $QTEXE
  do
    if [ -n "`echo $Q |grep framework`" ]; then
      echo Q=$Q
      OLD=`echo $Q |cut -d ' ' -f 1`
      echo OLD=$OLD
      REL=`echo $OLD |sed 's,.*\/\(.*\)\(\.framework\/.*\),\1\2,'`
      REL_DIR=${REL%/*}
      mkdir -p $REL_DIR

      if [ -f $REL ]; then
        echo "already exists. skip copy. $REL"
      else
        cp -L $Q $REL
      fi
      NEW="@rpath/Frameworks/$REL"
      install_name_tool -change $OLD $NEW $REL
      echo install_name_tool -change $OLD $NEW $EXE
    fi
  done
}

[ "$1" = "-c" ] && copy_qt $2 || fix_qt_path $1
