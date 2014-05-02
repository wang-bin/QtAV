SOURCES = sse2.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
#qt5 only has gcc, qcc, vc, linux icc. clang?
win32-icc: QMAKE_CFLAGS_SSE2 *= -arch:SSE2
isEmpty(QMAKE_CFLAGS_SSE2):error("This compiler does not support SSE2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2
