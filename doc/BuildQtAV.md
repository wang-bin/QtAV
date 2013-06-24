## 1. Setup the environment

QtAV depends on FFmpeg, PortAudio and some other optional libraries such as direct2d and xvideo.
You can download FFmpeg and PortAudio development files for windows from [QtAV sourceforge page](https://sourceforge.net/projects/qtav/files/depends)
You can also download FFmpeg development files for windows from  [Zeranoe](http://ffmpeg.zeranoe.com/builds)
Or you can build them your self [Compile FFmpeg and PortAudio](https://github.com/wang-bin/QtAV/wiki/Compile-FFmpeg-and-PortAudio)

You *MUST* let your compiler know where FFmpeg headers and libraries are. Otherwise you will get an error when running qmake. If they are already be where they should be, just skip this step.

vc compiler will search headers in __*INCLUDE*__ and search libraries in __*LIB*__, so you can set the environment like below if your compile in command line

    set INCLUDE=ffmpeg_path\include;portaudio_path\include;%INCLUDE%
    set LIB=ffmpeg_path\lib;portaudio_path\lib;%LIB%

GCC will search headers in environment var __*CPATH*__ and libraries in __*LIBRARY_PATH*__. So you can set those vars to include your FFmepg and PortAudio dir.

gcc in unix shell environment(including mingw with sh.exe):

    export CPATH=ffmpeg_path/include:portaudio_path/include:$CPATH
    export LIBRARY_PATH=ffmpeg_path/include:portaudio_path/lib:$LIBRARY_PATH

The project includes libQtAV.pri will not add linking options about FFmpeg etc., so the linker may find the depended libraries from $LD_LIBRARY_PATH:

    export LD_LIBRARY_PATH=ffmpeg_path/lib:portaudio_path/lib:$LD_LIBRARY_PATH

gcc in windows cmd environment without sh.exe

    set CPATH=ffmpeg_path\include;portaudio_path\include;%CPATH%
    set LIBRARY_PATH=ffmpeg_path\lib;portaudio_path\lib;%LIBRARY_PATH%

If you are building in QtCreator, goto QtCreator's 'Projects' page and add or append those environment.

## 2. Run qmake

For most platforms, just

    qmake
    make

It's strongly recommend not to build in source dir.  

    cd your_build_dir
    qmake QtAV_source_dir/QtAV.pro
    make

qmake will run check the required libraries at the first time, so you must make sure those libraries can be found by compiler.
Then qmake will create a cache file _.qmake.cache_ in your build dir. Cache file stores the check results, for example, whether portaudio is available. If you want to recheck, you can either delete _**.qmake.cache**_ and run qmake again, or run

    qmake QtAV_source_dir/QtAV.pro  CONFIG+=recheck


_WARNING_: If you are in windows mingw with sh.exe environment, you may need run qmake twice. I have not find out the reason!

## 3. Make

use make, jom, nmake or QtCreator to build it.



## Build in Windows

You MUST setup the environment before qmake as mention at the beginning.

#### Build In Visual Studio

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
