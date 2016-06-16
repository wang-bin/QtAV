set -ev

echo "$TRAVIS_BUILD_DIR"

tail -n 27 /proc/cpuinfo
uname -a
cat /etc/issue

echo "QtAV build script for travis-ci"

jobs=`cat /proc/cpuinfo |grep 'cpu cores' |wc -l`
cd $QTAV_OUT

rm -f build.log
type -a moc
SPEC=linux-clang
test -n "$CC" && test "${CC/gcc/}" != "$CC" && SPEC=linux-g++
qmake -r $TRAVIS_BUILD_DIR -spec $SPEC "CONFIG+=recheck"
make -j$((jobs+1))

cd $TRAVIS_BUILD_DIR
