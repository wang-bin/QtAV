TEMPLATE = subdirs

SUBDIRS = common

!no-widgets:!android {
SUBDIRS += \
    sharedoutput \
    simpletranscode \
    simpleplayer \
    player \
    filters \
    videocapture \
    videographicsitem \
    videogroup \
    videowall
}
player.depends += common

greaterThan(QT_MAJOR_VERSION, 4) {
  greaterThan(QT_MINOR_VERSION, 3) {
    !android: SUBDIRS += window
  }
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += QMLPlayer
    QMLPlayer.depends += common
  }
}

OTHER_FILES = qml/*.qml
