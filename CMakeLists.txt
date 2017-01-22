cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(PROJECT_VERSION 1.11.0)
set(SO_VERSION 1.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")



list(APPEND CMAKE_FIND_ROOT_PATH ${QTDIR})
list(APPEND CMAKE_PREFIX_PATH ${QTDIR})
set(QTCONFIGROOT ${QTDIR}/lib/cmake/Qt5)

find_package(Qt5Core REQUIRED)

message(STATUS "Qt version: ${Qt5Core_VERSION_STRING}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QT5_CFLAGS}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(POLICY CMP0063) # visibility. since 3.3
  cmake_policy(SET CMP0063 NEW)
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden) #use with -fdata-sections -ffunction-sections to reduce target size
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_AUTOMOC ON)

add_subdirectory(src)
find_package(Qt5Widgets)
if(Qt5Widgets_FOUND)
  add_subdirectory(widgets)
endif()

#set(CMAKE_REQUIRED_INCLUDES ${Qt5WebEngine_INCLUDE_DIRS};${Qt5WebEngine_PRIVATE_INCLUDE_DIRS})
#set(CMAKE_REQUIRED_LIBRARIES ${QT5_LIBRARIES})
