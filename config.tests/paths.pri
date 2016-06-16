TEMPLATE = lib # can not create exe for some platforms (winrt, ios). If check header only, staticlib is fine
# not static lib because sometimes we need to check link flags. if qt is static build, this chek may fail. we don't test link for static build because it's impossible to add all dependencies to link flags
INCLUDEPATH += $$[QT_INSTALL_HEADERS]
LIBS += -L$$[QT_INSTALL_LIBS]
CONFIG -= qt app_bundle lib_bundle
CONFIG += console
*msvc*: INCLUDEPATH *= $$PWD/../src/compat/msvc
!config_dx: INCLUDEPATH *= $$PWD/../contrib/dxsdk
