isEmpty(PROJECTROOT): PROJECTROOT = $$PWD
INSTALL_PREFIX = /usr/local

share.files = $$PROJECTROOT/qtc_packaging/common/changelog \
			$$PROJECTROOT/qtc_packaging/common/copyright \
			$$PROJECTROOT/qtc_packaging/common/README
share.path = $$[QT_INSTALL_PREFIX]/share/doc/$${TARGET}

isEqual(TEMPLATE, app) {
	unix:!symbian {
		!isEmpty(MEEGO_VERSION_MAJOR) {
			DEFINES += CACHE_APPDIR
			INSTALL_PREFIX = /opt/$${TARGET}
			desktopfile.files = $$PROJECTROOT/qtc_packaging/debian_harmattan/$${TARGET}.desktop
			desktopfile.path = $$[QT_INSTALL_PREFIX]/share/applications
			icon.files = $$PROJECTROOT/qtc_packaging/debian_harmattan/$${TARGET}.png
			icon.path = $$[QT_INSTALL_PREFIX]/share/icons/hicolor/80x80/apps
			#debian.files = $$PROJECTROOT/qtc_packaging/harmattan/control
		} else:maemo5 {
			INSTALL_PREFIX = /opt/$${TARGET}
			desktopfile.files = $$PROJECTROOT/qtc_packaging/debian_fremantle/$${TARGET}.desktop
			desktopfile.path = $$[QT_INSTALL_PREFIX]/share/applications/hildon
			icon.files = $$PROJECTROOT/qtc_packaging/debian_fremantle/$${TARGET}.png
			icon.path = $$[QT_INSTALL_PREFIX]/share/icons/hicolor/64x64/apps
			#debian.files = $$PROJECTROOT/qtc_packaging/fremantle/control
		} else {
			desktopfile.files = $$PROJECTROOT/qtc_packaging/debian_generic/$${TARGET}.desktop
			desktopfile.path = $$[QT_INSTALL_PREFIX]/share/applications
			icon.files = $$PROJECTROOT/qtc_packaging/debian_generic/$${TARGET}.png
			icon.path = $$[QT_INSTALL_PREFIX]/share/icons/hicolor/64x64/apps
			#debian.files = $$PROJECTROOT/qtc_packaging/generic/control
		}
		INSTALLS += desktopfile icon
		#debian.path = /DEBIAN
            isEmpty(target.path): target.path = $${INSTALL_PREFIX}/bin
        }
} else {
	unix:!symbian {
		sdkheaders.files = $$SDK_HEADERS
                sdkheaders_private.files = $$SDK_PRIVATE_HEADERS
		isEmpty(SDK_HEADERS) {
			sdkheaders.files = $$HEADERS
		}
                sdkheaders.path = $$[QT_INSTALL_HEADERS]/$$MODULE_INCNAME
                sdkheaders_private.path = $$[QT_INSTALL_HEADERS]/$$MODULE_INCNAME/$$MODULE_VERSION/$$MODULE_INCNAME/private
                !plugin: target.path = $$[QT_INSTALL_LIBS]
                INSTALLS += sdkheaders sdkheaders_private
	}
}

INSTALLS *= target share

for(bin, BIN_INSTALLS) {
    eval($${bin}.path = $${INSTALL_PREFIX}/bin)
    message("adding $$bin to bin install targets...")
    INSTALLS += $$bin
}


