CONFIG += qt
QT += core gui opengl
CONFIG += console

SOURCES += main.cpp

win32 {
	LIBS += -lopengl32
} else:macx {
	LIBS += -framework OpenGL -framework AGL
} else {
	LIBS += -lGL
}
