TARGET = Player
TEMPLATE = app
contains(QT_CONFIG, opengl): QT += opengl
QT += sql svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TRANSLATIONS = res/player_zh_CN.ts res/player.ts
!static:VERSION = $$QTAV_VERSION # vc: will create exp and lib, result in static build error

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
STATICLINK=1
include($$PWD/../common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)
INCLUDEPATH += $$PWD
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
genRC($$TARGET)
include(src.pri)

unix:!android:!mac {
#debian
player_bins = Player QMLPlayer
DEB_INSTALL_LIST = $$join(player_bins, \\n.$$[QT_INSTALL_BINS]/, .$$[QT_INSTALL_BINS]/)
DEB_INSTALL_LIST *= \
            usr/share/applications/Player.desktop \
            usr/share/applications/QMLPlayer.desktop \
            usr/share/icons/hicolor/scalable/apps/QtAV.svg
deb_install_list.target = qtav-players.install
deb_install_list.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${deb_install_list.target}
QMAKE_EXTRA_TARGETS += deb_install_list
target.depends *= $${deb_install_list.target}

qtav_players_links.target = qtav-players.links
qtav_players_links.commands = echo \"$$[QT_INSTALL_BINS]/Player /usr/bin/Player\n$$[QT_INSTALL_BINS]/QMLPlayer /usr/bin/QMLPlayer\" >$$PROJECTROOT/debian/$${qtav_players_links.target}
QMAKE_EXTRA_TARGETS *= qtav_players_links
target.depends *= $${qtav_players_links.target}
}

tv.files = res/tv.ini
#BIN_INSTALLS += tv
target.path = $$[QT_INSTALL_BINS]
include($$PROJECTROOT/deploy.pri)

RESOURCES += res/player.qrc
