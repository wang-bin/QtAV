TEMPLATE = subdirs

SUBDIRS += \
    ao \
    decoder \
    subtitle \
    transcode

!no-widgets {
  SUBDIRS += \
    extract \
    qiodevice \
    qrc \
    playerthread
}
