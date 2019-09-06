/*
 * SDRplay backend
 */
#pragma once

#include <QLibrary>
#include <QObject>
#include <QSettings>
#include <QWidget>

#include <mutex>

#include "nanosdr/common/ring_buffer.h"
#include "interfaces/sdr/sdr_device.h"
#include "sdr_device_sdrplay_api_defs.h"
#include "sdr_device_sdrplay_rxctl.h"

class SdrDeviceSdrplay : public SdrDevice
{
    Q_OBJECT

public:
    SdrDeviceSdrplay(QObject *parent = nullptr);
    ~SdrDeviceSdrplay() override;

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
    void    enableDebug(bool enabled);
    void    setLnaState(int lna_state);
    void    setGrdb(int grdb);

private:
    int     loadDriver(void);
    void    updateBufferSize(void);
    bool    bandChangeNeeded(double new_freq) const;
    void    updateGain(void);

    mir_sdr_ErrT    applySettings(mir_sdr_ReasonForReinitT reason);

    static void streamCallback(short *xi, short *xq, unsigned int first_sample_num,
                               int gr_changed, int rf_changed, int fs_changed,
                               unsigned int num_samples, unsigned int reset,
                               unsigned int hw_removed, void *ctx);
    static void gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void *ctx);


    QLibrary                driver;

    SdrDeviceSdrplayRxctl   rx_ctl;

    ring_buffer_t  *sample_buffer;
    std::mutex      reader_lock;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;
    sdrplay_settings_t      settings;

    unsigned char           hw_ver;

    struct state {
        mir_sdr_BandT       current_band;
    };
};
