SOURCES = sse4_1.cpp
#SSE4_1_SOURCES = sse4_1.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console sse4_1

# for Qt4. we can only detect sse2 in Qt4
#qt5 only has gcc, qcc, vc, linux icc.
win32-icc {
  QMAKE_CFLAGS_SSE4_1 = -arch:SSE4.1
} else:*-icc { #mac, linux
  QMAKE_CFLAGS_SSE4_1 = -xSSE4.1
} else:*msvc* {
  QMAKE_CFLAGS_SSE4_1 = -arch:SSE2
} else {
  QMAKE_CFLAGS_SSE4_1 = -msse4.1
}

sse4_1 {
  HEADERS += $$SSE4_1_HEADERS

  sse4_1_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
  !contains(QT_CPU_FEATURES, sse4_1):sse4_1_compiler.commands += $$QMAKE_CFLAGS_SSE4_1
  sse4_1_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
  sse4_1_compiler.dependency_type = TYPE_C
  sse4_1_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
  sse4_1_compiler.input = SSE4_1_SOURCES
  sse4_1_compiler.variable_out = OBJECTS
  sse4_1_compiler.name = compiling[sse4_1] ${QMAKE_FILE_IN}
  silent:sse4_1_compiler.commands = @echo compiling[sse4_1] ${QMAKE_FILE_IN} && $$sse4_1_compiler.commands
  QMAKE_EXTRA_COMPILERS += sse4_1_compiler
}

isEmpty(QMAKE_CFLAGS_SSE4_1):error("This compiler does not support SSE4.1")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_1
