
defineTest(copyFiles){

    file_pathes = $$1

    for(file_path,file_pathes){
        win32: file_path ~= s,/,\\,g
        for(dest_path,destination_pathes){
            win32: dest_path ~= s,/,\\,g

            win32: QMAKE_POST_LINK += $$quote(xcopy $${file_path} $${dest_path} /I /Y $$escape_expand(\n\t))
            else:linux: QMAKE_POST_LINK += $$quote((mkdir -p dirname $${dest_path}) && (cp -f $${file_path} $${dest_path}) $$escape_expand(\n\t))
        }
    }

    export(QMAKE_POST_LINK)
}

win32:{
    exists( $$OUT_PWD/../sdk_install.bat ) {
        QMAKE_POST_LINK += $$quote($$OUT_PWD/../sdk_install.bat  $$escape_expand(\n\t))
    }

    contains(QT_ARCH, i386): archName = 32
    else: archName = 64

    clear(file_pathes)
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/avcodec-58.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/avdevice-58.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/avfilter-7.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/avformat-58.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/avutil-56.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/swresample-3.dll
    file_pathes += $$PWD/ffmpegBin/win/$${archName}/bin/swscale-5.dll

    clear(destination_pathes)
    destination_pathes += $$QT.core.bins

    copyFiles($$file_pathes)
}
else:linux:{
    exists( $$OUT_PWD/../../sdk_install.sh ) {
        QMAKE_POST_LINK += $$quote(chmod +x $$OUT_PWD/../../sdk_install.sh  $$escape_expand(\n\t))
        QMAKE_POST_LINK += $$quote($$OUT_PWD/../../sdk_install.sh  $$escape_expand(\n\t))
    }

    contains(QT_ARCH, i386): archName = 32
    else: archName = 64

    clear(file_pathes)
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libavcodec.so.58
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libavdevice.so.58
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libavfilter.so.7
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libavformat.so.58
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libavutil.so.56
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libswresample.so.3
    file_pathes += $$PWD/ffmpegBin/linux/$${archName}/lib/libswscale.so.5

    clear(destination_pathes)
    destination_pathes += $$QT.core.libs

    copyFiles($$file_pathes)
}
else:android:{
    exists( $$OUT_PWD/../../sdk_install.sh ) {
        QMAKE_POST_LINK += $$quote(chmod +x $$OUT_PWD/../../sdk_install.sh  $$escape_expand(\n\t))
        QMAKE_POST_LINK += $$quote($$OUT_PWD/../../sdk_install.sh  $$escape_expand(\n\t))
    }

    clear(file_pathes)
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libavcodec.so.58
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libavdevice.so.58
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libavfilter.so.7
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libavformat.so.58
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libavutil.so.56
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libswresample.so.3
    file_pathes += $$PWD/ffmpegBin/Android/clang/lib/armeabi-v7a/libswscale.so.5

    clear(destination_pathes)
    destination_pathes += $$QT.core.libs

    copyFiles($$file_pathes)
}
