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
#define SDR_DEVICE_EINIT       -7   // Error trying to initialize device
#define SDR_DEVICE_ENOTAVAIL   -8   // Function not available for this device
#define SDR_DEVICE_ERANGE      -9   // Parameter out of range
/* clang-format on */

typedef struct {
    bool    driver_is_loaded;
    bool    device_is_open;
    bool    rx_is_running;
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

    virtual int         open(void) = 0;
    virtual int         close(void) = 0;
    virtual int         readSettings(const QSettings &s) = 0;
    virtual int         saveSettings(QSettings &s) = 0;
    virtual int         startRx(void);
    virtual int         stopRx(void);
    virtual quint32     getRxSamples(complex_t * buffer, quint32 count);
    virtual QWidget    *getRxControls(void);

    virtual int         setRxFrequency(quint64 freq);
    virtual int         setRxSampleRate(quint32 rate);
    virtual int         setRxBandwidth(quint32 bw);

protected:
    void    clearStatus(sdr_device_status_t &status);
    void    clearStats(sdr_device_stats_t &stats);
};

SdrDevice *sdr_device_create(const QString &device_type);
