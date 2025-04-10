#-------------------------------------------------
#
# Project created by QtCreator 2021-04-12T14:35:30
#
#-------------------------------------------------

QT  += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MembraneVibration
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG(debug, debug|release) {
  DESTDIR = bin.debug
  OBJECTS_DIR = bin.debug
}
CONFIG(release, debug|release) {
  DESTDIR = bin
  OBJECTS_DIR = bin
}

win32 {
  QMAKE_POST_LINK += $${QMAKE_COPY} membrane_vibration.glsl $$DESTDIR &
  QMAKE_POST_LINK += $${QMAKE_COPY} membrane_vibration.vert $$DESTDIR &
  QMAKE_POST_LINK += $${QMAKE_COPY} membrane_vibration.gsh  $$DESTDIR &
  QMAKE_POST_LINK += $${QMAKE_COPY} membrane_vibration.frag $$DESTDIR &
}


MOC_DIR = ./
RCC_DIR = ./
UI_DIR = ./

SOURCES += \
        main.cpp \
        widget.cpp

HEADERS += \
        widget.h


DISTFILES += \
    README.md \
    membrane_vibration.gsh \
    membrane_vibration.vert \
    membrane_vibration.frag \
    membrane_vibration.glsl
