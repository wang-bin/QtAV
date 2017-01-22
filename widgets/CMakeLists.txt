set(MODULE QtAVWidgets)
set(SDK_HEADERS
  QtAVWidgets/global.h
  QtAVWidgets/GLWidgetRenderer2.h #TODO: check opengl
  QtAVWidgets/GraphicsItemRenderer.h
  QtAVWidgets/OpenGLWidgetRenderer.h
  QtAVWidgets/QtAVWidgets
  QtAVWidgets/QtAVWidgets.h
  QtAVWidgets/version.h
  QtAVWidgets/VideoPreviewWidget.h
  QtAVWidgets/WidgetRenderer.h
)
set(HEADERS ${SDK_HEADERS})
list(APPEND SOURCES
  global.cpp
  GLWidgetRenderer2.cpp
  GraphicsItemRenderer.cpp
  OpenGLWidgetRenderer.cpp
  VideoPreviewWidget.cpp
  WidgetRenderer.cpp
)
# add HEADERS for moc
add_library(${MODULE} SHARED ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS})
qt5_use_modules(${MODULE} LINK_PUBLIC Core Gui Widgets OpenGL) #TODO: check OpenGL
target_link_libraries(${MODULE} LINK_PRIVATE ${EXTRA_LIBS})
target_link_libraries(${MODULE} LINK_PUBLIC QtAV)
set_target_properties(${MODULE} PROPERTIES
  MACOSX_RPATH ON
  FRAMEWORK ON
  VERSION ${PROJECT_VERSION}
  SOVERSION ${SO_VERSION}
  OUTPUT_NAME ${MODULE}
  CLEAN_DIRECT_OUTPUT 1
  #LINK_SEARCH_START_STATIC 1
)


#http://stackoverflow.com/questions/19866424/cmake-and-qt5-automoc-error
#http://doc.qt.io/qt-5/cmake-manual.html
set(CMAKE_INCLUDE_CURRENT_DIR ON) # for .moc
target_include_directories(${MODULE}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_SOURCE_DIR}/src
  PRIVATE ${EXTRA_INCLUDE}
  )

target_compile_definitions(QtAV
  PRIVATE ${EXTRA_DEFS}
)