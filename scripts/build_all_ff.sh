QTAV=$PWD/../QtAV
FFROOT=$PWD/..
FFVER=(0.11.5 1.0.9 1.1.12 1.2.7 2.0.5 2.1.5 2.2.5 2.3)
#LIBAVVER=(0.8.8 9.7)
BUILDROOT=$PWD
buildqtav() {
    FF=$1
    VER=$2
    FFSDK=$FFROOT/$FF-$VER/sdk
    export CPATH=$FFSDK/include
    export LIBRARY_PATH=$FFSDK/lib
    export LD_LIBRARY_PATH=$FFSDK/lib
    echo $CPATH
    echo "building... with $FFSDK"
    OUT=qtav_$FF$VER
    mkdir -p $OUT
    cd $OUT
    rm -f build.log
    qmake -r $QTAV -config silent
    time make -j4 2>&1 |tee build.log
    ln -sf $FFSDK/lib/* bin
    cd $BUILDROOT
}

time (
for V in ${LIBAVVER[@]}
do
    buildqtav libav $V
done
for V in ${FFVER[@]}
do
    buildqtav ffmpeg $V
done
)
