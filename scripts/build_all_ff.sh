QTAV=$PWD/../QtAV
FFROOT=$PWD/..
FFVER=(0.8.14 0.9.2 1.0.7 1.1.5 1.2.1 2.0.1)
LIBAVVER=(9.7)
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
    qmake -r $QTAV 
    time make -j4 2>&1 |tee build.log
    cd $BUILDROOT
}
for V in ${LIBAVVER[@]}
do
    buildqtav libav $V
done
for V in ${FFVER[@]}
do
    buildqtav ffmpeg $V
done

