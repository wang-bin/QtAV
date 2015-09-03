set -ev

echo "$TRAVIS_BUILD_DIR"

sysctl -b machdep.cpu
sysctl -n hw.ncpu
uname -a

jobs=`sysctl -n hw.ncpu`

cd $QTAV_OUT

rm -f build.log
type -a moc
qmake -r $TRAVIS_BUILD_DIR "CONFIG+=recheck"
make -j$jobs

cd $TRAVIS_BUILD_DIR
