/*
 * SDR device I/O base class and various utility functions
 */
#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QWidget>

#include "nanosdr/common/datatypes.h"


/* clang-format off */
// Error codes used for return values
#define SDR_DEVICE_OK           0
#define SDR_DEVICE_ERROR       -1   // Other unspecified error
#define SDR_DEVICE_ELIB        -2   // Error loading driver library
#define SDR_DEVICE_ENOTFOUND   -3   // Device not found
#define SDR_DEVICE_EBUSY       -4   // Device is busy
#define SDR_DEVICE_EPERM       -5   // Insuficcient permissions to open device
#define SDR_DEVICE_EOPEN       -6   // Other error while trying to open device
#define SDR_DEVICE_ENOTAVAIL   -7   // Function not available for this device
#define SDR_DEVICE_ERANGE      -8   // Parameter out of range

// IDs for supported devices
#define SDR_DEVICE_NONE         0
#define SDR_DEVICE_RTLSDR       1
#define SDR_DEVICE_AIRSPY       2
/* clang-format on */

typedef struct {
    bool    is_loaded;
    bool    is_open;
    bool    is_running;
} sdr_device_status_t;

typedef struct {
    quint64     rx_samples;
    quint64     rx_overruns;
} sdr_device_stats_t;

class SdrDevice : public QObject
{
    Q_OBJECT

public:
    explicit SdrDevice(QObject *parent = nullptr);

    virtual int open(QSettings *settings) = 0;
    virtual int close() = 0;

    virtual int         startRx(void);
    virtual int         stopRx(void);
    virtual quint32     getRxSamples(complex_t * buffer, quint32 count);
    virtual QWidget    *getRxControls(void);

    virtual int         setRxFrequency(quint64 freq);

    /* Returns SDR_DEVICE_XYZ */
    virtual int type(void) const;

protected:
    void    clearStatus(sdr_device_status_t &status);
    void    clearStats(sdr_device_stats_t &stats);
};

SdrDevice *sdr_device_create(const QString &device_type);
