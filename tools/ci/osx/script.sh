set -ev

echo "$TRAVIS_BUILD_DIR"

tail -n 27 /proc/cpuinfo
uname -a
cat /etc/issue

echo "QtAV build script for travis-ci"

jobs=`sysctl -n hw.ncpu`


cd $QTAV_OUT

rm -f build.log
type -a moc
qmake -r $TRAVIS_BUILD_DIR -spec linux-clang "CONFIG+=recheck"
make -j$jobs 2>&1 |tee build.log

cd $TRAVIS_BUILD_DIR
