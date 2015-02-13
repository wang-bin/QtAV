For QtAV version >= 1.3.4, QtAV can be installed as a Qt5 module easily. Integrating QtAV in your project is very easy. 

First, you have to build QtAV. Then go to building directory, you will find `sdk_install.sh` and `sdk_uninstall.sh` (`sdk_install.bat` and `sdk_uninstall.bat` on windows). Run `sdk_install.sh` or `sdk_install.bat` to install QtAV as a Qt module.

To use QtAV, just add the following line in your project

    CONFIG += av

For Qt5, use the following works too

    QT += av

In your C++ files, add

    #include <QtAV/QtAV.h>

In addition, you must copy ffmpeg and portaudio/openal libraries to Qt dir manually. Otherwise your application may not able to run in QtCreator. On windows, put the the dlls in QtDir/bin. On linux, put in QtDir/lib

Since QtAV 1.5 QWidget based renderers are moved to a new module QtAVWidgets. If you want to use QWidget based renderers, for example `OpenGLWidgetRenderer`, in project file add

    QT += avwidgets

In your C++ files, add 

    #include <QtAV>
    #include <QtAVWidgets>

Make sure `QtAV::Widgets::registerRenderers()` is called before creating a renderer.
 
# For QtAV < 1.3.4

It's easy to include QtAV in your project. Because it's pro file are well designed. For more information about the pro file i use, see my another project: https://github.com/wang-bin/LibProjWizard4QtCreator

You can see examples in QtAV to know how to use QtAV, or follow the steps below

## Steps
###1. Create a subdirs type project and a player project

myproject/myproject.pro

    TEMPLATE = subdirs
    SUBDIRS +=  libQtAV  myplayer
    myplayer.depends += libQtAV
    libQtAV.file = QtAV/QtAV.pro
    include(QtAV/root.pri)

###2. Put QtAV to myproject

You can use `git clone git@github.com:wang-bin/QtAV.git` in myproject/, or copy QtAV to myproject/. It's recommend to use git so that you can checkout the latest code easily.

the directory now is

> myproject/myproject.pro  
> myproject/myplayer/myplayer.pro  
> myproject/QtAV/QtAV.pro  
> myproject/QtAV/src/libQtAV.pro  
> myproject/QtAV/src/libQtAV.pri

###3. Add libQtAV.pri in you player project  
in myproject/myplayer/myplayer.pro, add  

    include(../QtAV/src/libQtAV.pri)

###4. generate Makefile

    qmake

or

    qmake -r

###5. make  
you player binary will be created in `bin` under build dir. If you are in windows, the QtAV dll also be there

Note: for windows user, if you run `qmake`(command line build. QtCreator uses `qmake -r` by default) you may run `qmake` twice. otherwise make may fail.
