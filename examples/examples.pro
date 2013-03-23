TEMPLATE = subdirs

SUBDIRS += \
    simpleplayer \
    videographicsitem \
    videowall

#TODO: mingw cross
config_gdiplus {
    SUBDIRS += player-gdi
}
config_direct2d {
    SUBDIRS += player-d2d
}
config_gl {
    SUBDIRS += player-gl
}
