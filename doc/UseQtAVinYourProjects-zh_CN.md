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