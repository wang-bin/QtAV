
# "avwidgets" module is required only for qwidget apps. QML apps only need "av" module
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += av avwidgets
} else {
  CONFIG += av avwidgets
}
#rpath for apple
mac {
  RPATHDIR *= @loader_path/../Frameworks
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
