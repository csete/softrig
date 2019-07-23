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
#include "sdr_device.h"
#include "sdr_device_rtlsdr_rxctl.h"

class SdrDeviceRtlsdr : public SdrDevice
{
    Q_OBJECT
public:
    explicit SdrDeviceRtlsdr(QObject *parent = nullptr);
    ~SdrDeviceRtlsdr() override;

    int open(QSettings *settings) override;
    int close(void) override;

    int         startRx(void) override;
    int         stopRx(void) override;
    quint32     getRxSamples(complex_t * buffer, quint32 count) override;
    QWidget    *getRxControls(void) override;

    int         setRxFrequency(quint64 freq) override;

    int type(void) const override;

private slots:
    void    setRxGain(int gain);
    void    setBias(bool bias_on);
    void    setAgc(bool agc_on);

private:
    int     loadDriver(void);
    void    readerThread(void);
    void    startReaderThread(void);
    void    stopReaderThread(void);

    static void readerCallback(unsigned char *buf, uint32_t len, void *data);

    void    setupTunerGains(void);

    QLibrary    driver;
    void       *device;

    SdrDeviceRtlsdrRxCtl    rx_ctl;

    ring_buffer_t  *reader_buffer;      // raw sample buffer
    std::mutex      reader_lock;
    std::thread    *reader_thread;
    bool            reader_running;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;

    bool        has_set_bw;

    quint32     sample_rate;
};
