/*
 * SDRplay backend
 *
 * Copyright  2019  Alexandru Csete OZ9AEC
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <QDebug>

#include <stdint.h>

#include "common/ring_buffer_cplx.h"
#include "sdr_device_sdrplay.h"
#include "sdr_device_sdrplay_api.h"


#define DEFAULT_BANDWIDTH   mir_sdr_BW_1_536
#define DEFAULT_LNA_STATE   2
#define DEFAULT_GRDB        40
#define DEFAULT_GAIN_MODE   mir_sdr_USE_RSP_SET_GR
#define DEFAULT_LO_MODE     mir_sdr_LO_Auto
#define DEFAULT_IF_MODE     mir_sdr_IF_Zero

#define CFG_KEY_LNA_STATE   "sdrplay/lna_state"
#define CFG_KEY_GRDB        "sdrplay/gain_reduction"
#define CFG_KEY_GAIN_MODE   "sdrplay/gain_mode"
#define CFG_KEY_LO_MODE     "sdrplay/lo_mode"
#define CFG_KEY_IF_MODE     "sdrplay/if_mode"


SdrDevice *sdr_device_create_sdrplay()
{
    return new SdrDeviceSdrplay();
}


SdrDeviceSdrplay::SdrDeviceSdrplay(QObject *parent) :
    SdrDevice(parent),
    driver("mirsdrapi-rsp", this)
{
    clearStatus(status);
    clearStats(stats);

    // initialize settings
    settings.frequency = 100000000;
    settings.sample_rate = 2000000;
    settings.bandwidth = DEFAULT_BANDWIDTH;
    settings.lna_state = DEFAULT_LNA_STATE;
    settings.grdb = DEFAULT_GRDB;
    settings.gain_mode = DEFAULT_GAIN_MODE;
    settings.lo_mode = DEFAULT_LO_MODE;
    settings.if_mode = DEFAULT_IF_MODE;

    sample_buffer = ring_buffer_cplx_create();
    ring_buffer_cplx_init(sample_buffer, quint32(0.5 * settings.sample_rate));

    // connect rx_ctl signals to slots
    rx_ctl.setEnabled(false);
    connect(&rx_ctl, SIGNAL(lnaStateChanged(int)), this, SLOT(setLnaState(int)));
    connect(&rx_ctl, SIGNAL(grdbChanged(int)), this, SLOT(setGrdb(int)));
    connect(&rx_ctl, SIGNAL(debugChanged(bool)), this, SLOT(enableDebug(bool)));
}

SdrDeviceSdrplay::~SdrDeviceSdrplay()
{
    if (status.rx_is_running)
        stopRx();

    if (status.device_is_open)
        close();

    if (status.driver_is_loaded)
        driver.unload();

    ring_buffer_cplx_delete(sample_buffer);
}

// There is no specific "open" function in the SDRplay API; however, it seems
// to be necessary to select a device by index before calling the init()
// function, otherwise the device will be in an unknown state
int SdrDeviceSdrplay::open()
{
    mir_sdr_DeviceT devices[4];
    unsigned int    num_devices;
    int             result;

    if (status.rx_is_running || status.device_is_open)
        return SDR_DEVICE_EBUSY;

    if (!status.driver_is_loaded)
    {
        if (loadDriver())
            return SDR_DEVICE_ELIB;
        status.driver_is_loaded = true;
    }

    // detect available devices and select the first one
    qDebug() << "Detecting SDRplay devices...";
    result = mir_sdr_GetDevices(devices, &num_devices, 4);
    if (result != mir_sdr_Success)
    {
        qCritical() << "Error detecting SDRplay devices" << result;
        return SDR_DEVICE_EOPEN;
    }

    if (num_devices == 0)
    {
        qCritical() << "Found no SDRplay devices";
        return SDR_DEVICE_EOPEN;
    }

    qInfo("Found %d device%s", num_devices, num_devices > 1 ? "s:" : ":");
    for (unsigned int i = 0; i < num_devices; i++)
        qInfo("  %s    %s    %d    %d", devices[i].SerNo, devices[i].DevNm,
              devices[i].hwVer, devices[i].devAvail);

    result = mir_sdr_SetDeviceIdx(0);
    if (result != mir_sdr_Success)
    {
        qCritical() << "Error selecting SDRplay device" << result;
        return SDR_DEVICE_EOPEN;
    }

    status.device_is_open = true;
    rx_ctl.setEnabled(true);

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::close(void)
{
    int     result;

    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    qDebug() << "Releasing SDRplay device";
    result = mir_sdr_ReleaseDeviceIdx();
    if (result != mir_sdr_Success)
        qInfo() << "Failed to release SDRplay device" << result;

    status.device_is_open = false;
    rx_ctl.setEnabled(false);

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::readSettings(const QSettings &s)
{
    bool    conv_ok;
    int     int_val;

    int_val = s.value(CFG_KEY_LNA_STATE, DEFAULT_LNA_STATE).toInt(&conv_ok);
    if (conv_ok)
        settings.lna_state = int_val;

    int_val = s.value(CFG_KEY_GRDB, DEFAULT_GRDB).toInt(&conv_ok);
    if (conv_ok)
        settings.grdb = int_val;

    int_val = s.value(CFG_KEY_GAIN_MODE, DEFAULT_GAIN_MODE).toInt(&conv_ok);
    if (conv_ok)
        settings.gain_mode = mir_sdr_SetGrModeT(int_val);

    int_val = s.value(CFG_KEY_LO_MODE, DEFAULT_LO_MODE).toInt(&conv_ok);
    if (conv_ok)
        settings.lo_mode = mir_sdr_LoModeT(int_val);

    int_val = s.value(CFG_KEY_IF_MODE, DEFAULT_IF_MODE).toInt(&conv_ok);
    if (conv_ok)
        settings.if_mode = mir_sdr_If_kHzT(int_val);

    if (status.rx_is_running)
        applySettings(mir_sdr_ReasonForReinitT(0x7F));

    rx_ctl.readSettings(settings);

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::saveSettings(QSettings &s)
{
    if (settings.lna_state == DEFAULT_LNA_STATE)
        s.remove(CFG_KEY_LNA_STATE);
    else
        s.setValue(CFG_KEY_LNA_STATE, settings.lna_state);

    if (settings.grdb == DEFAULT_GRDB)
        s.remove(CFG_KEY_GRDB);
    else
        s.setValue(CFG_KEY_GRDB, settings.grdb);

    if (settings.gain_mode == DEFAULT_GAIN_MODE)
        s.remove(CFG_KEY_GAIN_MODE);
    else
        s.setValue(CFG_KEY_GAIN_MODE, settings.gain_mode);

    if (settings.lo_mode == DEFAULT_LO_MODE)
        s.remove(CFG_KEY_LO_MODE);
    else
        s.setValue(CFG_KEY_LO_MODE, settings.lo_mode);

    if (settings.if_mode == DEFAULT_IF_MODE)
        s.remove(CFG_KEY_IF_MODE);
    else
        s.setValue(CFG_KEY_IF_MODE, settings.if_mode);

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::startRx(void)
{
    mir_sdr_ErrT    result;
    int     sps;
    int     grdb, grdb_sys = 0;

    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    if (status.rx_is_running)
        return SDR_DEVICE_OK;

    // we can not enable extended gain range before device is initialized
    grdb = settings.grdb < mir_sdr_NORMAL_MIN_GR ? mir_sdr_NORMAL_MIN_GR : settings.grdb;

    qDebug() << "Starting SDRplay...";
    result = mir_sdr_StreamInit(&grdb,
                                1.e-6 * double(settings.sample_rate),
                                1.e-6 * double(settings.frequency),
                                settings.bandwidth, settings.if_mode,
                                DEFAULT_LNA_STATE, &grdb_sys,
                                settings.gain_mode, &sps,
                                streamCallback, nullptr/*gainChangeCallback*/,
                                this);
    if (result != mir_sdr_Success)
    {
        qInfo() << "mir_sdr_StreamInit() failed with error code" << result;
        return SDR_DEVICE_EINIT;
    }

    status.rx_is_running = true;

    result = mir_sdr_RSP_SetGrLimits(mir_sdr_EXTENDED_MIN_GR);
    if (result != mir_sdr_Success)
        qInfo() << "Failed to enable extended gain range" << result;
    else
        updateGain();

    result = mir_sdr_GetHwVersion(&hw_ver);
    if (result != mir_sdr_Success)
        qInfo() << "Failed to read hardware version";
    else
        qInfo() << "SDRplay hardware version" << hw_ver;

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::stopRx(void)
{
    mir_sdr_ErrT    result;

    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    qDebug() << "Stopping SDRplay...";
    result = mir_sdr_StreamUninit();
    if (result != mir_sdr_Success)
        qInfo() << "mir_sdr_StreamUninit() failed with error code" << result;

    ring_buffer_cplx_clear(sample_buffer);
    status.rx_is_running = false;

    return SDR_DEVICE_OK;
}

quint32 SdrDeviceSdrplay::getRxSamples(complex_t * buffer, quint32 count)
{
    std::lock_guard<std::mutex> lock(reader_lock);

    if (!buffer || count == 0)
        return 0;

    if (count > ring_buffer_cplx_count(sample_buffer))
        return 0;

    ring_buffer_cplx_read(sample_buffer, buffer, count);

    return count;
}

QWidget *SdrDeviceSdrplay::getRxControls(void)
{
    return  &rx_ctl;
}

int SdrDeviceSdrplay::setRxFrequency(quint64 freq)
{
    mir_sdr_ErrT    result;

    if (freq < 1e3 || freq > 2e9)
        return SDR_DEVICE_ERANGE;

    if (freq == settings.frequency)
        return SDR_DEVICE_OK;

    settings.frequency = freq;
    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    if (bandChangeNeeded(freq))
    {
        result = applySettings(mir_sdr_CHANGE_RF_FREQ);
        if (result != mir_sdr_Success)
        {
            qCritical() << "Faled to set RF frequency to" << freq;
            return SDR_DEVICE_ERROR;
        }
    }
    else
    {
        result = mir_sdr_SetRf(1.e-6 * settings.frequency, 1, 0);
        if (result != mir_sdr_Success)
        {
            qCritical() << "Faled to move RF frequency to" << freq;
            return SDR_DEVICE_ERROR;
        }
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceSdrplay::setRxSampleRate(quint32 rate)
{
    mir_sdr_ErrT    result;

    if (rate < 2e6 || rate > 10e6)
        return SDR_DEVICE_ERANGE;

    if (rate == settings.sample_rate)
        return SDR_DEVICE_OK;

    settings.sample_rate = rate;
    updateBufferSize();
    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    result = applySettings(mir_sdr_CHANGE_FS_FREQ);
    if (result != mir_sdr_Success)
    {
        qCritical() << "Faled to set sample rate to" << rate;
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

static mir_sdr_Bw_MHzT bwType(quint32 bw)
{
    if (bw > 100e3 && bw < 250e3)
        return mir_sdr_BW_0_200;
    else if (bw < 350e3)
        return mir_sdr_BW_0_300;
    else if (bw < 700e3)
        return mir_sdr_BW_0_600;
    else if (bw < 2000e3)
        return mir_sdr_BW_1_536;
    else if (bw < 5500e3)
        return mir_sdr_BW_5_000;
    else if (bw < 6500e3)
        return mir_sdr_BW_6_000;
    else if (bw < 7500e3)
        return mir_sdr_BW_7_000;
    else if (bw < 8500e3)
        return mir_sdr_BW_8_000;
    else
        return mir_sdr_BW_Undefined;
}

int SdrDeviceSdrplay::setRxBandwidth(quint32 bw)
{
    mir_sdr_ErrT    result;
    mir_sdr_Bw_MHzT new_bw;

    new_bw = bwType(bw);
    if (new_bw == settings.bandwidth)
        return SDR_DEVICE_OK;

    settings.bandwidth = new_bw;
    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    result = applySettings(mir_sdr_CHANGE_BW_TYPE);
    if (result != mir_sdr_Success)
    {
        qCritical() << "Faled to set bandwidth to" << bw;
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

void SdrDeviceSdrplay::updateGain(void)
{
    mir_sdr_ErrT    result;
    mir_sdr_GainValuesT sys_gain;

    result = mir_sdr_RSP_SetGr(settings.grdb, settings.lna_state, 1, 0);
    if (result != mir_sdr_Success)
    {
        qCritical() << "Failed to update gain to"
                    << settings.lna_state << settings.grdb;
    }

    result = mir_sdr_GetCurrentGain(&sys_gain);
    if (result == mir_sdr_Success)
        rx_ctl.setSystemGainValue(sys_gain.curr);
    else
        qInfo() << "Could not read the system gain" << result;
}

void SdrDeviceSdrplay::setLnaState(int lna_state)
{
    settings.lna_state = lna_state;
    if (status.rx_is_running)
        updateGain();
}

void SdrDeviceSdrplay::setGrdb(int grdb)
{
    settings.grdb = grdb;
    if (status.rx_is_running)
        updateGain();
}

void SdrDeviceSdrplay::updateBufferSize(void)
{

    std::lock_guard<std::mutex> lock(reader_lock);
    quint32 new_size = quint32(0.5 * settings.sample_rate); // 500 msec

    if (new_size == 0 || new_size == ring_buffer_cplx_size(sample_buffer))
        return;

    ring_buffer_cplx_clear(sample_buffer);
    ring_buffer_cplx_resize(sample_buffer, new_size);
}

// returns the band according to the table in sec 6 of API docs
static int band(double freq)
{
    if (freq < 60.)         // AM
        return 1;
    else if (freq < 120.)   // VHF
        return 2;
    else if (freq < 250.)   // Band 3
        return 3;
    else if (freq < 420.)   // Band X
        return 4;
    else if (freq < 1000.)  // Band 4/5
        return 5;
    else if (freq < 2000.)  // L-band
        return 6;
    else
        return 0;
}

// Returns TRUE if new_freq is in a different tuner band
bool SdrDeviceSdrplay::bandChangeNeeded(double new_freq) const
{
    return band(new_freq) == band(settings.frequency);
}

mir_sdr_ErrT SdrDeviceSdrplay::applySettings(mir_sdr_ReasonForReinitT reason)
{
    int     sps;
    int     grdb_sys;
    mir_sdr_ErrT result;

    result =  mir_sdr_Reinit(&settings.grdb,
                             1.e-6 * double(settings.sample_rate),
                             1.e-6 * double(settings.frequency),
                             settings.bandwidth,
                             settings.if_mode,
                             settings.lo_mode,
                             settings.lna_state, &grdb_sys,
                             settings.gain_mode,
                             &sps, reason);
    return result;
}

void SdrDeviceSdrplay::streamCallback(short *xi, short *xq, unsigned int first_sample_num,
                                      int gr_changed, int rf_changed, int fs_changed,
                                      unsigned int num_samples, unsigned int reset,
                                      unsigned int hw_removed, void *ctx)
{
#define WORK_BUF_SIZE 8192
#define SAMPLE_SCALE  1.0f / 32767.5f

    Q_UNUSED(first_sample_num);
    Q_UNUSED(gr_changed);
    Q_UNUSED(rf_changed);
    Q_UNUSED(fs_changed);

    SdrDeviceSdrplay   *this_radio = reinterpret_cast<SdrDeviceSdrplay *>(ctx);
    complex_t           work_buf[WORK_BUF_SIZE];
    unsigned int        i;

    if (hw_removed)
    {
        // signal stopped
    }

    if (num_samples > WORK_BUF_SIZE)
    {
        qCritical() << "SdrDeviceSdrplay::streamCallback work buffer too small";
        num_samples = WORK_BUF_SIZE;
    }

    for (i = 0; i < num_samples; i++)
    {
        work_buf[i].re = (float(xi[i]) - 0.5f) * SAMPLE_SCALE;
        work_buf[i].im = (float(xq[i]) - 0.5f) * SAMPLE_SCALE;
    }

    if (num_samples > ring_buffer_cplx_size(this_radio->sample_buffer))
        num_samples = ring_buffer_cplx_size(this_radio->sample_buffer);

    this_radio->reader_lock.lock();
    if (reset)
        ring_buffer_cplx_clear(this_radio->sample_buffer);
    ring_buffer_cplx_write(this_radio->sample_buffer, work_buf, num_samples);
    this_radio->stats.rx_samples += num_samples;
    if (ring_buffer_cplx_is_full(this_radio->sample_buffer))
        this_radio->stats.rx_overruns++;
    this_radio->reader_lock.unlock();
}

void SdrDeviceSdrplay::gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB,
                                          void *ctx)
{
    Q_UNUSED(gRdB);
    Q_UNUSED(lnaGRdB);
    Q_UNUSED(ctx);
    return;
}

void SdrDeviceSdrplay::enableDebug(bool enabled)
{
    if (!status.device_is_open)
        return;

    mir_sdr_DebugEnable(enabled);
}

#define SYMBOL_EMSG "Error loading symbol address for"
int SdrDeviceSdrplay::loadDriver(void)
{
    float   api_version;

    if (!driver.isLoaded())
    {
        qInfo() << "Loading SDRplay driver library";
        if (!driver.load())
        {
            qCritical() << "Error loading SDRplay driver library";
            return 1;
        }
    }

    mir_sdr_ApiVersion = reinterpret_cast<mir_sdr_ErrT (*)(float *)>(driver.resolve("mir_sdr_ApiVersion"));
    if (mir_sdr_ApiVersion == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_ApiVersion";
        return 1;
    }

    // load and check library version
    if (mir_sdr_ApiVersion(&api_version) != mir_sdr_Success)
    {
        qCritical() << "Error reading library version";
        return 1;
    }

    qInfo("Library version is %.2f, the backend is using %.2f",
          double(api_version), double(MIR_SDR_API_VERSION));

    qInfo() << "Loading symbols from SDRplay library";

    mir_sdr_SetRf = reinterpret_cast<mir_sdr_ErrT (*)(double, int, int)>(driver.resolve("mir_sdr_SetRf"));
    if (mir_sdr_SetRf == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_SetRf";
        return 1;
    }

    mir_sdr_SetFs = reinterpret_cast<mir_sdr_ErrT (*)(double, int, int, int)>(driver.resolve("mir_sdr_SetFs"));
    if (mir_sdr_SetFs == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_SetFs";
        return 1;
    }

    mir_sdr_StreamInit = reinterpret_cast<mir_sdr_ErrT (*)(int *, double, double, mir_sdr_Bw_MHzT, mir_sdr_If_kHzT, int, int *, mir_sdr_SetGrModeT, int *, mir_sdr_StreamCallback_t, mir_sdr_GainChangeCallback_t, void *)>(driver.resolve("mir_sdr_StreamInit"));
    if (mir_sdr_StreamInit == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_StreamInit";
        return 1;
    }

    mir_sdr_StreamUninit = reinterpret_cast<mir_sdr_ErrT (*)(void)>(driver.resolve("mir_sdr_StreamUninit"));
    if (mir_sdr_StreamUninit == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_StreamUninit";
        return 1;
    }

    mir_sdr_Reinit = reinterpret_cast<mir_sdr_ErrT (*)(int *, double, double, mir_sdr_Bw_MHzT, mir_sdr_If_kHzT, mir_sdr_LoModeT, int LNAstate, int *, mir_sdr_SetGrModeT, int *, mir_sdr_ReasonForReinitT)>(driver.resolve("mir_sdr_Reinit"));
    if (mir_sdr_Reinit == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_Reinit";
        return 1;
    }

    mir_sdr_DebugEnable = reinterpret_cast<mir_sdr_ErrT (*)(unsigned int)>(driver.resolve("mir_sdr_DebugEnable"));
    if (mir_sdr_DebugEnable == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_DebugEnable";
        return 1;
    }

    mir_sdr_GetCurrentGain = reinterpret_cast<mir_sdr_ErrT (*)(mir_sdr_GainValuesT *)>(driver.resolve("mir_sdr_GetCurrentGain"));
    if (mir_sdr_GetCurrentGain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_GetCurrentGain";
        return 1;
    }

    mir_sdr_GetDevices = reinterpret_cast<mir_sdr_ErrT (*)(mir_sdr_DeviceT *, unsigned int *, unsigned int)>(driver.resolve("mir_sdr_GetDevices"));
    if (mir_sdr_GetDevices == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_GetDevices";
        return 1;
    }

    mir_sdr_SetDeviceIdx = reinterpret_cast<mir_sdr_ErrT (*)(unsigned int)>(driver.resolve("mir_sdr_SetDeviceIdx"));
    if (mir_sdr_SetDeviceIdx == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_GetHwVersion";
        return 1;
    }

    mir_sdr_ReleaseDeviceIdx = reinterpret_cast<mir_sdr_ErrT (*)(void)>(driver.resolve("mir_sdr_ReleaseDeviceIdx"));
    if (mir_sdr_ReleaseDeviceIdx == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_ReleaseDeviceIdx";
        return 1;
    }

    mir_sdr_GetHwVersion = reinterpret_cast<mir_sdr_ErrT (*)(unsigned char *)>(driver.resolve("mir_sdr_GetHwVersion"));
    if (mir_sdr_GetHwVersion == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_GetHwVersion";
        return 1;
    }

    mir_sdr_RSP_SetGr = reinterpret_cast<mir_sdr_ErrT (*)(int, int, int, int)>(driver.resolve("mir_sdr_RSP_SetGr"));
    if (mir_sdr_RSP_SetGr == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_RSP_SetGr";
        return 1;
    }

    mir_sdr_RSP_SetGrLimits = reinterpret_cast<mir_sdr_ErrT (*)(mir_sdr_MinGainReductionT)>(driver.resolve("mir_sdr_RSP_SetGrLimits"));
    if (mir_sdr_RSP_SetGrLimits == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "mir_sdr_RSP_SetGrLimits";
        return 1;
    }

    return 0;
}
