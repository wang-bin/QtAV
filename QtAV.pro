TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog
OTHER_FILES += templates/vo.h templates/vo.cpp templates/COPYRIGHT.h templates/mkclass.sh


#cache mkspecs. compare mkspec with cached one. if not equal, remove old cache to run new compile tests
!isEmpty(mkspecs_cached):!isEqual(mkspecs_cached, $$QMAKE_MKSPECS):new_compile_tests = 1

out_dir = $$OUT_PWD
greaterThan(QT_MAJOR_VERSION, 5): {
!isEmpty(new_compile_tests):write_file($$OUT_PWD/.qmake.cache)
load(configure)
} else {
include(common.pri)
!isEmpty(new_compile_tests):write_file($$OUT_PWD/.qmake.cache)
#use the following lines when building as a sub-project, write cache to this project src dir.
#if build this project alone and do not have sub-project depends on this lib, those lines are not necessary
####ASSUME compile tests and .qmake.cache is in project out root dir
#vars in .qmake.cache will affect all projects in subdirs, even if qmake's working dir is not in .qmake.cache dir
write_file($$OUT_PWD/.qmake.cache) ##TODO: erase the existing lines!!
include(configure.prf)
#cache() is available after include configure.prf
#load(configure.prf) #what's the difference?
message("cache: $$_QMAKE_CACHE_")
}
cache(BUILD_DIR, set, out_dir)

#qtCompileTest(ffmpeg)|error("FFmpeg is required, but not available")
#qtCompileTest(portaudio)|warning("PortAudio is not available. No audio output in QtAV")
#qtCompileTest(gdiplus)
#qtCompileTest(direct2d)
#qtCompileTest(openal)
