#load(qt_build_config)
TEMPLATE = subdirs
av.depends = core gui
avwidgets.depends = av
#MODULE_VERSION = $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
config_gl {
  avwidgets.depends += opengl
}
#load(qt_module)
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/common.pri)
preparePaths($$OUT_PWD/../../out)

VERSION = $$QTAV_VERSION
# windows: Qt5AV.dll, not Qt1AV.dll
!mac_framework: VERSION = $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}

PROJECT_LIBDIR = $$qtLongName($$BUILD_DIR/lib)

LIBPREFIX = lib
contains(QMAKE_HOST.os,Windows) {
  SCRIPT_SUFFIX=bat
  MOVE = move /y
  COPY = copy /y
  COPY_DIR = xcopy /s /q /y /i
  MKDIR = mkdir
  RM = del
  RM_DIR = rd /s /q
} else {
  SCRIPT_SUFFIX=sh
  MOVE = mv
  COPY = cp -f
  COPY_DIR = $$COPY -R
  MKDIR = mkdir -p
  RM = rm -f
  RM_DIR = rm -rf
}

win32 {
  *g++* {
    LIBSUFFIX = a
  } else {
    LIBSUFFIX = lib
    LIBPREFIX =
  }
} else:macx {
  LIBSUFFIX = dylib
} else:ios {
  LIBSUFFIX = a
} else {
  LIBSUFFIX = so
}
DEBUG_SUF=
mac: DEBUG_SUF=_debug
win32: DEBUG_SUF=d
NAME_SUF=
iphonesimulator:!device: NAME_SUF=_iphonesimulator
defineTest(createForModule) {
  MODULE_NAME = $$1
  MODULE_FULL_NAME = Qt$$MODULE_NAME
  MODULE = $$lower($$MODULE_NAME)
  MODULE_DEPENDS = $$eval($${MODULE}.depends)
  MODULE_DEFINES = QT_$$upper($${MODULE})_LIB
  static {
    ORIG_LIB = $${LIBPREFIX}$${MODULE_FULL_NAME}.$${LIBSUFFIX}
    ORIG_LIB_D = $${LIBPREFIX}$${MODULE_FULL_NAME}.$${LIBSUFFIX}
  } else {
    ORIG_LIB = $${LIBPREFIX}$$qtLibName($$MODULE_FULL_NAME, $$QTAV_MAJOR_VERSION).$${LIBSUFFIX}
    ORIG_LIB_D = $${LIBPREFIX}$$qtLibName($$MODULE_FULL_NAME$${DEBUG_SUF}, $$QTAV_MAJOR_VERSION).$${LIBSUFFIX}
  }
greaterThan(QT_MAJOR_VERSION, 4) {
  MODULE_PRF_FILE = $$OUT_PWD/mkspecs/features/$${MODULE}.prf
  NEW_LIB = $${LIBPREFIX}Qt$${QT_MAJOR_VERSION}$${MODULE_NAME}$${NAME_SUF}.$${LIBSUFFIX}
  NEW_LIB_D = $${LIBPREFIX}Qt$${QT_MAJOR_VERSION}$${MODULE_NAME}$${NAME_SUF}$${DEBUG_SUF}.$${LIBSUFFIX}
  MKSPECS_DIR = $$[QT_HOST_DATA]/mkspecs
} else {
  MODULE_PRF_FILE = $$PWD/qt4/$${MODULE}.prf
  NEW_LIB = $${ORIG_LIB}
  NEW_LIB_D = $${ORIG_LIB_D}
  MKSPECS_DIR=$$[QMAKE_MKSPECS]
}

# copy files to a dir need '/' at the end
mac_framework {
  sdk_install.commands = $$quote($$COPY_DIR $$system_path($$PROJECT_LIBDIR/$${MODULE_FULL_NAME}.framework) $$system_path($$[QT_INSTALL_LIBS]))
  sdk_install.commands += $$quote($$RM $$system_path($$[QT_INSTALL_LIBS])/$${MODULE_FULL_NAME}.framework/*.prl)
} else {
  sdk_install.commands = $$quote($$MKDIR $$system_path($$[QT_INSTALL_HEADERS]/$${MODULE_FULL_NAME}/))
  sdk_install.commands += $$quote($$COPY $$system_path($$PROJECT_LIBDIR/*Qt*AV*.$$LIBSUFFIX*) $$system_path($$[QT_INSTALL_LIBS]/))
  sdk_install.commands += $$quote($$COPY $$system_path($$PROJECT_LIBDIR/$$ORIG_LIB) $$system_path($$[QT_INSTALL_LIBS]/$$NEW_LIB))
  sdk_install.commands += $$quote($$COPY $$system_path($$PROJECT_LIBDIR/$$ORIG_LIB_D) $$system_path($$[QT_INSTALL_LIBS]/$$NEW_LIB_D))
  static {
    sdk_install.commands += $$quote($$COPY $$system_path($$PROJECT_LIBDIR/$$replace(ORIG_LIB, .$$LIBSUFFIX$, .prl)) $$system_path($$[QT_INSTALL_LIBS]/$$replace(NEW_LIB, .$$LIBSUFFIX$, .prl)))
  }
}
sdk_install.commands += $$quote($$COPY $$system_path($$MODULE_PRF_FILE) $$system_path($$MKSPECS_DIR/features/$${MODULE}.prf))
greaterThan(QT_MAJOR_VERSION, 4) {
  sdk_install.commands += $$quote($$COPY $$system_path($$OUT_PWD/mkspecs/modules/qt_lib_$${MODULE}*.pri) $$system_path($$MKSPECS_DIR/modules/))
}
win32:sdk_install.commands += $$quote($$COPY $$system_path($$BUILD_DIR/bin/Qt*AV*.dll) $$system_path($$[QT_INSTALL_BINS]/))

mac_framework {
  sdk_uninstall.commands = $$quote($$RM_DIR $$system_path($$[QT_INSTALL_LIBS]/$${MODULE_FULL_NAME}.framework))
} else {
  sdk_uninstall.commands = $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_LIBS]/*Qt*AV*))
}
sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_LIBS]/$$NEW_LIB))
sdk_uninstall.commands += $$quote($$RM_DIR $$system_path($$[QT_INSTALL_HEADERS]/$${MODULE_FULL_NAME}))
sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$MKSPECS_DIR/features/$${MODULE}.prf))
greaterThan(QT_MAJOR_VERSION, 4) {
  sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$MKSPECS_DIR/modules/qt_lib_$${MODULE}*.pri))
}
win32: sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_BINS]/Qt*AV*.dll))
android: sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_LIBS]/libcommon.so))

write_file($$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX, sdk_install.commands, append)
write_file($$BUILD_DIR/sdk_uninstall.$$SCRIPT_SUFFIX, sdk_uninstall.commands, append)

message(run $$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX to install $${MODULE_FULL_NAME} as a Qt module)

greaterThan(QT_MAJOR_VERSION, 4) {
AV_PRF_CONT = "QTAV_MAJOR_VERSION=$$QTAV_MAJOR_VERSION"
AV_PRF_CONT += "QTAV_MINOR_VERSION=$$QTAV_MINOR_VERSION"
AV_PRF_CONT += "QTAV_PATCH_VERSION=$$QTAV_PATCH_VERSION"
AV_PRF_CONT += "android: QMAKE_LFLAGS += -lOpenSLES"
AV_PRF_CONT += "static: DEFINES += BUILD_QT$$upper($${MODULE})_STATIC"
#AV_PRF_CONT += "QMAKE_LFLAGS += -lavutil -lavcodec -lavformat -lswscale"
#config_avresample: AV_PRF_CONT += "QMAKE_LFLAGS += -lavresample"
#config_swresample: AV_PRF_CONT += "QMAKE_LFLAGS += -lswresample"
AV_PRF_CONT += "!contains(QT, $${MODULE}): QT *= $${MODULE}"
mac_framework {
# mac module with config 'lib_bundle' only include Headers dir. see qtAddModule in qt_functions.prf
# but will add Qt.$${MODULE}.config to CONFIG
# $$eval(QT.$${MODULE}.libs) for Qt5
  AV_PRF_CONT += "INCLUDEPATH *= $$[QT_INSTALL_LIBS]/$${MODULE_FULL_NAME}.framework/Headers/$${MODULE_FULL_NAME}"
}
write_file($$MODULE_PRF_FILE, AV_PRF_CONT)


MODULE_QMAKE_OUTDIR = $$OUT_PWD
MODULE_CONFIG = $${MODULE}
MODULE_INCNAME = $${MODULE_FULL_NAME}

## the following is from mkspecs/features/qt_module_pris.prf
mod_work_pfx = $$MODULE_QMAKE_OUTDIR/mkspecs/modules
need_fwd_pri: \
    mod_inst_pfx = $$MODULE_QMAKE_OUTDIR/mkspecs/modules-inst
else: \
    mod_inst_pfx = $$mod_work_pfx
!internal_module {
    MODULE_ID = $$MODULE
    MODULE_PRIVATE_PRI = $$mod_inst_pfx/qt_lib_$${MODULE}_private.pri
    mods_to_load = $$MODULE $${MODULE}_private
} else {
    MODULE_ID = $${MODULE}_private
    mods_to_load = $${MODULE}_private
}
need_fwd_pri: \
    pris_to_load = $$MODULE_ID
else: \
    pris_to_load = $$mods_to_load
MODULE_PRI = $$mod_inst_pfx/qt_lib_$${MODULE_ID}.pri
MODULE_FWD_PRI = $$mod_work_pfx/qt_lib_$${MODULE_ID}.pri


!build_pass {

    # Create a module .pri file
    host_build: \
        module_libs = "\$\$QT_MODULE_HOST_LIB_BASE"
    else: \
        module_libs = "\$\$QT_MODULE_LIB_BASE"
    unix:!static {
        host_build: \
            module_rpath = "QT.$${MODULE_ID}.rpath = $$[QT_HOST_LIBS]"
        else: \
            module_rpath = "QT.$${MODULE_ID}.rpath = $$[QT_INSTALL_LIBS/raw]"
    } else {
        module_rpath =
    }
    !isEmpty(QT_PRIVATE): \
        module_rundep = "QT.$${MODULE_ID}.run_depends = $$replace(QT_PRIVATE, -private$, _private)"
    else: \
        module_rundep =
    static: \
        module_build_type = staticlib
    else:mac:contains(QT_CONFIG, qt_framework): \
        module_build_type = lib_bundle
    else: \
        module_build_type =
    internal_module: \
        module_build_type += internal_module
    !isEmpty(MODULE_CONFIG): \
        module_config = "QT.$${MODULE_ID}.CONFIG = $$MODULE_CONFIG"
    else: \
        module_config =
    !isEmpty(MODULE_PLUGIN_TYPES): \
        module_plugtypes = "QT.$${MODULE_ID}.plugin_types = $$replace(MODULE_PLUGIN_TYPES, /.*$, )"
    else: \
        module_plugtypes =
    !no_module_headers:!minimal_syncqt {
        MODULE_INCLUDES = \$\$QT_MODULE_INCLUDE_BASE \$\$QT_MODULE_INCLUDE_BASE/$$MODULE_INCNAME
        MODULE_PRIVATE_INCLUDES = \$\$QT_MODULE_INCLUDE_BASE/$$MODULE_INCNAME/$$VERSION \
                                  \$\$QT_MODULE_INCLUDE_BASE/$$MODULE_INCNAME/$$VERSION/$$MODULE_INCNAME
    }
    split_incpath: \
        MODULE_SHADOW_INCLUDES = $$replace(MODULE_INCLUDES, ^\\\$\\\$QT_MODULE_INCLUDE_BASE, \
                                                            $$MODULE_BASE_OUTDIR/include)
    MODULE_INCLUDES += $$MODULE_AUX_INCLUDES
    MODULE_PRIVATE_INCLUDES += $$MODULE_PRIVATE_AUX_INCLUDES
    internal_module: \
        MODULE_INCLUDES += $$MODULE_PRIVATE_INCLUDES
    split_incpath: \
        MODULE_FWD_PRI_CONT_SUFFIX += "QT.$${MODULE_ID}.includes += $$MODULE_SHADOW_INCLUDES"
    MODULE_PRI_CONT = \
        "QT.$${MODULE_ID}.VERSION = $${VERSION}" \
        "QT.$${MODULE_ID}.MAJOR_VERSION = $$section(VERSION, ., 0, 0)" \
        "QT.$${MODULE_ID}.MINOR_VERSION = $$section(VERSION, ., 1, 1)" \
        "QT.$${MODULE_ID}.PATCH_VERSION = $$section(VERSION, ., 2, 2)" \
        "" \
        "QT.$${MODULE_ID}.name = $${MODULE_FULL_NAME}" \
        "QT.$${MODULE_ID}.libs = $$module_libs" \
        $$module_rpath \
        "QT.$${MODULE_ID}.includes = $$MODULE_INCLUDES"
    !host_build: MODULE_PRI_CONT += \
        "QT.$${MODULE_ID}.bins = \$\$QT_MODULE_BIN_BASE" \
        "QT.$${MODULE_ID}.libexecs = \$\$QT_MODULE_LIBEXEC_BASE" \
        "QT.$${MODULE_ID}.plugins = \$\$QT_MODULE_PLUGIN_BASE" \
        "QT.$${MODULE_ID}.imports = \$\$QT_MODULE_IMPORT_BASE" \
        "QT.$${MODULE_ID}.qml = \$\$QT_MODULE_QML_BASE" \
        $$module_plugtypes
    MODULE_PRI_CONT += \
        "QT.$${MODULE_ID}.depends =$$join(MODULE_DEPENDS, " ", " ")" \
        $$module_rundep \
        "QT.$${MODULE_ID}.module_config =$$join(module_build_type, " ", " ")" \
        $$module_config \
        "QT.$${MODULE_ID}.DEFINES = $$MODULE_DEFINES" \ # assume sufficient quoting
        "" \
        "QT_MODULES += $$MODULE"
    write_file($$MODULE_PRI, MODULE_PRI_CONT)|error("Aborting.")
    !internal_module {
        module_build_type += internal_module no_link
        MODULE_PRIVATE_PRI_CONT = \
            "QT.$${MODULE}_private.VERSION = $${VERSION}" \
            "QT.$${MODULE}_private.MAJOR_VERSION = $$section(VERSION, ., 0, 0)" \
            "QT.$${MODULE}_private.MINOR_VERSION = $$section(VERSION, ., 1, 1)" \
            "QT.$${MODULE}_private.PATCH_VERSION = $$section(VERSION, ., 2, 2)" \
            "" \
            "QT.$${MODULE}_private.name = $${MODULE_FULL_NAME}" \   # Same name as base module
            "QT.$${MODULE}_private.libs = $$module_libs" \
            "QT.$${MODULE}_private.includes = $$MODULE_PRIVATE_INCLUDES" \
            "QT.$${MODULE}_private.depends = $$replace($$list($$MODULE $$QT_FOR_PRIVATE), -private$, _private)" \
            "QT.$${MODULE}_private.module_config =$$join(module_build_type, " ", " ")"
        write_file($$MODULE_PRIVATE_PRI, MODULE_PRIVATE_PRI_CONT)|error("Aborting.")
    }
    MODULE_PRI_FILES = $$MODULE_PRI $$MODULE_PRIVATE_PRI

    need_fwd_pri {

        split_incpath: \
            MODULE_BASE_INCDIR = $$MODULE_BASE_INDIR
        else: \
            MODULE_BASE_INCDIR = $$MODULE_BASE_OUTDIR

        # Create a forwarding module .pri file
        MODULE_FWD_PRI_CONT = \
            "QT_MODULE_BIN_BASE = $$MODULE_BASE_OUTDIR/bin" \
            "QT_MODULE_INCLUDE_BASE = $$MODULE_BASE_INCDIR/include" \
            "QT_MODULE_IMPORT_BASE = $$MODULE_BASE_OUTDIR/imports" \
            "QT_MODULE_QML_BASE = $$MODULE_BASE_OUTDIR/qml" \
            "QT_MODULE_LIB_BASE = $$MODULE_BASE_OUTDIR/lib" \
            "QT_MODULE_HOST_LIB_BASE = $$MODULE_BASE_OUTDIR/lib" \
            "QT_MODULE_LIBEXEC_BASE = $$MODULE_BASE_OUTDIR/libexec" \
            "QT_MODULE_PLUGIN_BASE = $$MODULE_BASE_OUTDIR/plugins" \
            "include($$MODULE_PRI)"
        !internal_module: MODULE_FWD_PRI_CONT += \
            "include($$MODULE_PRIVATE_PRI)"
        MODULE_FWD_PRI_CONT += $$MODULE_FWD_PRI_CONT_SUFFIX
        write_file($$MODULE_FWD_PRI, MODULE_FWD_PRI_CONT)|error("Aborting.")
        touch($$MODULE_FWD_PRI, $$MODULE_PRI)
        MODULE_PRI_FILES += $$MODULE_FWD_PRI

    } else {

        # This is needed for the direct include() below. Mirrors qt_config.prf
        QT_MODULE_BIN_BASE = $$[QT_INSTALL_BINS]
        QT_MODULE_INCLUDE_BASE = $$[QT_INSTALL_HEADERS]
        QT_MODULE_IMPORT_BASE = $$[QT_INSTALL_IMPORTS]
        QT_MODULE_QML_BASE = $$[QT_INSTALL_QML]
        QT_MODULE_LIB_BASE = $$[QT_INSTALL_LIBS]
        QT_MODULE_HOST_LIB_BASE = $$[QT_HOST_LIBS]
        QT_MODULE_LIBEXEC_BASE = $$[QT_INSTALL_LIBEXECS]
        QT_MODULE_PLUGIN_BASE = $$[QT_INSTALL_PLUGINS]

    }

    # Then, inject the new module into the current cache state
    !contains(QMAKE_INTERNAL_INCLUDED_FILES, $$MODULE_PRI): \ # before the actual include()!
        cache(QMAKE_INTERNAL_INCLUDED_FILES, add transient, MODULE_PRI_FILES)
    for(pri, pris_to_load): \
        include($$mod_work_pfx/qt_lib_$${pri}.pri)
    for(mod, mods_to_load) {
        for(var, $$list(VERSION MAJOR_VERSION MINOR_VERSION PATCH_VERSION \
                        name depends run_depends plugin_types module_config CONFIG DEFINES \
                        includes bins libs libexecs plugins imports qml \
                )):defined(QT.$${mod}.$$var, var):cache(QT.$${mod}.$$var, transient)
    }
    cache(QT_MODULES, transient)

} # !build_pass


# Schedule the regular .pri file for installation
CONFIG += qt_install_module
} #Qt5

eval(qt$${MODULE}_pri.files = $$MODULE_PRI_FILES)
eval(qt$${MODULE}_pri.path = $$MKSPECS_DIR/modules)
greaterThan(QT_MAJOR_VERSION, 4): INSTALLS += qt$${MODULE}_pri
eval(qt$${MODULE}_prf.files = $$MODULE_PRF_FILE)
eval(qt$${MODULE}_prf.path = $$MKSPECS_DIR/features)
INSTALLS += qt$${MODULE}_prf

# export is required, otherwise INSTALLS is not valid
  export(qt$${MODULE}_pri.files)
  export(qt$${MODULE}_pri.path)
  export(qt$${MODULE}_prf.files)
  export(qt$${MODULE}_prf.path)
  !contains(QMAKE_HOST.os, Windows):export(INSTALLS)

  return(true)
} #createForModule

write_file($$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX)
write_file($$BUILD_DIR/sdk_uninstall.$$SCRIPT_SUFFIX)

avmodules = AV
!no-widgets: avmodules += AVWidgets
for(module, $$list($$avmodules)) {
message("creating script for module Qt$$module ...")
  createForModule($$module)
}

#headers
!mac_framework {
  sdk_h_install.commands = $$quote($$COPY $$system_path($$PROJECTROOT/src/QtAV/*.h) $$system_path($$[QT_INSTALL_HEADERS]/QtAV/))
  sdk_h_install.commands += $$quote($$COPY $$system_path($$PROJECTROOT/src/QtAV/QtAV) $$system_path($$[QT_INSTALL_HEADERS]/QtAV/))
  !no-widgets {
    sdk_h_install.commands += $$quote($$COPY $$system_path($$PROJECTROOT/widgets/QtAVWidgets/*.h) $$system_path($$[QT_INSTALL_HEADERS]/QtAVWidgets/))
    sdk_h_install.commands += $$quote($$COPY $$system_path($$PROJECTROOT/widgets/QtAVWidgets/QtAVWidgets) $$system_path($$[QT_INSTALL_HEADERS]/QtAVWidgets/))
  }
  sdk_h_install.commands += $$quote($$MKDIR $$system_path($$[QT_INSTALL_HEADERS]/QtAV/$$VERSION/QtAV/))
  contains(QMAKE_HOST.os,Windows) {
    sdk_h_install.commands += $$quote($$COPY_DIR $$system_path($$PROJECTROOT/src/QtAV/private) $$system_path($$[QT_INSTALL_HEADERS]/QtAV/private))
    sdk_h_install.commands += $$quote($$COPY_DIR $$system_path($$PROJECTROOT/src/QtAV/private) $$system_path($$[QT_INSTALL_HEADERS]/QtAV/$$VERSION/QtAV/private))
  } else {
    sdk_h_install.commands += $$quote($$COPY_DIR $$system_path($$PROJECTROOT/src/QtAV/private) $$system_path($$[QT_INSTALL_HEADERS]/QtAV))
    sdk_h_install.commands += $$quote($$COPY_DIR $$system_path($$PROJECTROOT/src/QtAV/private) $$system_path($$[QT_INSTALL_HEADERS]/QtAV/$$VERSION/QtAV))
  }
  write_file($$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX, sdk_h_install.commands, append)
}
#qml
greaterThan(QT_MAJOR_VERSION, 4) {
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    contains(QMAKE_HOST.os,Windows) {
      sdk_qml_install.commands = $$quote($$COPY_DIR $$system_path($$BUILD_DIR/bin/QtAV) $$system_path($$[QT_INSTALL_QML]/QtAV))
    } else {
      sdk_qml_install.commands = $$quote($$COPY_DIR $$system_path($$BUILD_DIR/bin/QtAV) $$system_path($$[QT_INSTALL_QML]))
    }
    static: sdk_qml_install.commands += $$quote($$COPY $$system_path($$PROJECT_LIBDIR/$$ORIG_LIB/libQmlAV*) $$system_path($$[QT_INSTALL_QML]/QtAV/))
#static qml plugin is not copied to bin/QtAV. copy it and prl
    sdk_qml_install.commands += $$quote($$COPY $$system_path($$PROJECTROOT/qml/plugins.qmltypes) $$system_path($$[QT_INSTALL_QML]/QtAV/))
    sdk_qml_uninstall.commands = $$quote($$RM_DIR $$system_path($$[QT_INSTALL_QML]/QtAV))
    write_file($$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX, sdk_qml_install.commands, append)
    write_file($$BUILD_DIR/sdk_uninstall.$$SCRIPT_SUFFIX, sdk_qml_uninstall.commands, append)
  }
}
