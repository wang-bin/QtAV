SOURCES = sse4_1.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
#qt5 only has gcc, qcc, vc, linux icc. clang?
win32-icc: QMAKE_CFLAGS_SSE4_1 *= -arch:SSE4.1
isEmpty(QMAKE_CFLAGS_SSE4_1):error("This compiler does not support SSE4.1")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_1
