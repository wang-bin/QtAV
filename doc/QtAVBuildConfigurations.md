Since QtAV 1.3.1, you can configure how to build QtAV in a easier way.

When running qmake, qmake will do some tests to check the building environment and the features can be enabled. The default behavior is to check all the features in both `$$EssentialDepends` and `$$OptionalDepends`(see QtAV.pro) and to build all parts in source tree including examples, tests. Now you can simply select what you want to check and build using a *user.conf* in QtAV source directory.


Disable Features Manually
=========================
Currently features in `$$EssentialDepends` will always be checked: avutil, avcodec, avformat, swscale, swresample, avresample, gl. Features in `$$OptionalDepends` will also be checked by default.

Features in `$$OptionalDepends` can be disabled by adding a CONFIG value in *QtAV_SRC_DIR/user.conf*(create it if not exists). For example, to disable vaapi, just add:

    CONFIG += no-vaapi

All options available are: no-openal, no-portaudio, no-direct2d, no-gdiplus, no-dxva, no-xv, no-vaapi, no-cedarv.


Disable config.tests
====================

config.tests will check what features QtAV can support. It may takes a few seconds. Some you may skip these checking to save time. You can add

    CONFIG += no_config_tests

in *QtAV_SRC/user.conf* to disable it then qmake will be faster. But you then must enable the features you want manually in *user.conf*. All features available are `$$OptionalDepends`. For example, to enable *avresample*, add

    CONFIG += config_avresample

in *user.conf*.

Link Against Static FFmpeg
==========================

It is supported since 1.4.2. You can add `CONFIG += static_ffmpeg` in QTAV_BUILD_DIR/.qmake.cache. Additional link flags will be added. Only ffmpeg build with QtAV/script/build_ffmpeg.sh is tested.

Old GLibc Compatibility
======================

QtAV built from a newer linux system like ubuntu 14.04 may not work on old system like ubuntu 12.04 because of some symbols is missing. For example, `clock_get_time` is in glibc>=2.17 and librt. An convenience option is added since 1.4.2. Just add `CONFIG += glibc_compat` in QTAV_BUILD_DIR/.qmake.cache


TODO
====
more flexible configuration, such as build path, blablabla.