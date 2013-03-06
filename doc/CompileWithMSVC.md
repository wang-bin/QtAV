## Build FFmpeg

The difficulty is compiling FFmpeg with msvc. The method can be found on the internet. But there is an easier way. When compiling the latest version of FFmpeg with mingw gcc, the *.lib are also created and installed in /usr/local/bin, which can be linked by msvc.  
For other libraries compiled with mingw gcc, we can link them in a similar way. I introduce how to use the portaudio library created by mingw gcc. If you compile portaudio with mingw gcc, portaudio.lib file will not be created. So we should create it manually. But the def file can be found in lib/.lib, it will not be installed by default. There is a powerful tool named _dlltool_, with which we can create a lib from a def file for vc, using the following command  
`dlltool -m i386 -d libportaudio-2.dll.def -l portaudio.lib -D libportaudio-2.dll`  
Then put portaudio.lib into /usr/local/lib and it will be found when linking.  
The compiled libraries with msvc support can be found [here](https://sourceforge.net/projects/qtav/files/depends)

## Build In Visual Studio

I don't put any vs project file in QtAV, because it's easy to create by qmake.  

Open cmd

    qmake -r BUILD_DIR=%CD%\out -tp vc QtAV.pro

If you are in sh environment

    qmake -r BUILD_DIR=$PWD/out -tp vc QtAV.pro

Then the sln and vcxproj(vcproj) files will be created. Open QtAV.sln in your Visual Studio, Compile it.

Another solution is using Qt vs plugin. It will help you to load qmake projects.

## Build In QtCreator

QtCreator will detect VC compiler if it is installed. So it's easy to build in QtCreator


## Build in Command Line VC

I have got VC compiler and win sdk from latest VS2012 Update1. You can download it from http://qtbuild.googlecode.com/files/vs2012-x86.7z

The environment is small but has the almost complete functionality. At least it can build Qt.

# ISSUES

If you want to build a debug version, you may get some link errors. It's not your problem, my qmake projects have some little problem. Just fix it manually yourself.