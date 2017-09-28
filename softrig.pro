#
# Qmake project file for sofrig
#
# Common options you may want to passs to qmake:
#
#   CONFIG+=debug          Enable debug mode
#   PREFIX=/some/prefix    Install binary in /PREFIX/bin/
#

QT       += core gui multimedia widgets

TARGET = softrig
TEMPLATE = app

# emit warnings when using deprecated features
DEFINES += QT_DEPRECATED_WARNINGS

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

include(nanosdr/nanosdr.pro)

SOURCES += \
    app/main.cpp \
    app/sdr_thread.cpp \
    gui/control_panel.cpp \
    gui/freq_ctrl.cpp \
    gui/mainwindow.cpp \
    interfaces/audio_output.cpp

HEADERS += \
    app/sdr_thread.h \
    gui/control_panel.h \
    gui/freq_ctrl.h \
    gui/mainwindow.h \
    util/qthread_wrapper.h \
    interfaces/audio_output.h

FORMS += \
    gui/control_panel.ui \
    gui/mainwindow.ui

# make clean target
QMAKE_CLEAN += softrig

# make install target
isEmpty(PREFIX) {
    PREFIX=/usr/local
}
message("Using $${PREFIX} as install prefix")
target.path  = $$PREFIX/bin
INSTALLS    += target
