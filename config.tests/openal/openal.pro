include(../paths.pri)

SOURCES += main.cpp
mac: DEFINES += HEADER_OPENAL_PREFIX

exists(../../contrib/capi/capi.pri) {
  CONFIG = staticlib
} else {
  win32: LIBS += -lOpenAL32
  unix:!mac:!blackberry: LIBS += -lopenal
  blackberry: LIBS += -lOpenAL
  mac: LIBS += -framework OpenAL
}

