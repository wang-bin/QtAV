***Uninstall QtAV SDK before building to avoid header files confliction. Run sdk_uninstall.bat/sh under your build dir***

Shadow build is recommended (required for mac build).

## 0. Prerequisites

- Get QtAV source code

  use git

      git clone https://github.com/wang-bin/QtAV.git
      git submodule update --init

- FFmpeg>=1.0/Libav>=9.0. The latest release is recommended.

  You can download latest [prebuilt FFmpeg](https://sourceforge.net/projects/qtav/files/depends), or [build yourself](https://github.com/wang-bin/build_ffmpeg/wiki)

- Libass headers is required if you need ass subtitle rendering support.

For windows, download http://sourceforge.net/projects/qtav/files/depends/QtAV-depends-windows-x86%2Bx64.7z/download and using `bin, include, lib` in the package is enough to build.

Other requirements are:

- ***Windows***

  OpenAL(Optional). OpenAL is not required since QtAV1.8.0. XAudio2 is always used. XAudio2 supports XP~windows 10 and Windows Store apps. Windows 8 and later natively supports XAudio2. For Windows 7 and older, you have to install [the driver from DirectX](http://sourceforge.net/projects/qtav/files/depends/DXSDK2010_XAudio2_redist.7z/download) to run.

- ***OS X, iOS*** None. AudioToolbox and System OpenAL is used
- ***Android***

  On Windows you must put `mingw32-make.exe` in one of `%PATH%` to avoid qmake error.

- ***Ubuntu***

  OpenAL(recommended) or PulseAudio. To enable all supported features, you must install libass, XVideo and VA-API dev packages.

      sudo apt-get install libopenal-dev libpulse-dev libva-dev libxv-dev libass-dev libegl1-mesa-dev

  You may have to [install VA-API drivers](https://github.com/wang-bin/QtAV/wiki/Hardware-Accelerated-Decoding#va-api) to make VA-API available at runtime


## 1. Setup the environment

You **MUST** let your compiler know where FFmpeg headers and libraries are. Otherwise you will get an error when running qmake. If they are already be where they should be, for example you install from apt on ubunt, just skip this step.

Choose one of the following methods.

#### (Recommended) Put FFmpeg headers and libs into Qt directories

It's the simplest and best way. Qt include and lib dir are always searched in QtAV. It should work for all platforms, including android, iOS, WinRT and meego etc.

#### (NOT Recommended)Use Environment Vars

- VC: `INCLUDE` and `LIB`

  command line:

      set INCLUDE=ffmpeg_path\include;openal_path\include;%INCLUDE%
      set LIB=ffmpeg_path\lib;openal_path\lib;%LIB%

- gcc/clang: `CPATH` and `LIBRARY_PATH`

  unix shell environment(including mingw with sh.exe) command line

      export CPATH=ffmpeg_path/include:openal_path/include:$CPATH
      export LIBRARY_PATH=ffmpeg_path/lib:openal_path/lib:$LIBRARY_PATH

  windows cmd.exe environment without UNIX Shell command line

      set CPATH=ffmpeg_path\include;openal_path\include;%CPATH%
      set LIBRARY_PATH=ffmpeg_path\lib;openal_path\lib;%LIBRARY_PATH%

- If build in QtCreator, open 'Projects' page and add/append the environment/values.

  ![QtCreator Settings](http://wang-bin.github.io/qtav.org/images/qtc-set.jpg "QtCreator Settings")


## 2. Build in QtCreator or command line
- Command line build

  For most platforms, just run

    mkdir your_build_dir
    cd your_build_dir
    qmake QtAV_source_dir/QtAV.pro
    make -j4

  It's strongly recommended not to build in source dir(especially OSX).  

  qmake will check the required libraries to make sure they can be found by compiler.

  Then qmake will create a cache file `.qmake.cache` in your build dir. Cache file stores the dependencies check results, for example, whether openal is available. If you want to recheck, you can either delete `.qmake.cache` and run qmake again, or run

      qmake QtAV_source_dir/QtAV.pro -r "CONFIG+=recheck"

_WARNING_: If you are in windows mingw with sh.exe environment, you may need run qmake twice. I have not found out the reason behind this phenomenon.


--------------------------------

#### Visual Studio/MSBuild

I don't put any vs project file in QtAV, because it's easy to create by qmake.  

Open cmd

    qmake -r -tp vc QtAV.pro

Then sln and vcxproj(vcproj) files will be created. Run `msbuild /m` to build the projects. You can also open QtAV.sln in your Visual Studio to Compile it. 

Another solution is using Qt vs plugin. It will help you to load qmake projects(not tested).


#### QtCreator With MSVC

QtCreator will detect VC compiler if it is installed. So it's easy to build in QtCreator

#### Build for Android on Windows

You may get qmake error `libavutil is required, but compiler can not find it`. That's because `mingw32-make.exe` can not be found in the config test step. An workaround is put your `mingw32-make.exe` to one of `%PATH%` dirs

#### Build Debian Packages

run

    debuild -us -uc

in QtAV source tree

#### Link to Static FFmpeg and OpenAL

QtAV >=1.4.2 supports linking to static ffmpeg and openal libs. It's disabled by default. To enable it, add

    CONFIG += static_ffmpeg static_openal

in $QtAV/.qmake.conf for Qt5 or $QtAV_BUILD_DIR/.qmake.cache

#### Ubuntu 12.04 Support

If QtAV, FFmpeg and OpenAL are built on newer OS, some symbols will not be found on 12.04. For example, clock_gettime is in both librt and glibc2.17, we must force the linker link against librt because 12.04 glibc does not have that symbol. add

    CONFIG += glibc_compat

to .qmake.conf or .qmake.cache

#### CI

- [travis ci for linux and OSX](https://travis-ci.org/wang-bin/QtAV)
- [appveyor for mingw and vs2013(msbuild+nmake)](https://ci.appveyor.com/project/wang-bin/qtav)

You can read the build log to see how they work.


### Build Error

- qmake error `Checking for avutil... no`

Make sure ffmpeg headers and libs can be found by compiler. Read config.log in build dir for details.
