TEMPLATE = subdirs

SUBDIRS = common
!android{
SUBDIRS += \
    sharedoutput \
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
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += QMLPlayer
    QMLPlayer.depends += common
  }
}
