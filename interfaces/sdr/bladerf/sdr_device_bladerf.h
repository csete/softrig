/*
 * BladeRF backend
 */
#pragma once

#include <QLibrary>
#include <QObject>
#include <QSettings>
#include <QWidget>

#include <mutex>
#include <thread>

#include "nanosdr/common/ring_buffer.h"
#include "interfaces/sdr/sdr_device.h"
#include "sdr_device_bladerf_api_defs.h"
#include "sdr_device_bladerf_rxctl.h"


class SdrDeviceBladerf : public SdrDevice
{
    Q_OBJECT

public:
    SdrDeviceBladerf(QObject *parent = nullptr);
    ~SdrDeviceBladerf() override;

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
    void    setRxGain(int gain);
    void    setBias(bladerf_channel ch, bool enable);

private:
    int     loadDriver(void);
    void    updateRxBufferSize(void);
    void    readDeviceInfo();
    void    printDeviceInfo();
    void    applySettings();
    void    readerThread(void);

    QLibrary                driver;
    struct bladerf         *device;
    SdrDeviceBladerfRxctl   rx_ctl;

    ring_buffer_t  *reader_buffer;
    std::mutex      reader_lock;
    std::thread    *reader_thread;
    bool            keep_running;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;
    bladerf_settings_t      settings;
    bladerf_info_t          device_info;
};
