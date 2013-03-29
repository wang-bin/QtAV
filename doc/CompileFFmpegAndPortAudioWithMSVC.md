## Build FFmpeg With MSVC

The difficulty is compiling FFmpeg with msvc. The method can be found on the internet. But there is an easier way. When compiling the latest version of FFmpeg with mingw gcc, the *.lib are also created and installed in /usr/local/bin, which can be linked by msvc.  
For other libraries compiled with mingw gcc, we can link them in a similar way. I introduce how to use the portaudio library created by mingw gcc. If you compile portaudio with mingw gcc, portaudio.lib file will not be created. So we should create it manually. But the def file can be found in lib/.lib, it will not be installed by default. There is a powerful tool named _dlltool_, with which we can create a lib from a def file for vc, using the following command  
`dlltool -m i386 -d libportaudio-2.dll.def -l portaudio.lib -D libportaudio-2.dll`  
Then put portaudio.lib into /usr/local/lib and it will be found when linking.  
The compiled libraries with msvc support can be found [here](https://sourceforge.net/projects/qtav/files/depends)


## Build PortAudio

## Build QtAV

https://github.com/wang-bin/QtAV/wiki/Build-QtAV
