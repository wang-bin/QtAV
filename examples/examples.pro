TEMPLATE = subdirs

SUBDIRS = common
!android:!ios:!winrt {
  SUBDIRS += audiopipeline
!no-widgets {
  SUBDIRS += \
    sharedoutput \
    simpletranscode \
    simpleplayer \
    player \
    filters \
    framereader \
    videocapture \
    videographicsitem \
    videogroup \
    videowall
contains(QT_CONFIG, opengl): SUBDIRS += \
    shader \
    glslfilter

  player.depends += common

  sdk_build {
    SUBDIRS *= \
        simpleplayer/simpleplayer_sdk.pro \
        player/player_sdk.pro
  }
}
}

greaterThan(QT_MAJOR_VERSION, 4) {
  isEqual(QT_MAJOR_VERSION,5):greaterThan(QT_MINOR_VERSION,3)|greaterThan(QT_MAJOR_VERSION,5) {
    contains(QT_CONFIG, opengl):!winrt:!ios:!android: SUBDIRS += window
  }
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += QMLPlayer
    QMLPlayer.depends += common
    sdk_build: SUBDIRS *= QMLPlayer/QMLPlayer_sdk.pro
  }
}

OTHER_FILES = qml/*.qml
