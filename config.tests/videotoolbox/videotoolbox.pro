DEFINES += __STDC_CONSTANT_MACROS
SOURCES += main.cpp

LIBS += -lavcodec -lavutil -framework CoreVideo -framework CoreFoundation -framework CoreMedia -framework VideoToolbox
include(../paths.pri)
