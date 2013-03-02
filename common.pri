# qmake common template pri file
# Copyright (C) 2011-2013 Wang Bin <wbsecg1@gmail.com>
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

CONFIG += profile
#profiling, -pg is not supported for msvc
debug:!*msvc*:profile {
	QMAKE_CXXFLAGS_DEBUG += -pg
	QMAKE_LFLAGS_DEBUG += -pg
	QMAKE_CXXFLAGS_DEBUG = $$unique(QMAKE_CXXFLAGS_DEBUG)
	QMAKE_LFLAGS_DEBUG = $$unique(QMAKE_LFLAGS_DEBUG)
}

#$$[TARGET_PLATFORM]
#$$[QT_ARCH] #windows symbian windowsce arm
_OS =
_ARCH =
_EXTRA =

unix {
	_OS = _unix
	macx: _OS = _mac
	else:*linux*: _OS = _linux
	*maemo* {
		_OS = _maemo
		*maemo5*:_OS = _maemo5
		*maemo6*:_OS = _maemo6
	} else:*meego*: _OS = _meego
	!isEmpty(MEEGO_EDITION): _OS = _$$MEEGO_EDITION
} else:wince* {
	_OS = _wince
} else:win32 { #true for wince
	_OS = _win32
}
#*arm*: _ARCH = $${_ARCH}_arm
contains(QT_ARCH, arm.*) {
	_ARCH = $${_ARCH}_$${QT_ARCH}
}
*64: _ARCH = $${_ARCH}_x64
*llvm*: _EXTRA = _llvm
#*msvc*:

win32-msvc* {
	#Don't warn about sprintf, fopen etc being 'unsafe'
	DEFINES += _CRT_SECURE_NO_WARNINGS
}

#################################functions#########################################
defineReplace(cleanPath) {
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

#Acts like qtLibraryTarget. From qtcreator.pri
defineReplace(qtLibName) {
	#TEMPLATE += fakelib
	#LIB_FULLNAME = $$qtLibraryTarget($$1)
	#TEMPLATE -= fakelib
	unset(LIBRARY_NAME)
	LIBRARY_NAME = $$1
	CONFIG(debug, debug|release) {
		!debug_and_release|build_pass {
			mac:RET = $$member(LIBRARY_NAME, 0)_debug
			else:win32:RET = $$member(LIBRARY_NAME, 0)d
		}
	}
	isEmpty(RET):RET = $$LIBRARY_NAME
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
	unset(LIB_FULLNAME)
	LIB_FULLNAME = $$qtLibName($$1, $$2)
	*msvc*|win32-icc: LIB_FULLNAME = $$member(LIB_FULLNAME, 0).lib
	else: LIB_FULLNAME = lib$$member(LIB_FULLNAME, 0).a
	return($$LIB_FULLNAME)
}

defineReplace(qtSharedLib) {
	unset(LIB_FULLNAME)
	LIB_FULLNAME = $$qtLibName($$1, $$2)
	win32: LIB_FULLNAME = $$member(LIB_FULLNAME, 0).dll
	else {
		macx|ios: LIB_FULLNAME = lib$$member(LIB_FULLNAME, 0).$${QMAKE_EXTENSION_SHLIB} #default_post.prf
		else: LIB_FULLNAME = lib$$member(LIB_FULLNAME, 0).so
	}
	return($$LIB_FULLNAME)
}

defineReplace(qtLongName) {
	unset(LONG_NAME)
		LONG_NAME = $$1$${_OS}$${_ARCH}$${_EXTRA}
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

##TODO: add defineReplace(getValue): parameter is varname
lessThan(QT_MAJOR_VERSION, 5): {

defineTest(write_file) {
    !isEmpty(4): error("write_file(name, [content var, [append]]) requires one to three arguments.")
    ##TODO: 1.how to replace old value
##getting the ref value requires a function whose parameter has var name and return a string. join() is the only function
## var name is $$2.
## echo a string with "\n" will fail, so we can not use join
    #val = $$join($$2, $$escape_expand(\n))$$escape_expand(\n)
    isEmpty(3)|!isEqual(3, append) {
#system("$$QMAKE_DEL_FILE $$1") #for win commad "del", path format used in qmake such as D:/myfile is not supported, "/" will be treated as an otpion for "del"
        empty_file($$1)
    }
    for(val, $$2) {
        system("echo $$val >> \"$$1\"")|return(false)
    }
    return(true)
}

#defineTest(cache) {
#    !isEmpty(4): error("cache(var, [set|add|sub] [transient] [super], [srcvar]) requires one to three arguments.")
#}

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
    message(BUILD_DIR=$$BUILD_DIR)
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
        CONFIG(release, debug|release): !isEmpty(QMAKE_STRIP): QMAKE_POST_LINK = -$$QMAKE_STRIP $$DESTDIR/$${TARGET}$${EXE_EXT} #.exe in win
    } else: DESTDIR = $$qtLongName($$BUILD_DIR/lib)
    !build_pass {
        message(target: $$DESTDIR/$$TARGET)
        !isEmpty(PROJECTROOT) {
            TRANSLATIONS *= $$PROJECTROOT/i18n/$${TARGET}_zh-cn.ts $$PROJECTROOT/i18n/$${TARGET}_zh_CN.ts
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
