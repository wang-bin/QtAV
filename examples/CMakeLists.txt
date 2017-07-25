find_package(Qt5Sql REQUIRED)
if(WIN32)
  set(EXE_TYPE WIN32)
elseif(APPLE)
  #set(EXE_TYPE MACOSX_BUNDLE)
endif()
set(COMMON_SRC
  common/common.cpp
  common/Config.cpp
  common/qoptions.cpp
)
set(COMMON_HEADERS
  common/common.h
  common/Config.h
  common/qoptions.h
  common/ScreenSaver.h
  common/common_export.h
)
if(NOT IOS)
  list(APPEND COMMON_SRC common/ScreenSaver.cpp)
  if(APPLE)
    list(APPEND COMMON_LIBS "-framework CoreServices")
  endif()
endif()

SET(COMMON_QRC common/theme/theme.qrc)
SOURCE_GROUP("Resources" FILES ${COMMON_QRC})
qt5_add_resources(COMMON_RES ${COMMON_QRC})
set_source_files_properties(${COMMON_RES} PROPERTIES GENERATED ON)

# add HEADERS for moc
add_library(common STATIC ${COMMON_SRC} ${COMMON_RES} ${COMMON_HEADERS})
target_link_libraries(common
  LINK_PUBLIC Qt5::Core Qt5::Gui ${COMMON_LIBS}
  LINK_PRIVATE Qt5::Sql
)
target_compile_definitions(common PUBLIC -DBUILD_COMMON_STATIC)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(PLAYER_SRC
    player/main.cpp
    player/MainWindow.cpp
    player/ClickableMenu.cpp
    player/StatisticsView.cpp
    player/Slider.cpp
    player/TVView.cpp
    player/EventFilter.cpp
    player/config/ConfigDialog.cpp
    player/config/ConfigPageBase.cpp
    player/config/CaptureConfigPage.cpp
    player/config/VideoEQConfigPage.cpp
    player/config/DecoderConfigPage.cpp
    player/config/MiscPage.cpp
    player/filters/OSD.cpp
    player/filters/OSDFilter.cpp
    player/playlist/PlayListModel.cpp
    player/playlist/PlayListItem.cpp
    player/playlist/PlayListDelegate.cpp
    player/playlist/PlayList.cpp
    player/config/PropertyEditor.cpp
    player/config/AVFormatConfigPage.cpp
    player/config/AVFilterConfigPage.cpp
    player/config/ShaderPage.cpp
    player/filters/AVFilterSubtitle.cpp
)
set(PLAYER_HEADERS
    player/MainWindow.h
    player/ClickableMenu.h
    player/StatisticsView.h
    player/Slider.h
    player/TVView.h
    player/EventFilter.h
    player/config/ConfigDialog.h
    player/config/ConfigPageBase.h
    player/config/CaptureConfigPage.h
    player/config/VideoEQConfigPage.h
    player/config/DecoderConfigPage.h
    player/config/MiscPage.h
    player/filters/OSD.h
    player/filters/OSDFilter.h
    player/playlist/PlayListModel.h
    player/playlist/PlayListItem.h
    player/playlist/PlayListDelegate.h
    player/playlist/PlayList.h
    player/config/PropertyEditor.h
    player/config/AVFormatConfigPage.h
    player/config/AVFilterConfigPage.h
    player/config/ShaderPage.h
    player/filters/AVFilterSubtitle.h
)

SET(PLAYER_QRC player/res/player.qrc)
SOURCE_GROUP("Resources" FILES ${PLAYER_QRC})
qt5_add_resources(PLAYER_RES ${PLAYER_QRC})
set_source_files_properties(${PLAYER_RES} PROPERTIES GENERATED ON)

if(Qt5Quick_FOUND)
  set(QMLPLAYER_QRC QMLPlayer/qmlplayer.qrc)
  qt5_add_resources(QMLPLAYER_RES ${QMLPLAYER_QRC})
  set_source_files_properties(${QMLPLAYER_RES} PROPERTIES GENERATED ON)
  set(MODULE QMLPlayer)
  if(WIN32)
    set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.rc)
    configure_file(${QTAV_SOURCE_DIR}/cmake/QtAV.rc.in ${RC_FILE})
  endif()
  add_executable(QMLPlayer ${EXE_TYPE}
      QMLPlayer/main.cpp
      QMLPlayer/qtquick2applicationviewer/qtquick2applicationviewer.cpp
      QMLPlayer/qtquick2applicationviewer/qtquick2applicationviewer.h
      ${QMLPLAYER_RES}
      ${RC_FILE}
      )
  target_include_directories(QMLPlayer PRIVATE QMLPlayer/qtquick2applicationviewer)
  target_link_libraries(QMLPlayer Qt5::Quick common)
  install(TARGETS QMLPlayer
    RUNTIME DESTINATION ${QTAV_INSTALL_BINS}
  )
endif()
if(Qt5Widgets_FOUND)
  set(MODULE Player)
  if(WIN32)
    set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE}.rc)
    configure_file(${QTAV_SOURCE_DIR}/cmake/QtAV.rc.in ${RC_FILE})
  endif()
  add_executable(Player ${EXE_TYPE} ${PLAYER_SRC} ${PLAYER_HEADERS} ${PLAYER_RES} ${RC_FILE})
  target_link_libraries(Player QtAVWidgets common)

  install(TARGETS Player
    RUNTIME DESTINATION ${QTAV_INSTALL_BINS}
  )
  if(BUILD_EXAMPLES)
    SET(FILTERS_QRC filters/res.qrc)
    SOURCE_GROUP("Resources" FILES ${FILTERS_QRC})
    qt5_add_resources(FILTERS_RES ${FILTERS_QRC})
    set_source_files_properties(${FILTERS_RES} PROPERTIES GENERATED ON)
    add_executable(filters filters/main.cpp filters/SimpleFilter.cpp ${FILTERS_RES})
    target_link_libraries(filters QtAVWidgets)

    add_executable(glslfilter glslfilter/main.cpp)
    target_link_libraries(glslfilter QtAVWidgets)

    add_executable(shader shader/main.cpp)
    target_link_libraries(shader QtAVWidgets)

    add_executable(sharedoutput sharedoutput/main.cpp sharedoutput/widget.cpp)
    target_link_libraries(sharedoutput QtAVWidgets)

    add_executable(simpleplayer simpleplayer/main.cpp simpleplayer/playerwindow.cpp)
    target_link_libraries(simpleplayer QtAVWidgets)

    add_executable(videocapture videocapture/main.cpp videocapture/playerwindow.cpp)
    target_link_libraries(videocapture QtAVWidgets)

    add_executable(videographicsitem videographicsitem/main.cpp videographicsitem/videoplayer.cpp)
    target_link_libraries(videographicsitem QtAVWidgets)

    add_executable(videogroup videogroup/main.cpp videogroup/videogroup.cpp)
    target_link_libraries(videogroup QtAVWidgets)

    add_executable(videowall videowall/main.cpp videowall/VideoWall.cpp)
    target_link_libraries(videowall QtAVWidgets)

    add_executable(simpletranscode simpletranscode/main.cpp)
    target_link_libraries(simpletranscode QtAV)
  endif()
endif()

if(BUILD_EXAMPLES)
  add_executable(audiopipeline audiopipeline/main.cpp)
  target_link_libraries(audiopipeline QtAV)

  add_executable(framereader framereader/main.cpp)
  target_link_libraries(framereader QtAV)

  add_executable(window window/main.cpp)
  target_link_libraries(window QtAV)
endif()

