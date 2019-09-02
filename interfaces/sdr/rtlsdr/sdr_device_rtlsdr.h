/* 
 * RTL-SDR backend
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
#include "sdr_device_rtlsdr_rxctl.h"

class SdrDeviceRtlsdr : public SdrDevice
{
    Q_OBJECT
public:
    explicit SdrDeviceRtlsdr(QObject *parent = nullptr);
    ~SdrDeviceRtlsdr() override;

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
    void    setBias(bool bias_on);
    void    setAgc(bool agc_on);
    void    setDsMode(int mode);

private:
    int     loadDriver(void);
    void    readerThread(void);
    void    startReaderThread(void);
    void    stopReaderThread(void);

    static void readerCallback(unsigned char *buf, uint32_t len, void *data);

    void    setupTunerGains(void);
    void    applySettings(void);

    QLibrary    driver;
    void       *device;

    SdrDeviceRtlsdrRxCtl    rx_ctl;

    ring_buffer_t  *reader_buffer;      // raw sample buffer
    std::mutex      reader_lock;
    std::thread    *reader_thread;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;
    rtlsdr_settings_t       settings;

    int         ds_channel;
    bool        ds_mode_auto;

    bool        has_set_bw;

};
