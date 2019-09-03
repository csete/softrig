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
CONFIG += c++11

SOFTRIG_VERSION = $$system(./git-version-gen .tarball-version)
message(Configuring softrig version $${SOFTRIG_VERSION})
SOFTRIG_VERSION_STR = '\\"$${SOFTRIG_VERSION}\\"'
DEFINES += VERSION=\"$${SOFTRIG_VERSION_STR}\"

# emit warnings when using deprecated features
DEFINES += QT_DEPRECATED_WARNINGS

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

include(nanosdr/nanosdr.pro)

SOURCES += \
    app/app_config.cpp \
    app/main.cpp \
    app/sdr_thread.cpp \
    gui/control_panel.cpp \
    gui/device_config_dialog.cpp \
    gui/fft_widget.cpp \
    gui/freq_ctrl.cpp \
    gui/mainwindow.cpp \
    gui/ssi_widget.cpp \
    gui/tmp_bookmarks.cpp \
    gui/tmp_plotter.cpp \
    interfaces/audio_output.cpp \
    interfaces/sdr/sdr_device.cpp \
    interfaces/sdr/airspy/sdr_device_airspy.cpp \
    interfaces/sdr/airspy/sdr_device_airspy_rxctl.cpp \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr.cpp \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr_rxctl.cpp

HEADERS += \
    app/app_config.h \
    app/config_keys.h \
    app/sdr_thread.h \
    gui/control_panel.h \
    gui/device_config_dialog.h \
    gui/fft_widget.h \
    gui/freq_ctrl.h \
    gui/mainwindow.h \
    gui/ssi_widget.h \
    gui/tmp_bookmarks.h \
    gui/tmp_plotter.h \
    util/qthread_wrapper.h \
    interfaces/audio_output.h \
    interfaces/sdr/sdr_device.h \
    interfaces/sdr/airspy/sdr_device_airspy.h \
    interfaces/sdr/airspy/sdr_device_airspy_api.h \
    interfaces/sdr/airspy/sdr_device_airspy_api_defs.h \
    interfaces/sdr/airspy/sdr_device_airspy_fir.h \
    interfaces/sdr/airspy/sdr_device_airspy_rxctl.h \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr.h \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr_api.h \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr_rxctl.h

FORMS += \
    gui/control_panel.ui \
    gui/device_config_dialog.ui \
    interfaces/sdr/airspy/sdr_device_airspy_rxctl.ui \
    interfaces/sdr/rtlsdr/sdr_device_rtlsdr_rxctl.ui

# make clean target
QMAKE_CLEAN += softrig

# make install target
isEmpty(PREFIX) {
    PREFIX=/usr/local
}
message("Using $${PREFIX} as install prefix")
target.path  = $$PREFIX/bin
INSTALLS    += target
