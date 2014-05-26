#load(qt_build_config)
TEMPLATE = subdirs
TARGET = QtAV
MODULE_DEPENDS = core widgets opengl gui
MODULE = av
VERSION = $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
#MODULE_VERSION = $$VERSION
#load(qt_module)

STATICLINK = 0
PROJECTROOT = $$PWD/../../..
include($$PROJECTROOT/common.pri)
preparePaths($$OUT_PWD/../../../out)


PROJECT_LIBDIR = $$qtLongName($$BUILD_DIR/lib)

LIBPREFIX = lib
win32 {
  *g++* {
    LIBSUFFIX = a
  } else {
    LIBSUFFIX = lib
    LIBPREFIX =
  }
} else {
  macx: LIBSUFFIX = dylib
  else: LIBSUFFIX = so
}

ORIG_LIB = $${LIBPREFIX}QtAV1.$${LIBSUFFIX}
NEW_LIB = $${LIBPREFIX}Qt$${QT_MAJOR_VERSION}AV.$${LIBSUFFIX}


sdk.target = sdk
sdk.commands = $$quote($$QMAKE_COPY $$system_path($$PROJECT_LIBDIR/*QtAV*) $$system_path($$[QT_INSTALL_LIBS]))
sdk.commands += $$quote($$QMAKE_COPY $$system_path($$PROJECT_LIBDIR/$$ORIG_LIB) $$system_path($$[QT_INSTALL_LIBS]/$$NEW_LIB))
sdk.commands += $$quote($$QMAKE_COPY_DIR $$system_path($$PROJECTROOT/src/QtAV) $$system_path($$[QT_INSTALL_HEADERS]/QtAV))
sdk.commands += $$quote($$QMAKE_COPY_DIR $$system_path($$PROJECTROOT/qml/QmlAV) $$system_path($$[QT_INSTALL_HEADERS]/QmlAV))
sdk.commands = $$join(sdk.commands, $$escape_expand(\\n\\t))
QMAKE_EXTRA_TARGETS += sdk

sdk_uninstall.target = sdk_uninstall
sdk_uninstall.commands = $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_LIBS]/*QtAV*))
sdk_uninstall.commands += $$quote($$QMAKE_DEL_FILE $$system_path($$[QT_INSTALL_LIBS]/$$NEW_LIB))
sdk_uninstall.commands += $$quote($$QMAKE_DEL_DIR $$system_path($$[QT_INSTALL_HEADERS]/QtAV))
sdk_uninstall.commands += $$quote($$QMAKE_DEL_DIR $$system_path($$[QT_INSTALL_HEADERS]/QmlAV))
sdk_uninstall.commands = $$join(sdk_uninstall.commands, $$escape_expand(\\n\\t))
QMAKE_EXTRA_TARGETS += sdk_uninstall


SCRIPT_SUFFIX=sh
win32: SCRIPT_SUFFIX=bat

write_file($$BUILD_DIR/sdk_install.$$SCRIPT_SUFFIX, sdk.commands)
write_file($$BUILD_DIR/sdk_uninstall.$$SCRIPT_SUFFIX, sdk_uninstall.commands)


AV_PRF_CONT = "android: QMAKE_LFLAGS += -lOpenSLES"
#AV_PRF_CONT += "QMAKE_LFLAGS += -lavutil -lavcodec -lavformat -lswscale"
#config_avresample: AV_PRF_CONT += "QMAKE_LFLAGS += -lavresample"
#config_swresample: AV_PRF_CONT += "QMAKE_LFLAGS += -lswresample"
write_file($$[QT_INSTALL_BINS]/../mkspecs/features/av.prf, AV_PRF_CONT)

MODULE_QMAKE_OUTDIR = $$[QT_INSTALL_BINS]/..
MODULE_CONFIG = av


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
        "QT.$${MODULE_ID}.name = $$TARGET" \
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
            "QT.$${MODULE}_private.name = $${TARGET}" \   # Same name as base module
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
                        name depends module_config CONFIG DEFINES sources \
                        includes bins libs libexecs plugins imports qml \
                        rpath_link \
                )):defined(QT.$${mod}.$$var, var):cache(QT.$${mod}.$$var, transient)
    }
    cache(QT_MODULES, transient)

} # !build_pass

# Schedule the regular .pri file for installation
CONFIG += qt_install_module
