TEMPLATE = subdirs

SUBDIRS += \
    common \
    sharedoutput \
    player \
    filters \
    videographicsitem \
    videogroup \
    videowall

player.depends += common

config_gl {
    SUBDIRS += vo-gl
}

greaterThan(QT_MAJOR_VERSION, 4):qtHaveModule(quick) {
  SUBDIRS += QMLPlayer \
            qmlvideofx
QMLPlayer.depends += common
}
