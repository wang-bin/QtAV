cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
set(PROJECT_VERSION 1.11.0)
set(SO_VERSION 1.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")


option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_QT5OPENGL "Build with Qt5 OpenGL module" ON)

list(APPEND CMAKE_FIND_ROOT_PATH ${QTDIR})
list(APPEND CMAKE_PREFIX_PATH ${QTDIR})
set(QTCONFIGROOT ${QTDIR}/lib/cmake/Qt5)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)

message(STATUS "Qt version: ${Qt5Core_VERSION_STRING}")
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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)
find_package(Qt5Widgets)
if(Qt5Widgets_FOUND)
  add_subdirectory(widgets)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


