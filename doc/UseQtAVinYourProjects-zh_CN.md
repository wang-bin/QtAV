QtAV 1.3.4 之后可以很方便地使之作为一个Qt模块，就像其他Qt模块那样。首先要编译QtAV，编译好后在编译目录会生成安装为Qt模块的脚本 `sdk_install.sh` 和 `sdk_uninstall.sh`，windows为`sdk_install.bat` 和 `sdk_uninstall.bat`，运行`sdk_install.sh` 或`sdk_install.bat`进行安装。安装后要使用QtAV只要在你的qmake工程里加上

    CONFIG += av

就行。对于Qt5，也可以使用

    QT += av

在 C++ 文件中，加入

    #include <QtAV/QtAV.h>

自 QtAV 1.5 基于 QWidget 的渲染器被移到了新模块 QtAVWidgets, 若需要使用这些渲染器比如 OpenGLWidgetRenderer，在工程文件加入

    QT += avwidgets

C++ 代码中加入

    #include <QtAV>
    #include <QtAVWidgets>

请确保在使用渲染器前调用`QtAV::Widgets::registerRenderers()`

## QtAV < 1.3.4 

在你的项目中包含 QtAV 非常容易. 因为 QtAV 的qmake工程是精心设计的. （可以参考: https://github.com/wang-bin/LibProjWizard4QtCreator）

你可以参考 QtAV 里的例子来了解如何使用 QtAV, 或者也可以使用以下步骤


###1. 新建一个 subdirs 类型的工程myproject及一个直接调用QtAV的player子工程

myproject/myproject.pro

    TEMPLATE = subdirs
    SUBDIRS +=  libQtAV  myplayer
    myplayer.depends += libQtAV
    libQtAV.file = QtAV/QtAV.pro
    include(QtAV/root.pri)

###2. 把 QtAV 放到myproject

可以在 myproject/ 目录下使用 `git clone git@github.com:wang-bin/QtAV.git`, 或者复制 QtAV 代码到 myproject/. 建议使用git，这样方便获取最新代码.

现在的目录变为

> myproject/myproject.pro  
> myproject/myplayer/myplayer.pro  
> myproject/QtAV/QtAV.pro  
> myproject/QtAV/src/libQtAV.pro  
> myproject/QtAV/src/libQtAV.pri

###3. 在player工程中包含 libQtAV.pri
  
在 myproject/myplayer/myplayer.pro, add  

    include(../QtAV/src/libQtAV.pri)

###4. 生成 Makefile

    qmake

or

    qmake -r

###5. make  
player 会生成在编译目录下的bin目录. 在windows下, QtAV 的 dll文件也会在那里生成

注意：windows 下如果用`qmake`命令（命令行下。QtCreator 默认使用`qmake -r`），则可能需要运行`qmake`两次，否则 make 可能失败