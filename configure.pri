#Designed by Wang Bin(Lucas Wang). 2013 <wbsecg1@gmail.com>

### ONLY FOR Qt4. common.pri must be included before it so that write_file() can be used#######
### .qmake.cache MUST be created before it!
####ASSUME compile tests and .qmake.cache is in project out root dir

# Ensure that a cache is present. If none was found on startup, this will create
# one in the build directory of the project which loads this feature.
#cache()

QMAKE_CONFIG_LOG = $$dirname(_QMAKE_CACHE_QT4_)/config.log
QMAKE_CONFIG_TESTS_DIR = $$_PRO_FILE_PWD_/config.tests

lessThan(QT_MAJOR_VERSION, 5) {
defineTest(cache) {
    !isEmpty(4): error("cache(var, [set|add|sub] [transient] [super], [srcvar]) requires one to three arguments.")
    !exists($$_QMAKE_CACHE_QT4_) {
        log("Info: creating cache file $$_QMAKE_CACHE_QT4_")
        write_file($$_QMAKE_CACHE_QT4_)|return(false)
    }
    isEmpty(1):return(true)
    !isEmpty(2):isEqual(2, set):mode_set=1
    isEmpty(3) {
        isEmpty(mode_set):error("cache(): modes other than 'set' require a source variable.")
        srcvar = $$1
        srcval = $$join($$srcvar, $$escape_expand(\\t)) ##TODO: how to use white space?
    } else {
        srcvar = $$3
        dstvar = $$1
        #We need the ref value, so the function's parameter must be a var name. join() is!
        srcval = $$join($$srcvar, $$escape_expand(\\t)) ##TODO: how to use white space?
        isEqual(2, add) {
            varstr = "$$1 += $$srcval" ##TODO: how to use white space?
        } else:isEqual(2, sub) {
            varstr = "$$1 -= $$srcval"
        } else:isEqual(2, set) {
            ##TODO: erase existing VAR in cache
            varstr = "$$1 = $$srcval"
        } else {
            error("cache(): invalid flag $$2.")
        }
    }
    #log("varstr: $$varstr")
##TODO: remove existing lines contain $$srcvar
    #because write_file() will write 1 line for each value(separated by space), so the value must be closed with "", then it's 1 value, not list
#erase the existing var and value pair
    win32 {#:isEmpty(QMAKE_SH) { #windows sucks. can not access the cache

    } else {
#use sed for unix or msys
#convert '/' to '\/' for sed
        srcval ~= s,/,\\/,g
        srcval ~= s,\\+,\\\\+,g #for sed regexp. '+' in qmake is '\+' ?
        system("sed -i -r '/.*$${dstvar}.*$${srcval}.*/d' $$_QMAKE_CACHE_QT4_ >/dev/null")
    }
    write_file($$_QMAKE_CACHE_QT4_, varstr, append)
}
} #Qt4
QMAKE_MAKE = $$(MAKE)
!isEmpty(QMAKE_MAKE) {
    # We were called recursively. Use the right make, as MAKEFLAGS may be set as well.
} equals(MAKEFILE_GENERATOR, UNIX) {
    QMAKE_MAKE = make
} else:equals(MAKEFILE_GENERATOR, MINGW) {
    !equals(QMAKE_HOST.os, Windows): \
        QMAKE_MAKE = make
    else: \
        QMAKE_MAKE = mingw32-make
} else:if(equals(MAKEFILE_GENERATOR, MSVC.NET)|equals(MAKEFILE_GENERATOR, MSBUILD)) {
    QMAKE_MAKE = nmake
} else {
# error: the reset qmake will not work and displays nothing in qtc
    warning("Configure tests are not supported with the $$MAKEFILE_GENERATOR Makefile generator.")
}

defineTest(qtRunLoggedCommand) {
    msg = "+ $$1"
    write_file($$QMAKE_CONFIG_LOG, msg, append)#|error("write file failed") #for debug
    system("$$1 >> \"$$QMAKE_CONFIG_LOG\" 2>&1")|return(false)
    return(true)
}

# Try to build the test project in $$QMAKE_CONFIG_TESTS_DIR/$$1
# ($$_PRO_FILE_PWD_/config.tests/$$1 by default).
#
# If the test passes, config_$$1 will be added to CONFIG.
# The result is automatically cached. Use of cached results
# can be suppressed by passing CONFIG+=recheck to qmake.
#
# Returns: true iff the test passes
defineTest(qtCompileTest) {
    positive = config_$$1
    done = done_config_$$1

    $$done:!recheck {
        $$positive:return(true)
        return(false)
    }

    log("Checking for $${1}... ")
    msg = "executing config test $$1"
    write_file($$QMAKE_CONFIG_LOG, msg, append)
    test_dir = $$QMAKE_CONFIG_TESTS_DIR/$$1
    test_out_dir = $$shadowed($$test_dir)
    #system always call win32 cmd in windows, so we need "cd /d" to ensure success cd to a different partition
    contains(QMAKE_HOST.os,Windows):test_cmd_base = "cd /d $$system_quote($$system_path($$test_out_dir)) &&"
    else: test_cmd_base = "cd $$system_quote($$system_path($$test_out_dir)) &&"

# On WinRT we need to change the entry point as we cannot create windows applications
  winrt {
     qmake_configs += " \"QMAKE_LFLAGS+=/ENTRY:main\""
  }
    # Disable qmake features which are typically counterproductive for tests
    qmake_configs = "\"CONFIG -= qt debug_and_release app_bundle lib_bundle\""
    iphoneos: qmake_configs += "\"CONFIG+=iphoneos\""
    iphonesimulator: qmake_configs += "\"CONFIG+=iphonesimulator\""
    # Clean up after previous run
    exists($$test_out_dir/Makefile):qtRunLoggedCommand("$$test_cmd_base $$QMAKE_MAKE distclean")

    mkpath($$test_out_dir)#|error("Aborting.") #mkpath currently return false, do not know why
    SPEC =
    !isEmpty(QMAKESPEC): SPEC = "-spec $$QMAKESPEC"
    qtRunLoggedCommand("$$test_cmd_base $$system_quote($$system_path($$QMAKE_QMAKE)) $$SPEC $$qmake_configs $$system_path($$test_dir)") {
        qtRunLoggedCommand("$$test_cmd_base $$QMAKE_MAKE") {
            log("yes$$escape_expand(\\n)")
            msg = "test $$1 succeeded"
            write_file($$QMAKE_CONFIG_LOG, msg, append)

            !$$positive {
                CONFIG += $$positive
                cache(CONFIG, add, positive)
            }
            !$$done {
                CONFIG += $$done
                cache(CONFIG, add, done)
            }
            export(CONFIG)
            return(true)
        }
    }

    log("no$$escape_expand(\\n)")
    msg = "test $$1 FAILED"
    write_file($$QMAKE_CONFIG_LOG, msg, append)

    $$positive {
        CONFIG -= $$positive
        cache(CONFIG, sub, positive)
    }
    !$$done {
        CONFIG += $$done
        cache(CONFIG, add, done)
    }
    export(CONFIG)
    return(false)
}
