It's easy to include QtAV in your project. Because it's pro file are well designed. For more information about the pro file i use, see my another project: https://github.com/wang-bin/LibProjWizard4QtCreator

You can see examples in QtAV to know how to use QtAV, or follow the steps below

## Steps
1. Create a subdirs type project in directory myproject  
myproject/myplayer.pro  
`
    TEMPLATE = subdirs
`  
`
    SUBDIRS +=  libQtAV  myplayer
`  
`  
    myplayer.depends += libQtAV
`  
`
    libQtAV.file = QtAV/src/libQtAV.pro
`
2. Clone/Put QtAV to myproject  
the directory now is
> myproject/myplayer/myplayer.pro  
> myproject/QtAV/src/libQtAV.pro  
> myproject/QtAV/src/libQtAV.pri

3. Add libQtAV.pri in you project  
in myproject/myplayer.pro, add  
`
    include(../QtAV/src/libQtAV.pri)
`
4. generate Makefile  

    qmake


>> If you have problem when building with `qmake`, try `qmake -r BUILD_DIR=some_dir`  
>>  YOU MUST use parameter **_-r_** and **_BUILD_DIR=some_dir_**, or the dependence will be broken when building due to my pro structure.  
>>  If you are using QtCreator to build the project, you should go to _Projects_->_Build Steps_->_qmake_->_Additional arguments_, add BUILD_DIR=your/buid/dir  

5. make  
you player binary will be created in $$BUILD_DIR/bin, BUILD_DIR is your shadow build dir. If you are in windows, the QtAV dll also be there

### ISSUES

If you want to build a debug version, you may get some link errors. It's not your problem, my qmake projects have some little problem. Just fix it manually yourself