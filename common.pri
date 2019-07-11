# qmake common template pri file
# Copyright (C) 2011-2016 Wang Bin <wbsecg1@gmail.com>
# Shanghai, China.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

isEmpty(COMMON_PRI_INCLUDED): { #begin COMMON_PRI_INCLUDED

mac:contains(QT_CONFIG, qt_framework):!mac_dylib: CONFIG += mac_framework

isEmpty(QMAKE_EXTENSION_SHLIB) {
  unix {
    mac|ios: QMAKE_EXTENSION_SHLIB = dylib
    else: QMAKE_EXTENSION_SHLIB = so #why android is empty?
  } else:win* {
    QMAKE_EXTENSION_SHLIB = dll
  }
}

#$$[TARGET_PLATFORM]
#$$[QT_ARCH] #windows symbian windowsce arm
_OS =
_ARCH =
_EXTRA =

unix {
    _OS = _unix
    android {
        _OS = _android
    } else:ios {
        _OS = _ios
    } else:macx {
        _OS = _osx
    } else:*maemo* {
        _OS = _maemo
        *maemo5*:_OS = _maemo5
        *maemo6*:_OS = _maemo6
    } else:*meego* {
        _OS = _meego
        !isEmpty(MEEGO_EDITION): _OS = _$$MEEGO_EDITION
    } else:*linux* {
        _OS = _linux
    }
# QMAKE_RPATHDIR will be ignored if QMAKE_LFLAGS_RPATH is not defined. e.g. qt4.8 unsupported/macx-clang-libc++
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
} else:wince* {
    _OS = _wince
} else:winrt {
    _OS = _winrt
} else:win32 { #true for wince
    _OS = _win
}
#*arm*: _ARCH = $${_ARCH}_arm
contains(QT_ARCH, arm.*) {
	_ARCH = $${_ARCH}_$${QT_ARCH}
}
*64|contains(QMAKE_TARGET.arch, x86_64): _ARCH = $${_ARCH}_x64
*llvm*: _EXTRA = _llvm
#*msvc*:

win32-msvc* {
	#Don't warn about sprintf, fopen etc being 'unsafe'
	DEFINES += _CRT_SECURE_NO_WARNINGS
}

#################################functions#########################################
defineTest(qtAtLeast) { #e.g. qtAtLeast(4), qtAtLeast(5, 2), qtAtLeast(5, 4, 2)
  lessThan(QT_MAJOR_VERSION, $$1):return(false)
  isEmpty(2):return(true)
  greaterThan(QT_MAJOR_VERSION, $$1):return(true)

  lessThan(QT_MINOR_VERSION, $$2):return(false)
  isEmpty(3):return(true)
  greaterThan(QT_MINOR_VERSION, $$2):return(true)

  lessThan(QT_PATCH_VERSION, $$3):return(false)
  return(true)
}

defineTest(qtRunQuitly) {
    #win32 always call windows command
    contains(QMAKE_HOST.os,Windows) {
      system("$$1 2>&1 >nul")|return(false)  #system always call win32 cmd
    } else {
      system("$$1 2>&1 >/dev/null")|return(false)
    }
    return(true)
}

defineReplace(platformTargetSuffix) {
    ios:CONFIG(iphonesimulator, iphonesimulator|iphoneos): \
        suffix = _iphonesimulator
    else: \
        suffix =

    CONFIG(debug, debug|release) {
        !debug_and_release|build_pass {
            mac: return($${suffix}_debug)
            win32: return($${suffix}d)
        }
    }
    return($$suffix)
}

#Acts like qtLibraryTarget. From qtcreator.pri
defineReplace(qtLibName) {
	#TEMPLATE += fakelib
	#LIB_FULLNAME = $$qtLibraryTarget($$1)
	#TEMPLATE -= fakelib
        unset(RET)
        RET = $$1
#qt5.4.2 add qt5LibraryTarget to fix qtLibraryTarget break
    qtAtLeast(5, 4) {
        mac:CONFIG(shared, static|shared):contains(QT_CONFIG, qt_framework) {
          QMAKE_FRAMEWORK_BUNDLE_NAME = $$RET
          export(QMAKE_FRAMEWORK_BUNDLE_NAME)
       } else {
           # insert the major version of Qt in the library name
           # unless it's a framework build
           isEqual(QT_MAJOR_VERSION, 5):isEqual(QT_MINOR_VERSION,4):lessThan(QT_PATCH_VERSION, 2):RET ~= s,^Qt,Qt$$QT_MAJOR_VERSION,
       }
    }
        RET = $$RET$$platformTargetSuffix()
        !win32: return($$RET)

	isEmpty(2): VERSION_EXT = $$VERSION
	else: VERSION_EXT = $$2
	!isEmpty(VERSION_EXT) {
		VERSION_EXT = $$section(VERSION_EXT, ., 0, 0)
		#isEqual(VERSION_EXT, 0):unset(VERSION_EXT)
	}
	RET = $${RET}$${VERSION_EXT}
	unset(VERSION_EXT)
        return($$RET)
}


#fakelib
defineReplace(qtStaticLib) {
# static lib does not have major version flag at the end
	unset(LIB_FULLNAME)
        TEMPLATE += fakelib
        LIB_FULLNAME = $$qtLibraryTarget($$1)
        TEMPLATE -= fakelib
        LIB_FULLNAME = $${QMAKE_PREFIX_STATICLIB}$$member(LIB_FULLNAME, 0).$${QMAKE_EXTENSION_STATICLIB}
	return($$LIB_FULLNAME)
}

defineReplace(qtSharedLib) {
	unset(LIB_FULLNAME)
	LIB_FULLNAME = $$qtLibName($$1, $$2)
        LIB_FULLNAME = $${QMAKE_PREFIX_SHLIB}$$member(LIB_FULLNAME, 0).$${QMAKE_EXTENSION_SHLIB} #default_post.prf
	return($$LIB_FULLNAME)
}

defineReplace(qtLongName) {
  unset(LONG_NAME)
  LONG_NAME = $$1$${_OS}_$$join(TARGET_ARCH,+)$${_EXTRA}
  return($$LONG_NAME)
}

defineTest(empty_file) {
    isEmpty(1): error("empty_file(name) requires one argument")
#"type NUL >filename" can create an empty file in windows, see http://stackoverflow.com/questions/210201/how-to-create-empty-text-file-from-a-batch-file
# 'echo. >file' or 'echo >file' will insert a new line, so use stderr
    win32:isEmpty(QMAKE_SH) {
        system("echo. 2> $$1")|return(false)
    } else {
#if sh is after win's echo, then "echo >$$1" fails because win's echo is used
        system("sh -c echo 2> $$1")|return(false)
    }
}

config_simd {
#TODO: QMAKE_CFLAGS_XXX, QT_CPU_FEATURES
*g++*|*qcc*: QMAKE_CFLAGS_NEON = -mfpu=neon
win32-icc {
  QMAKE_CFLAGS_SSE2 = -arch:SSE2
  QMAKE_CFLAGS_SSE4_1 = -arch:SSE4.1
} else:*-icc { #mac, linux
  QMAKE_CFLAGS_SSE2 = -xSSE2
  QMAKE_CFLAGS_SSE4_1 = -xSSE4.1
} else:*msvc* {
# all x64 processors supports sse2. unknown option for vc
  #!isEqual(QT_ARCH, x86_64)|!x86_64 {
    QMAKE_CFLAGS_SSE2 = -arch:SSE2
    QMAKE_CFLAGS_SSE4_1 = -arch:SSE2
  #}
} else {
  QMAKE_CFLAGS_SSE2 = -msse2
  QMAKE_CFLAGS_SSE4_1 = -msse4.1
}

#mac: simd will load qt_build_config and the result is soname will prefixed with QT_INSTALL_LIBS and link flag will append soname after QMAKE_LFLAGS_SONAME
defineTest(addSimdCompiler) { #from qt5 simd.prf
    name = $$1
    upname = $$upper($$name)
    headers_var = $${upname}_HEADERS
    sources_var = $${upname}_SOURCES
    asm_var = $${upname}_ASM
# config_$$1 is defined by config.tests (tests/arch)
    CONFIG($$1)|CONFIG(config_$$1) {
        cflags = $$eval(QMAKE_CFLAGS_$${upname})
        contains(QT_CPU_FEATURES, $$name) {
            # Default compiler settings include this feature, so just add to SOURCES
            SOURCES += $$eval($$sources_var)
            export(SOURCES)
        } else {
#qt4 always need eval() if var is not const
            # We need special compiler flags
            eval($${name}_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS) $$cflags $(INCPATH) ${QMAKE_FILE_IN})
            msvc: eval($${name}_compiler.commands += -Fo${QMAKE_FILE_OUT})
            else: eval($${name}_compiler.commands += -o ${QMAKE_FILE_OUT})

            eval($${name}_compiler.dependency_type = TYPE_C)
            eval($${name}_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)})
            eval($${name}_compiler.input = $$sources_var)
            eval($${name}_compiler.variable_out = OBJECTS)
            eval($${name}_compiler.name = compiling[$${name}] ${QMAKE_FILE_IN})
            silent: eval($${name}_compiler.commands = @echo compiling[$${name}] ${QMAKE_FILE_IN} && $$eval($${name}_compiler.commands))
            QMAKE_EXTRA_COMPILERS += $${name}_compiler

            export($${name}_compiler.commands)
            export($${name}_compiler.dependency_type)
            export($${name}_compiler.output)
            export($${name}_compiler.input)
            export($${name}_compiler.variable_out)
            export($${name}_compiler.name)
        }

        # We always need an assembler (need to run the C compiler and without precompiled headers)
        msvc {
            # Don't know how to run MSVC's assembler...
            !isEmpty($$asm_var): error("Sorry, not implemented: assembling $$upname for MSVC.")
        } else: false {
            # This is just for the IDE
            SOURCES += $$eval($$asm_var)
            export(SOURCES)
        } else {
            eval($${name}_assembler.commands = $$QMAKE_CC -c $(CFLAGS))
            !contains(QT_CPU_FEATURES, $${name}): eval($${name}_assembler.commands += $$cflags)
            clang:no_clang_integrated_as: eval($${name}_assembler.commands += -fno-integrated-as)
            eval($${name}_assembler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT})
            eval($${name}_assembler.dependency_type = TYPE_C)
            eval($${name}_assembler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)})
            eval($${name}_assembler.input = $$asm_var)
            eval($${name}_assembler.variable_out = OBJECTS)
            eval($${name}_assembler.name = assembling[$${name}] ${QMAKE_FILE_IN})
            silent: eval($${name}_assembler.commands = @echo assembling[$${name}] ${QMAKE_FILE_IN} && $$eval($${name}_assembler.commands))
            QMAKE_EXTRA_COMPILERS += $${name}_assembler

            export($${name}_assembler.commands)
            export($${name}_assembler.dependency_type)
            export($${name}_assembler.output)
            export($${name}_assembler.input)
            export($${name}_assembler.variable_out)
            export($${name}_assembler.name)
        }

        HEADERS += $$eval($$headers_var)
        export(HEADERS)
        export(QMAKE_EXTRA_COMPILERS)
    }
}
addSimdCompiler(sse2)
addSimdCompiler(sse3)
addSimdCompiler(ssse3)
addSimdCompiler(sse4_1)
addSimdCompiler(sse4_2)
addSimdCompiler(avx)
addSimdCompiler(avx2)
addSimdCompiler(neon)
addSimdCompiler(mips_dsp)
addSimdCompiler(mips_dspr2)
} #config_simd

##TODO: add defineReplace(getValue): parameter is varname
lessThan(QT_MAJOR_VERSION, 5) {
defineTest(log){
    system(echo $$system_quote($$1))
}

defineTest(mkpath) {
    qtRunQuitly("$$QMAKE_MKDIR $$system_path($$1)")|return(false)
    return(true)
}

defineTest(write_file) {
    #log("write_file($$1, $$2, $$3)")
    !isEmpty(4): error("write_file(name, [content var, [append]]) requires one to three arguments.")
    ##TODO: 1.how to replace old value
##getting the ref value requires a function whose parameter has var name and return a string. join() is the only function
## var name is $$2.
## echo a string with "\n" will fail, so we can not use join
    #val = $$join($$2, $$escape_expand(\\n))$$escape_expand(\\n)
    isEmpty(3)|!isEqual(3, append) {
#system("$$QMAKE_DEL_FILE $$1") #for win commad "del", path format used in qmake such as D:/myfile is not supported, "/" will be treated as an otpion for "del"
        empty_file($$1)
    }
    for(val, $$2) {
        system("echo $$system_quote($$val) >> \"$$1\"")|return(false)
    }
    return(true)
}

#defineTest(cache) {
#    !isEmpty(4): error("cache(var, [set|add|sub] [transient] [super], [srcvar]) requires one to three arguments.")
#}

defineReplace(clean_path) {
    win32:1 ~= s|\\\\|/|g
    contains(1, ^/.*):pfx = /
    else:pfx =
    segs = $$split(1, /)
    out =
    for(seg, segs) {
        equals(seg, ..):out = $$member(out, 0, -2)
        else:!equals(seg, .):out += $$seg
    }
    return($$join(out, /, $$pfx))
}

#make sure BUILD_DIR and SOURCE_ROOT is already defined. otherwise return the input path
#only operate on string, seperator is always "/"
defineReplace(shadowed) {
    isEmpty(SOURCE_ROOT)|isEmpty(BUILD_DIR):return($$1)
    1 ~= s,$$SOURCE_ROOT,,g
    shadow_dir = $$BUILD_DIR/$$1
    shadow_dir ~= s,//,/,g
    return($$shadow_dir)
}

defineReplace(shell_path) {
# QMAKE_DIR_SEP: \ for win cmd and / for sh
    1 ~= s,\\\\,$$QMAKE_DIR_SEP,g
    1 ~= s,//,$$QMAKE_DIR_SEP,g
    return($$1)
}

defineReplace(shell_quote_win) {
# Chars that should be quoted (TM).
# - control chars & space
# - the windows shell meta chars "&()<>^|
# - the potential separators ,;=
#TODO: how to deal with  "^", "|"? every char are separated by "|"?
#how to avoid replacing "^" again for the second time
    isEmpty(1):error("shell_quote(arg) requires one argument.")
    special_chars = & \( \) < >
    for(c, special_chars) {
        1 ~= s,$$c,^$$c,g
    }
#for qmake \\
    #1 ~= s,\\),^\),g
    #1 ~= s,\\(,^\(,g
    return($$1)
}

defineReplace(shell_quote_unix) {
# - unix shell:  0-32 \'"$`<>|;&(){}*?#!~[]
#TODO: how to deal with "#" "|" and "^"?
#how to avoid replacing "^" again for the second time
# \$ is eol
    special_chars = & \( \) < > \\ \' \" ` ; \{ \} * ? ! ~ \[ \]
    for(c, special_chars) {
        1 ~= s,$$c,\\$$c,g
    }
    return($$1)
}
##TODO: see qmake/library/ioutils.cpp
defineReplace(shell_quote) {
    contains(QMAKE_HOST.os,Windows):isEmpty(QMAKE_SH):return($$shell_quote_win($$1))
    return($$shell_quote_unix($$1))
}

##TODO: see qmake/library/ioutils.cpp
defineReplace(system_quote) {
    isEmpty(1):error("system_quote(arg) requires one argument.")
    contains(QMAKE_HOST.os,Windows): return($$shell_quote_win($$1))
    return($$shell_quote_unix($$1))
}

defineReplace(system_path) {
    contains(QMAKE_HOST.os,Windows) {
        1 ~= s,/,\\,g #qmake \\=>put \\=>real \?
    } else {
        1 ~= s,\\\\,/,g  ##why is \\\\. real \=>we read \\=>qmake \\\\?
    }
    return($$1)
}
} #lessThan(QT_MAJOR_VERSION, 5)

# set default rpath and add user defined rpaths
defineTest(set_rpath) {
  !unix: return(true)
  RPATHDIR = $$ARGS #see doc
  LIBS += -L/usr/local/lib
# $$[QT_INSTALL_LIBS] and $$DESTDIR and pro dir will be auto added to QMAKE_RPATHDIR if QMAKE_RPATHDIR is not empty
# Current (sub)project dir is auto added to the first value as prefix. e.g. QMAKE_RPATHDIR = .. ==> -Wl,-rpath,ROOT/..
# Executable dir search: ld -z origin, g++ -Wl,-R,'$ORIGIN', in makefile -Wl,-R,'$$ORIGIN'
# Working dir search: "."
# mac: install_name @rpath will search paths set in rpath link flags
# QMAKE_RPATHDIR: lflags maybe wrong, paths are modified
  #!cross_compile: RPATHDIR *= $$PROJECT_LIBDIR
  macx|ios {
    RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  } else {
    RPATHDIR *= \$\$ORIGIN \$\$ORIGIN/lib . /usr/local/lib $$[QT_INSTALL_LIBS]
# $$PROJECT_LIBDIR only for host == target. But QMAKE_TARGET.arch is only available on windows. QT_ARCH is bad, e.g. QT_ARCH=i386 while QMAKE_HOST.arch=i686
# https://bugreports.qt-project.org/browse/QTBUG-30263
    QMAKE_LFLAGS *= -Wl,-z,origin #\'-Wl,-rpath,$$join(RPATHDIR, ":")\'
  }
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
#  QMAKE_RPATHDIR *= $$RPATHDIR
#export(QMAKE_RPATHDIR)
  export(QMAKE_LFLAGS_SONAME)
  export(QMAKE_LFLAGS)
  return(true)
}

#argument 1 is default dir if not defined
defineTest(getBuildRoot) {
    !isEmpty(2): unset(BUILD_DIR)
    isEmpty(BUILD_DIR) {
        BUILD_DIR=$$(BUILD_DIR)
        isEmpty(BUILD_DIR) {
            #build_cache = $$PROJECTROOT/.build.cache #use root project's cache for subdir projects
            #!exists($$build_cache):build_cache = $$PWD/.build.cache #common.pri is in the root dir of a sub project
            #exists($$build_cache):include($$build_cache)
            isEmpty(BUILD_DIR) {
                BUILD_DIR=$$[BUILD_DIR]
                isEmpty(BUILD_DIR) {
                    !isEmpty(1) {
                        BUILD_DIR=$$1
                    } else {
                        BUILD_DIR = $$OUT_PWD
                        warning(BUILD_DIR not specified, using $$BUILD_DIR)
                    }
                }
            }
        }
    }
    export(BUILD_DIR)
    #message(BUILD_DIR=$$BUILD_DIR)
    return(true)
}

##############################paths####################################
#for Qt2, Qt3 which does not have QT_VERSION. Qt4: $$[QT_VERSION]
defineTest(preparePaths) {
    getBuildRoot($$1, $$2)
    MOC_DIR = $$BUILD_DIR/.moc/$${QT_VERSION}/$$TARGET
    RCC_DIR = $$BUILD_DIR/.rcc/$${QT_VERSION}/$$TARGET
    UI_DIR  = $$BUILD_DIR/.ui/$${QT_VERSION}/$$TARGET
    #obj is platform dependent
    OBJECTS_DIR = $$qtLongName($$BUILD_DIR/.obj/$$TARGET)
#before target name changed
    isEqual(TEMPLATE, app) {
        DESTDIR = $$BUILD_DIR/bin
#	TARGET = $$qtLongName($$TARGET)
        EXE_EXT =
        win32: EXE_EXT = .exe
        CONFIG(release, debug|release): !isEmpty(QMAKE_STRIP):!mac_framework: QMAKE_POST_LINK = -$$QMAKE_STRIP $$DESTDIR/$${TARGET}$${EXE_EXT} #.exe in win
    } else: DESTDIR = $$qtLongName($$BUILD_DIR/lib)
    !build_pass {
        message(target: $$DESTDIR/$$TARGET)
        !isEmpty(PROJECTROOT) {
            TRANSLATIONS *= i18n/$${TARGET}_zh_CN.ts
            export(TRANSLATIONS)
        }
    }
#export vars outside this function
    export(MOC_DIR)
    export(RCC_DIR)
    export(UI_DIR)
    export(OBJECTS_DIR)
    export(DESTDIR)
    #export(TARGET)
    return(true)
}
COMMON_PRI_INCLUDED = 1

} #end COMMON_PRI_INCLUDED
