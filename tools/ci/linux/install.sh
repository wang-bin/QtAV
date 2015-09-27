set -ev
sudo apt-get update -qq
sudo apt-get install -qq -y libegl1-mesa-dev portaudio19-dev libpulse-dev libopenal-dev libva-dev libass-dev libxv-dev parallel
# TODO: do not use apt, use the latest code instead

wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/linux/${AV}-linux-x86+x64.tar.xz/download -O ${AV}-linux-x86+x64.tar.xz
tar Jxf ${AV}-linux-x86+x64.tar.xz


wget http://sourceforge.net/projects/buildqt/files/lite/Qt${QT}-Linux64.tar.xz/download -O Qt${QT}-Linux64.tar.xz
tar -I xz -xf Qt${QT}-Linux64.tar.xz

export FFMPEG_DIR=$PWD/${AV}-linux-x86+x64
export PATH=$PWD/Qt${QT}-Linux64/bin:$PATH
export CPATH=$FFMPEG_DIR/include
export LIBRARY_PATH=$FFMPEG_DIR/lib/x64
export LD_LIBRARY_PATH=$FFMPEG_DIR/lib/x64
export QTAV_OUT=QtAV-Qt${QT}-${AV}-ubuntu1204-${TRAVIS_COMMIT:0:7}
mkdir -p $QTAV_OUT

#if [ "$TRAVIS_BRANCH" == "prelease" ]; then
#linux
