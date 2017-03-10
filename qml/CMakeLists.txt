set(MODULE QmlAV)
set(EXTRA_DEFS -DBUILD_QMLAV_LIB)

file(GLOB SDK_PRIVATE_HEADERS QtAVWidgets/*.h)
#URI = QtAV #uri used in QtAVQmlPlugin::registerTypes(uri)
#qtAtLeast(5, 3): QMAKE_MOC_OPTIONS += -Muri=$$URI # not sure what moc does
set(HEADERS
    QmlAV/Export.h
    QmlAV/MediaMetaData.h
    QmlAV/SGVideoNode.h
    QmlAV/QQuickItemRenderer.h
    QmlAV/QuickFilter.h
    QmlAV/QmlAVPlayer.h
)
set(SOURCES
    plugin.cpp
    QQuickItemRenderer.cpp
    SGVideoNode.cpp
    QmlAVPlayer.cpp
    QuickFilter.cpp
    QuickSubtitle.cpp
    MediaMetaData.cpp
    QuickSubtitleItem.cpp
    QuickVideoPreview.cpp
)
if(NOT Qt5Qml_VERSION VERSION_LESS 5.2.0)
  list(APPEND HEADERS QmlAV/QuickFBORenderer.h)
  list(APPEND SOURCES QuickFBORenderer.cpp)
endif()
# add HEADERS for moc
add_library(${MODULE} MODULE ${SOURCES} ${RESOURCES_SOURCES} ${HEADERS})
target_link_libraries(${MODULE}
  LINK_PRIVATE ${EXTRA_LIBS}
  LINK_PUBLIC QtAV Qt5::Qml Qt5::Quick
)
set_target_properties(${MODULE} PROPERTIES
  OUTPUT_NAME ${MODULE}
  CLEAN_DIRECT_OUTPUT 1
  #LINK_SEARCH_START_STATIC 1
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/QtAV
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

install(TARGETS ${MODULE}
  EXPORT ${MODULE}-targets
  RUNTIME DESTINATION ${QTAV_INSTALL_QML}/QtAV
  LIBRARY DESTINATION ${QTAV_INSTALL_QML}/QtAV
  ARCHIVE DESTINATION ${QTAV_INSTALL_QML}/QtAV
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
)
install(EXPORT ${MODULE}-targets
  DESTINATION ${QTAV_INSTALL_LIBS}/cmake/${MODULE}
  FILE ${MODULE}-config.cmake
)
