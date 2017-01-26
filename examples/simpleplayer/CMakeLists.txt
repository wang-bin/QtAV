cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
find_package(QtAV REQUIRED)
find_package(Qt5Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

add_executable(simpleplayer main.cpp playerwindow.cpp playerwindow.h)
target_include_directories(simpleplayer PRIVATE ${QTAVWIDGETS_INCLUDE_DIRS})
target_link_libraries(simpleplayer ${QTAVWIDGETS_LIBRARIES} Qt5::Widgets)