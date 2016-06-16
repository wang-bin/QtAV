########## template for QtAV app project BEGIN ################
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += avwidgets
} else {
  CONFIG += avwidgets
}
#rpath for osx qt4
mac {
  RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
########## template for QtAV app project END ################
################# Add your own code below ###################
include(src.pri)
