
NANOSDR_VERSION = 0.58

message(Building with nanosdr version $${NANOSDR_VERSION})

INCLUDEPATH += nanosdr

##win32:LIBS += "???"
unix:LIBS += -ldl

## FIXME: Nanosdr build files define endianness and OS
DEFINES += NANOSDR_OS_LITTLE_ENDIAN
DEFINES += LINUX

## FIXME: speexdsp
CONFIG += link_pkgconfig
PKGCONFIG += speexdsp

NANODSP_HEADERS += \
    nanosdr/nanodsp/agc.h \
    nanosdr/nanodsp/amdemod.h \
#    nanosdr/nanodsp/apt_demod.h \
    nanosdr/nanodsp/cute_fft.h \
    nanosdr/nanodsp/fastfir.h \
    nanosdr/nanodsp/fft.h \
    nanosdr/nanodsp/filter/decimator.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_70.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_100.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_140.h \
#    nanosdr/nanodsp/filtercoef.h \
    nanosdr/nanodsp/fir.h \
    nanosdr/nanodsp/kiss_fft.h \
    nanosdr/nanodsp/_kiss_fft_guts.h \
    nanosdr/nanodsp/nfm_demod.h \
    nanosdr/nanodsp/smeter.h \
    nanosdr/nanodsp/ssbdemod.h \
    nanosdr/nanodsp/translate.h

NANODSP_SOURCES += \
    nanosdr/nanodsp/agc.cpp \
    nanosdr/nanodsp/amdemod.cpp \
#    nanosdr/nanodsp/apt_demod.cpp \
    nanosdr/nanodsp/cute_fft.cpp \
    nanosdr/nanodsp/fastfir.cpp \
    nanosdr/nanodsp/fft.cpp \
    nanosdr/nanodsp/filter/decimator.cpp \
    nanosdr/nanodsp/fir.cpp \
    nanosdr/nanodsp/kiss_fft.c \
    nanosdr/nanodsp/nfm_demod.cpp \
    nanosdr/nanodsp/smeter.cpp \
    nanosdr/nanodsp/translate.cpp

HEADERS += \
    $${NANODSP_HEADERS} \
    nanosdr/common/bithacks.h \
    nanosdr/common/datatypes.h \
    nanosdr/common/library_loader.h \
    nanosdr/common/ring_buffer.h \
    nanosdr/common/ring_buffer_cplx.h \
    nanosdr/common/sdr_data.h \
    nanosdr/common/thread_class.h \
    nanosdr/common/time.h \
    nanosdr/common/util.h \
#    nanosdr/interfaces/nanosdr_protocol.h \
#    nanosdr/interfaces/sdr_ctl.h \
#    nanosdr/interfaces/sdr_ctl_queue.h \
    nanosdr/interfaces/sdr_device.h \
    nanosdr/interfaces/sdr_device_rtlsdr_reader.h \
    nanosdr/fft_thread.h \
    nanosdr/receiver.h \
    nanosdr/sdriq/sdriq.h \
    nanosdr/sdriq/sdriq_parser.h

SOURCES += \
    $${NANODSP_SOURCES} \
#    nanosdr/interfaces/nanosdr_protocol.c \
#    nanosdr/interfaces/sdr_ctl_queue.cpp \
    nanosdr/interfaces/sdr_device_airspy.cpp \
    nanosdr/interfaces/sdr_device_file.cpp \
    nanosdr/interfaces/sdr_device_rtlsdr.cpp \
    nanosdr/interfaces/sdr_device_rtlsdr_reader.c \
    nanosdr/interfaces/sdr_device_sdriq.cpp \
    nanosdr/interfaces/sdr_device_stdin.cpp \
    nanosdr/fft_thread.cpp \
    nanosdr/receiver.cpp \
    nanosdr/sdriq/sdriq.c
