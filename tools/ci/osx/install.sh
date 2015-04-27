FFVER=2.6.2
test -f ffmpeg-${FFVER}-OSX.tar.xz || wget http://sourceforge.net/projects/qtav/files/depends/FFmpeg/OSX/ffmpeg-${FFVER}-OSX.tar.xz/download -O ffmpeg-${FFVER}-OSX.tar.xz
tar Jxf ffmpeg-${FFVER}-OSX.tar.xz

export FFMPEG_DIR=$PWD/ffmpeg-${FFVER}-OSX
export CPATH=$FFMPEG_DIR/include
export LIBRARY_PATH=$FFMPEG_DIR/lib/x64
export LD_LIBRARY_PATH=$FFMPEG_DIR/lib/x64

# TODO: download qt
brew update
brew install qt5

#export QTDIR
