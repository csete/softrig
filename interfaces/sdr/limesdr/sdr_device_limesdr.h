/* 
 * LimeSDR backend
 */
#pragma once

#include <QLibrary>
#include <QObject>
#include <QSettings>
#include <QWidget>

#include <stdint.h>
#include <mutex>
#include <thread>

#include "nanosdr/common/ring_buffer.h"
#include "interfaces/sdr/sdr_device.h"
#include "sdr_device_limesdr_api_defs.h"
#include "sdr_device_limesdr_rxctl.h"


class SdrDeviceLimesdr : public SdrDevice
{
    Q_OBJECT
public:
    explicit SdrDeviceLimesdr(QObject *parent = nullptr);
    ~SdrDeviceLimesdr() override;

    int         open(void) override;
    int         close(void) override;
    int         readSettings(const QSettings &s) override;
    int         saveSettings(QSettings &s) override;
    int         startRx(void) override;
    int         stopRx(void) override;
    quint32     getRxSamples(complex_t * buffer, quint32 count) override;
    QWidget    *getRxControls(void) override;

    int         setRxFrequency(quint64 freq) override;
    int         setRxSampleRate(quint32 rate) override;
    int         setRxBandwidth(quint32 bw) override;

private slots:
    void    setRxGain(unsigned int gain);
    void    enableRxLpf(bool enabled);
    void    enableRxGfir(bool enabled);
    void    calibrateRx(void);
    void    calibrateTx(void);

private:
    int     loadDriver(void);
    void    readDeviceLimits(void);
    void    printDeviceLimits(void);
    void    applySettings(void);
    void    readerThread(void);
    void    updateBufferSize(void);

    QLibrary    driver;
    void       *device;

    lms_stream_t    rx_stream;

    SdrDeviceLimesdrRxctl   rx_ctl;

    ring_buffer_t  *reader_buffer;      // raw sample buffer
    std::mutex      reader_lock;
    std::thread    *reader_thread;
    bool            keep_running;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;
    limesdr_settings_t      settings;
    limesdr_info_t          info;

};
