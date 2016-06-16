include(../paths.pri)

SOURCES += main.cpp

exists(../../contrib/capi/capi.pri) {
  CONFIG = staticlib
} else {
  LIBS += -lass
}
