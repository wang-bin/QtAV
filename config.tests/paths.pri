TEMPLATE = lib # can not create exe for some platforms (winrt, ios)
# not static lib because sometimes we need to check link flags
INCLUDEPATH += $$[QT_INSTALL_HEADERS]
LIBS += -L$$[QT_INSTALL_LIBS]
CONFIG -= app_bundle lib_bundle
