TEMPLATE = subdirs

SUBDIRS += \
    vo-qt \
    player \
    videographicsitem \
    videowall

#TODO: mingw cross
config_gdiplus {
    SUBDIRS += vo-gdi
}
config_direct2d {
    SUBDIRS += vo-d2d
}
config_gl {
    SUBDIRS += vo-gl
}

greaterThan(QT_MAJOR_VERSION, 4):qtHaveModule(quick) {
  SUBDIRS += QMLPlayer
}
