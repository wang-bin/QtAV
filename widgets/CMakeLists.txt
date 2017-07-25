set(MODULE QtAVWidgets)
set(EXTRA_DEFS -DBUILD_QTAVWIDGETS_LIB)
list(APPEND EXTRA_DEFS -DQTAV_HAVE_GL=1)

set(HAVE_QT5OPENGL 0)
if(BUILD_QT5OPENGL)
  find_package(Qt5OpenGL)
  if(Qt5OpenGL_FOUND)
    set(HAVE_QT5OPENGL 1)
  endif()
endif()

set(SDK_HEADERS
  QtAVWidgets/global.h
  QtAVWidgets/GraphicsItemRenderer.h
  QtAVWidgets/OpenGLWidgetRenderer.h
  QtAVWidgets/QtAVWidgets
  QtAVWidgets/QtAVWidgets.h
  QtAVWidgets/version.h
  QtAVWidgets/VideoPreviewWidget.h
  QtAVWidgets/WidgetRenderer.h
)
file(GLOB SDK_PRIVATE_HEADERS QtAVWidgets/*.h)

set(SOURCES
  global.cpp
  GraphicsItemRenderer.cpp
  OpenGLWidgetRenderer.cpp
  VideoPreviewWidget.cpp
  WidgetRenderer.cpp
)
if(HAVE_QT5OPENGL)
  list(APPEND SDK_HEADERS QtAVWidgets/GLWidgetRenderer2.h)
  list(APPEND SOURCES GLWidgetRenderer2.cpp)
endif()

# TODO: Qt5Gui_OPENGL_IMPLEMENTATION?
if(Qt5Gui_VERSION VERSION_LESS 5.4.0)
  list(APPEND SDK_HEADERS QtAVWidgets/QOpenGLWidget.h)
  list(APPEND SOURCES QOpenGLWidget.cpp)
endif()

set(HEADERS ${SDK_HEADERS})
if(WIN32)
  set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.rc)
  configure_file(${QTAV_SOURCE_DIR}/cmake/QtAV.rc.in ${RC_FILE})
endif()
# add HEADERS for moc
add_library(${MODULE} SHARED ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS} ${RC_FILE})
target_link_libraries(${MODULE}
  LINK_PRIVATE ${EXTRA_LIBS}
  LINK_PUBLIC QtAV Qt5::Widgets
)
if(HAVE_QT5OPENGL)
  target_link_libraries(${MODULE} LINK_PUBLIC Qt5::OpenGL)
endif()
set_target_properties(${MODULE} PROPERTIES
  MACOSX_RPATH ON
  FRAMEWORK ON
  VERSION ${PROJECT_VERSION}
  SOVERSION ${SO_VERSION}
  OUTPUT_NAME ${MODULE}
  CLEAN_DIRECT_OUTPUT 1
  #LINK_SEARCH_START_STATIC 1
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)


#http://stackoverflow.com/questions/19866424/cmake-and-qt5-automoc-error
#http://doc.qt.io/qt-5/cmake-manual.html
set(CMAKE_INCLUDE_CURRENT_DIR ON) # for .moc
target_include_directories(${MODULE}
  PUBLIC $<INSTALL_INTERFACE:include>
  PUBLIC $<INSTALL_INTERFACE:include/${MODULE}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}>
  PRIVATE ${EXTRA_INCLUDE}
  )
target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})


install(FILES
  ${SDK_HEADERS}
  DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}
)
install(FILES
  ${SDK_PRIVATE_HEADERS}
  DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}/private
)
install(TARGETS ${MODULE}
  EXPORT ${MODULE}-targets
  #PUBLIC_HEADER DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}
  #PRIVATE_HEADER DESTINATION ${QTAV_INSTALL_HEADERS}/${MODULE}/private
  RUNTIME DESTINATION ${QTAV_INSTALL_BINS}
  LIBRARY DESTINATION ${QTAV_INSTALL_LIBS}
  ARCHIVE DESTINATION ${QTAV_INSTALL_LIBS}
  FRAMEWORK DESTINATION ${QTAV_INSTALL_LIBS}
)
install(EXPORT ${MODULE}-targets
  DESTINATION ${QTAV_INSTALL_LIBS}/cmake/${MODULE}
  FILE ${MODULE}-config.cmake
)
