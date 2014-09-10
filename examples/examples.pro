TEMPLATE = subdirs

SUBDIRS += \
    common \
    sharedoutput \
    simpleplayer \
    player \
    filters \
    videographicsitem \
    videogroup \
    videowall

player.depends += common

greaterThan(QT_MAJOR_VERSION, 4) {
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += QMLPlayer
    QMLPlayer.depends += common
  }
}
