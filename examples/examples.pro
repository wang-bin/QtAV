TEMPLATE = subdirs

SUBDIRS += \
    common \
    sharedoutput \
    simpleplayer \
    player \
    filters \
    videographicsitem \
    videogroup \
    videowall \
    qmlmultiwindow

player.depends += common

greaterThan(QT_MAJOR_VERSION, 4):qtHaveModule(quick) {
  SUBDIRS += QMLPlayer \
            qmlvideofx
  QMLPlayer.depends += common
}
