TARGET = arch
SOURCES = arch.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console warn_on

win32-icc {
  QMAKE_CXXFLAGS *= -arch:SSE4.1 #AVX
} *msvc* {

} else {
## gcc like. can not add here otherwise other archs can not be detected
#  QMAKE_CXXFLAGS *= -msse4.1
}

arch_pp.target = preprocess
arch_pp.commands = $$QMAKE_CXX \$<  #for gnu make
arch_pp.depends = $$PWD/arch.h      #TODO: win path. shell_path()?

QMAKE_EXTRA_TARGETS = arch_pp
