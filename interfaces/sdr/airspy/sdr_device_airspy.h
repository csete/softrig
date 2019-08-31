/*
 * Airspy backend
 */
#pragma once

#include <QLibrary>
#include <QObject>
#include <QSettings>
#include <QWidget>

#include <mutex>

#include "nanosdr/common/ring_buffer.h"
#include "interfaces/sdr/sdr_device.h"
#include "sdr_device_airspy_api.h"
#include "sdr_device_airspy_rxctl.h"

class SdrDeviceAirspyBase : public SdrDevice
{
    Q_OBJECT

public:
    SdrDeviceAirspyBase(bool mini, QObject *parent);
    ~SdrDeviceAirspyBase() override;

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

    int type(void) const override
    {
        return is_mini ? SDR_DEVICE_AIRSPYMINI : SDR_DEVICE_AIRSPY;
    }

private slots:
    void    saveGainMode(const QString &mode);
    void    setLinearityGain(int gain);
    void    setSensitivityGain(int gain);
    void    setLnaGain(int gain);
    void    setMixerGain(int gain);
    void    setVgaGain(int gain);
    void    setAgc(bool enabled);
    void    setBiasTee(bool enabled);

private:
    int     loadDriver(void);
    void    freeMemory(void);

    static int airspy_rx_callback(airspy_transfer_t *transfer);

    void    applySettings(void);

    QLibrary                driver;
    struct airspy_device   *device;
    SdrDeviceAirspyRxctl    rx_ctl;
    airspy_lib_version_t    lib_ver;
    bool                    is_mini;

    ring_buffer_t  *sample_buffer;
    std::mutex      reader_lock;

    sdr_device_status_t     status;
    sdr_device_stats_t      stats;
    airspy_settings_t       settings;

};

class SdrDeviceAirspy : public SdrDeviceAirspyBase
{
public:
    SdrDeviceAirspy(QObject *parent = nullptr) :
        SdrDeviceAirspyBase(false, parent)
    {
    }
};

class SdrDeviceAirspyMini : public SdrDeviceAirspyBase
{
public:
    SdrDeviceAirspyMini(QObject *parent = nullptr) :
        SdrDeviceAirspyBase(true, parent)
    {
    }
};
