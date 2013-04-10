It's easy to include QtAV in your project. Because it's pro file are well designed. For more information about the pro file i use, see my another project: https://github.com/wang-bin/LibProjWizard4QtCreator

You can see examples in QtAV to know how to use QtAV, or follow the steps below

## Steps
###1. Create a subdirs type project and a player project

myproject/myproject.pro

    TEMPLATE = subdirs
    SUBDIRS +=  libQtAV  myplayer
    myplayer.depends += libQtAV
    libQtAV.file = QtAV/src/libQtAV.pro

###2. Put QtAV to myproject

You can use `git clone git@github.com:wang-bin/QtAV.git` in myproject/, or copy QtAV to myproject/. It's recommend to use git so that you can checkout the latest code easily.

the directory now is

> myproject/myproject.pro  
> myproject/myplayer/myplayer.pro  
> myproject/QtAV/src/libQtAV.pro  
> myproject/QtAV/src/libQtAV.pri

###3. Add libQtAV.pri in you player project  
in myproject/myplayer/myplayer.pro, add  

    include(../QtAV/src/libQtAV.pri)

###4. generate Makefile

`qmake -r BUILD_DIR=some_dir`

  YOU MUST use parameter **_-r_** and **_BUILD_DIR=some_dir_**, or the dependence will be broken when building due to my pro structure.

  If you are using QtCreator to build the project, you should go to _Projects_->_Build Steps_->_qmake_->_Additional arguments_, add BUILD_DIR=your/buid/dir

###5. make  
you player binary will be created in $$BUILD_DIR/bin. If you are in windows, the QtAV dll also be there