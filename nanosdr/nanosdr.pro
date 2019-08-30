
NANOSDR_VERSION = 0.86

message(Building with nanosdr version $${NANOSDR_VERSION})

INCLUDEPATH += nanosdr

##win32:LIBS += "???"
unix:LIBS += -ldl

## FIXME: Nanosdr build files define endianness and OS
DEFINES += NANOSDR_OS_LITTLE_ENDIAN
DEFINES += LINUX

NANODSP_HEADERS += \
    nanosdr/nanodsp/agc.h \
    nanosdr/nanodsp/amdemod.h \
    nanosdr/nanodsp/cute_fft.h \
    nanosdr/nanodsp/fastfir.h \
    nanosdr/nanodsp/fft.h \
    nanosdr/nanodsp/filter/decimator.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_70.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_100.h \
    nanosdr/nanodsp/filter/filtercoef_hbf_140.h \
    nanosdr/nanodsp/fir.h \
    nanosdr/nanodsp/fract_resampler.h \
    nanosdr/nanodsp/kiss_fft.h \
    nanosdr/nanodsp/_kiss_fft_guts.h \
    nanosdr/nanodsp/nfm_demod.h \
    nanosdr/nanodsp/smeter.h \
    nanosdr/nanodsp/ssbdemod.h \
    nanosdr/nanodsp/translate.h

NANODSP_SOURCES += \
    nanosdr/nanodsp/agc.cpp \
    nanosdr/nanodsp/amdemod.cpp \
    nanosdr/nanodsp/cute_fft.cpp \
    nanosdr/nanodsp/fastfir.cpp \
    nanosdr/nanodsp/fft.cpp \
    nanosdr/nanodsp/filter/decimator.cpp \
    nanosdr/nanodsp/fir.cpp \
    nanosdr/nanodsp/fract_resampler.cpp \
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
    nanosdr/fft_thread.h \
    nanosdr/receiver.h

SOURCES += \
    $${NANODSP_SOURCES} \
    nanosdr/fft_thread.cpp \
    nanosdr/receiver.cpp
