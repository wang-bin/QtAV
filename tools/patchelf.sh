BINDIR=$1
echo $BINDIR

runpath() {
  local exe=$1
  test -n "$2" && exe=`echo $1 |sed "s,$2,,"`
  [ "${exe:0:2}" = "./" ] && exe=${exe:2}
  local d=`echo $exe |sed 's,[^\/],,g'`
  d=`echo $d |sed  's,\/,\/..,g'`
#  echo "$exe => $d"
  echo "\$ORIGIN:\$ORIGIN$d/lib:\$ORIGIN$d"
}

[ -d "$BINDIR" ] || BINDIR=$PWD
qmlso=(`find $BINDIR/qml -name "*.so" -type f`)
for q in ${qmlso[@]}; do
echo patching $q
  patchelf --set-rpath `runpath $q $BINDIR/` $q
done
plugins=(`find $BINDIR/plugins -name "*.so" -type f`)
for p in ${plugins[@]}; do
echo patching $p
  patchelf --set-rpath `runpath $p $BINDIR/` $p
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
