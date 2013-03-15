TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog
OTHER_FILES += templates/vo.h templates/vo.cpp templates/COPYRIGHT.h templates/mkclass.sh
OTHER_FILES += \
	templates/base.h templates/base.cpp templates/base_p.h \
	templates/derived.h templates/derived.cpp templates/derived_p.h \
	templates/final.h templates/final.cpp
#OTHER_FILES += config.test/mktest.sh

#cache mkspecs. compare mkspec with cached one. if not equal, remove old cache to run new compile tests
mkspecs_build = $$[QMAKE_MKSPECS]
mkspecs_build ~= s,\\\\,/,g #avoid warning for '\'. qmake knows how to deal with '/'
!isEmpty(mkspecs_cached) {
    !isEqual(mkspecs_cached, $$mkspecs_build):new_compile_tests = 1
} else {
    new_compile_tests = 1
}

##TODO: BUILD_DIR=>BUILD_ROOT
#if not empty, it means the parent project may already set it
isEmpty(out_dir):out_dir = $$OUT_PWD
out_dir ~= s,\\\\,/,g #avoid warning for '\'. qmake knows how to deal with '/'
isEmpty(SOURCE_ROOT):SOURCE_ROOT = $$PWD
SOURCE_ROOT ~= s,\\\\,/,g #avoid warning for '\'. qmake knows how to deal with '/'
isEmpty(BUILD_DIR):BUILD_DIR=$$out_dir
message("BUILD_DIR=$$BUILD_DIR")

greaterThan(QT_MAJOR_VERSION, 5) {
    !isEmpty(new_compile_tests):write_file($$BUILD_DIR/.qmake.cache)
    load(configure)
} else {
    _QMAKE_CACHE_QT4_ = $$_QMAKE_CACHE_
    #_QMAKE_CACHE_QT4_ is built in and always not empty
    isEmpty(_QMAKE_CACHE_QT4_)|isEqual(_QMAKE_CACHE_QT4_,) {
        message("No .qmake.cache found")
        _QMAKE_CACHE_QT4_=$$BUILD_DIR/.qmake.cache
    }
    message("_QMAKE_CACHE_QT4_: $$_QMAKE_CACHE_QT4_")
    include(common.pri)
    !isEmpty(new_compile_tests):write_file($$BUILD_DIR/.qmake.cache)
    #use the following lines when building as a sub-project, write cache to this project src dir.
    #if build this project alone and do not have sub-project depends on this lib, those lines are not necessary
    ####ASSUME compile tests and .qmake.cache is in project out root dir
    #vars in .qmake.cache will affect all projects in subdirs, even if qmake's working dir is not in .qmake.cache dir
    #write_file($$BUILD_DIR/.qmake.cache) ##TODO: erase the existing lines!!
    include(configure.prf)
#clear config.log iff reconfigure is required
    write_file($$QMAKE_CONFIG_LOG)
    #cache() is available after include configure.prf
    #load(configure.prf) #what's the difference?
    message("cache: $$_QMAKE_CACHE_QT4_")
}
cache(BUILD_DIR, set, BUILD_DIR)
#cache(BUILD_ROOT, set, BUILD_DIR)
cache(SOURCE_ROOT, set, SOURCE_ROOT)
cache(mkspecs_cached, set, mkspecs_build)

qtCompileTest(avutil)|error("FFmpeg avutil is required, but compiler can not find it")
qtCompileTest(avcodec)|error("FFmpeg avcodec is required, but compiler can not find it")
qtCompileTest(avformat)|error("FFmpeg avformat is required, but compiler can not find it")
qtCompileTest(swscale)|error("FFmpeg swscale is required, but compiler can not find it")
qtCompileTest(portaudio)|warning("PortAudio is not available. No audio output in QtAV")
qtCompileTest(direct2d)
qtCompileTest(gdiplus)
#qtCompileTest(openal)
