## 0. Prerequisites

Get QtAV source code

    git clone https://github.com/wang-bin/QtAV.git
    git submodule update --init


FFmpeg (>=1.0) or Libav (>=9.0) is always required. The latest FFmpeg release is recommended (that's what i use). If you use libav, you vaapi can not work in C++ apps and libavfilter does not work.

You can download precompiled FFmpeg from [QtAV sourceforge page](https://sourceforge.net/projects/qtav/files/depends/FFmpeg), or if you are using Windows you can also download FFmpeg development files from [Zeranoe](http://ffmpeg.zeranoe.com/builds).

Another option is to build FFmpeg yourself. See [FFmpeg compilation guides](http://trac.ffmpeg.org/wiki/CompilationGuide) for some pointers.

Other requirements are:

#### Windows

PortAudio or OpenAL.

#### OS X, iOS

None. System's OpenAL is used

#### Android

OpenAL with OpenSL backend. Currently OpenSL code doesn't work correctly, but OpenAL works fine.

#### Ubuntu

OpenAL. To enable all supported features, you must install XVideo and VA-API dev packages.

    sudo apt-get install libopenal-dev libva-dev libxv-dev

You may have to install VA-API drivers to make VA-API available at runtime. See https://github.com/wang-bin/QtAV/wiki/Enable-Hardware-Decoding


## 1. Setup the environment

You **MUST** let your compiler know where FFmpeg headers and libraries are. Otherwise you will get an error when running qmake. If they are already be where they should be, just skip this step.

Choose one of the following methods.

#### (Recommended) Put FFmpeg headers and libs into Qt directories

This is the simplest and best way to let compilers find ffmpeg and other depend libraries. Qt header dir and library dir is always be searched. This should work for all platforms, including android, iOS and meego.

#### Use Environment Vars

On Windows, Visual Studio will search headers in `%INCLUDE%` and search libraries in `%LIB%`, so you can set the environment like below if your compile in command line:

    set INCLUDE=ffmpeg_path\include;portaudio_path\include;%INCLUDE%
    set LIB=ffmpeg_path\lib;portaudio_path\lib;%LIB%

GCC will search headers in environment variables `$CPATH` and libraries in `$LIBRARY_PATH`. So you can set those vars to include your FFmepg and PortAudio dir.

gcc in unix shell environment(including mingw with sh.exe):

    export CPATH=ffmpeg_path/include:portaudio_path/include:$CPATH
    export LIBRARY_PATH=ffmpeg_path/lib:portaudio_path/lib:$LIBRARY_PATH

The project includes libQtAV.pri will not add linking options about FFmpeg etc., so the linker may find the dependent libraries from $LD_LIBRARY_PATH:

    export LD_LIBRARY_PATH=ffmpeg_path/lib:portaudio_path/lib:$LD_LIBRARY_PATH

GCC on windows cmd.exe environment without UNIX Shell:

    set CPATH=ffmpeg_path\include;portaudio_path\include;%CPATH%
    set LIBRARY_PATH=ffmpeg_path\lib;portaudio_path\lib;%LIBRARY_PATH%

If you are building in QtCreator, goto QtCreator's 'Projects' page and add or append those environment.

![QtCreator Settings](http://wang-bin.github.io/qtav.org/images/qtc-set.jpg "QtCreator Settings")

## 2. Run qmake

For most platforms, just

    qmake
    make

It's strongly recommend not to build in source dir.  

    mkdir your_build_dir
    cd your_build_dir
    qmake QtAV_source_dir/QtAV.pro
    make

qmake will run check the required libraries at the first time, so you must make sure those libraries can be found by compiler.

Then qmake will create a cache file `.qmake.cache` in your build dir. Cache file stores the check results, for example, whether portaudio is available. If you want to recheck, you can either delete `.qmake.cache` and run qmake again, or run

    qmake QtAV_source_dir/QtAV.pro  CONFIG+=recheck

_WARNING_: If you are in windows mingw with sh.exe environment, you may need run qmake twice. I have not found out the reason behind this phenomenon.

## 3. Make

use make, jom, nmake or QtCreator to build it.


### Build on Windows

You MUST setup the environment before qmake as mention at the beginning.

#### Build in Visual Studio

I don't put any vs project file in QtAV, because it's easy to create by qmake.  

Open cmd

    qmake -r -tp vc QtAV.pro

Then sln and vcxproj(vcproj) files will be created. Open QtAV.sln in your Visual Studio, Compile it. 

Another solution is using Qt vs plugin. It will help you to load qmake projects(not tested).

#### Build In QtCreator With MSVC

QtCreator will detect VC compiler if it is installed. So it's easy to build in QtCreator


#### Build in Command Line VC

I have got VC compiler and win sdk from latest VS2012 Update1. You can download it from http://qtbuild.googlecode.com/files/vs2012-x86.7z

The environment is small but has the almost complete functionality for developing C++. At least it can build Qt.


## Build Debian Packages

run

    debuild -us -uc

in QtAV source tree
