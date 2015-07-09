BINDIR=$1
echo $BINDIR
[ -d "$BINDIR" ] || BINDIR=$PWD
plugins=(`find $BINDIR/plugins -name "*.so" -type f`)
for plugin in ${plugins[@]}; do
  echo $plugin
  patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN/../..' $plugin
done
for qt in $BINDIR/libQt5*.so.5; do
echo patching $qt
  patchelf --set-rpath '$ORIGIN' $qt
done

QMLAV=$BINDIR/QtAV/libQmlAV.so 
test -f $QMLAV && echo "$QMLAV" && patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN/..:$ORIGIN/../..' $QMLAV
QMLAV=$BINDIR/qml/QtAV/libQmlAV.so
test -f $QMLAV && echo "$QMLAV" && patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN/..:$ORIGIN/../..' $QMLAV

EXE=(`find $BINDIR -maxdepth 1 -executable -type f |grep -v lib`)
for exe in ${EXE[@]}; do
  echo "patching $exe..."
  patchelf --set-rpath '$ORIGIN:$ORIGIN/lib:$ORIGIN/../lib' $exe
done
