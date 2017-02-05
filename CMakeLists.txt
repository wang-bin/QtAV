cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
set(PROJECT_VERSION 1.11.0)
set(SO_VERSION 1.11)

option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_QT5OPENGL "Build with Qt5 OpenGL module" ON)

list(APPEND CMAKE_FIND_ROOT_PATH ${QTDIR})

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
get_target_property(qmake Qt5::qmake LOCATION)
execute_process(
    COMMAND ${qmake} -query QT_INSTALL_HEADERS
    OUTPUT_VARIABLE QT_INSTALL_HEADERS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${qmake} -query QT_INSTALL_LIBS
    OUTPUT_VARIABLE QT_INSTALL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${qmake} -query QT_INSTALL_BINS
    OUTPUT_VARIABLE QT_INSTALL_BINS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${qmake} -query QT_INSTALL_PREFIX
    OUTPUT_VARIABLE QT_INSTALL_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QT5_CFLAGS}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(POLICY CMP0063) # visibility. since 3.3
  cmake_policy(SET CMP0063 NEW)
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden) #use with -fdata-sections -ffunction-sections to reduce target size
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON) # for .moc
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${QT_INSTALL_PREFIX} CACHE PATH "default install path" FORCE)
  set(QTAV_INSTALL_HEADERS ${QT_INSTALL_HEADERS} CACHE PATH "")
  set(QTAV_INSTALL_LIBS ${QT_INSTALL_LIBS} CACHE PATH "")
  set(QTAV_INSTALL_BINS ${QT_INSTALL_BINS} CACHE PATH "")
else()
  set(QTAV_INSTALL_HEADERS ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "")
  set(QTAV_INSTALL_LIBS ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "")
  set(QTAV_INSTALL_BINS ${CMAKE_INSTALL_PREFIX}/bin CACHE PATH "")
endif()

message(STATUS "Qt version: ${Qt5Core_VERSION_STRING}")
message(STATUS "Qt prefix: ${QT_INSTALL_PREFIX}")
message(STATUS "QtAV headers prefix: ${QTAV_INSTALL_HEADERS}")
message(STATUS "QtAV libs prefix: ${QTAV_INSTALL_LIBS}")
message(STATUS "QtAV bins prefix: ${QTAV_INSTALL_BINS}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#set(CMAKE_DEBUG_POSTFIX d)

install(FILES
  cmake/FindQtAV.cmake
  DESTINATION ${QT_INSTALL_LIBS}/cmake
)

add_subdirectory(src)
find_package(Qt5Widgets)
if(Qt5Widgets_FOUND)
  add_subdirectory(widgets)
endif()


if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


