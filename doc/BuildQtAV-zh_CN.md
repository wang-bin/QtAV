## 1. Setup the environment

QtAV 依赖 FFmpeg, PortAudio，以及一些可选的库如direct2d, xvideo
可以从这里下载windows下FFmpeg和PortAudio开发用的文件 [QtAV sourceforge page](https://sourceforge.net/projects/qtav/files/depends)
windows 下的 FFmpeg 也可以从这里下载 [Zeranoe](http://ffmpeg.zeranoe.com/builds)
Or you can build them your self [Compile FFmpeg and PortAudio](https://github.com/wang-bin/QtAV/wiki/Compile-FFmpeg-and-PortAudio)

首先你 *必须* 让编译器能找到 FFmpeg 的头文件和库文件.， 否则在 qmake 时会出错。如果你已经把那些文件放到该放的位置了，可以忽略这步。

vc 编译器会在环境变量 __*INCLUDE*__ 制定的那些目录搜索头文件， __*LIB*__ 制定的目录搜索库文件，因此如果你使用命令行编译的话可以这样设置环境

    set INCLUDE=ffmpeg_path\include;portaudio_path\include;%INCLUDE%
    set LIB=ffmpeg_path\lib;portaudio_path\lib;%LIB%


GCC 会在环境变量 __*CPATH*__ 搜索头文件， __*LIBRARY_PATH*__ 里搜索库文件。因此你可以设置这些变量来包含 FFmpeg 和 PortAudio 相关的路径。
unix shell环境下的 gcc (也包括环境中有sh.exe的mingw环境):

    export CPATH=ffmpeg_path/include:portaudio_path/include:$CPATH
    export LIBRARY_PATH=ffmpeg_path/include:portaudio_path/lib:$LIBRARY_PATH

由于包含 libQtAV.pri 的工程不会添加 FFmpeg 等相关的链接参数，所以链接器可能会从 $LD_LIBRARY_PATH 中去找 QtAV 库的依赖库:

    export LD_LIBRARY_PATH=ffmpeg_path/lib:portaudio_path/lib:$LD_LIBRARY_PATH

windows 无sh.exe的环境下的 gcc

    set CPATH=ffmpeg_path\include;portaudio_path\include;%CPATH%
    set LIBRARY_PATH=ffmpeg_path\lib;portaudio_path\lib;%LIBRARY_PATH%

如果使用 QtCreator 进行编译, 打开左边的 '工程' 页面，添加或追加相应的环境变量就行

## 2. qmake

对于大多数系统, 只要

    qmake
    make

强烈建议不要在源码目录编译，而是使用如下的方法 

    cd your_build_dir
    qmake QtAV_source_dir/QtAV.pro
    make

qmake 在第一次运行的时候会检测所依赖的库, 你要保证这些库能被找到。
然后 qmake 会在编译目录生成一个 cache 文件 _.qmake.cache_ . cache 文件包含了检测结果，比如 portaudio 是否支持。 如果你想重新检测, 则可以删除 _**.qmake.cache**_ 再运行 qmake， 也可以直接给 qmake 加个额外参数

    qmake QtAV_source_dir/QtAV.pro  CONFIG+=recheck


_WARNING_: If you are in windows mingw with sh.exe environment, you may need run qmake twice. I have not find out the reason!

## 3. Make

使用 make, jom, nmake 或者 QtCreator 进行编译.



## Windows 下的编译

你必须在qmake前配置好环境，如最开始所说的。

#### Visual Studio

我没有在 QtAV 里放任何 vs 的工程文件，因为这些工程很容易由 qmake 生成

打开命令行

    qmake -r -tp vc QtAV.pro

然后 sln 和 vcxproj(vcproj) 文件会创建. 用 Visual Studio 打开 QtAV.sln 进行编译.

另外你也可以使用 Qt vs plugin 来导入qmake 工程（未测试）

#### QtCreator 里使用 MSVC

QtCreator 会检测 VC 编译器，编译过程和 gcc 的差不多，很简单。


#### VC 命令行下编译

我从  VS2012 Update1 中提取了 VC 编译器和 windows sdk. 可以从这里下载 http://qtbuild.googlecode.com/files/vs2012-x86.7z

这个编译环境很精简但是开发C++的功能很完整，至少能用它成功编译 Qt。
