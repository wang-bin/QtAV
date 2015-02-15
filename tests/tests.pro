TEMPLATE = subdirs

SUBDIRS += \
    ao \
    decoder \
    subtitle

!no-widgets {
  SUBDIRS += \
    extract \
    qiodevice \
    qrc \
    playerthread
}
