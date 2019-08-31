/*
 * Airspy backend
 *
 * Copyright  2014-2019  Alexandru Csete OZ9AEC
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
#include "sdr_device_airspy.h"
#include "sdr_device_airspy_api.h"
#include "sdr_device_airspy_fir.h"


#define DEFAULT_GAIN_MODE   "Linearity"
#define DEFAULT_LIN_GAIN    16
#define DEFAULT_SENS_GAIN   16
#define DEFAULT_LNA_GAIN    10
#define DEFAULT_MIX_GAIN    10
#define DEFAULT_VGA_GAIN    10
#define DEFAULT_BIAS        false

#define CFG_KEY_GAIN_MODE   "airspy/gain_mode"
#define CFG_KEY_LIN_GAIN    "airspy/linearity_gain"
#define CFG_KEY_SENS_GAIN   "airspy/sensitivity_gain"
#define CFG_KEY_LNA_GAIN    "airspy/lna_gain"
#define CFG_KEY_MIX_GAIN    "airspy/mixer_gain"
#define CFG_KEY_VGA_GAIN    "airspy/vga_gain"
#define CFG_KEY_BIAS        "airspy/bias_on"


SdrDevice *sdr_device_create_airspy()
{
    return new SdrDeviceAirspy();
}

SdrDevice *sdr_device_create_airspymini()
{
    return new SdrDeviceAirspyMini();
}

/* The callback function is a static method to allow access to private
 * members of the instance.
 */
int SdrDeviceAirspyBase::airspy_rx_callback(airspy_transfer_t *transfer)
{
    // we are in a static method, so we need to get the instance
    SdrDeviceAirspyBase *sdrdev = (SdrDeviceAirspyBase *)transfer->ctx;

    if (transfer->sample_type != AIRSPY_SAMPLE_FLOAT32_IQ)
    {
        qCritical() << "Airspy is running with unsupported sample type:"
                    << transfer->sample_type;
        return -1;
    }

    std::lock_guard<std::mutex> lock(sdrdev->reader_lock);
    ring_buffer_cplx_write(sdrdev->sample_buffer,
                           (complex_t *)transfer->samples,
                           transfer->sample_count);

    sdrdev->stats.rx_samples += quint64(transfer->sample_count);
    if (ring_buffer_cplx_is_full(sdrdev->sample_buffer))
        sdrdev->stats.rx_overruns++;

    return 0;
}

SdrDeviceAirspyBase::SdrDeviceAirspyBase(bool mini, QObject *parent) :
    SdrDevice(parent),
    driver("airspy", this),
    device(nullptr),
    is_mini(mini)
{
    clearStatus(status);
    clearStats(stats);

    sample_buffer = ring_buffer_cplx_create();
    ring_buffer_cplx_init(sample_buffer, 1000000);

    // initialize settings
    settings.frequency = 100e6;
    settings.sample_rate = mini ? 6e6 : 10e6;
    settings.bandwidth = 0;
    settings.gain_mode = DEFAULT_GAIN_MODE;
    settings.linearity_gain = DEFAULT_LIN_GAIN;
    settings.sensitivity_gain = DEFAULT_SENS_GAIN;
    settings.lna_gain = DEFAULT_LNA_GAIN;
    settings.mixer_gain = DEFAULT_MIX_GAIN;
    settings.vga_gain = DEFAULT_VGA_GAIN;
    settings.bias_on = DEFAULT_BIAS;

    // connect rx_ctl signals to slots
    rx_ctl.setEnabled(false);
    connect(&rx_ctl, SIGNAL(gainModeChanged(const QString &)), this, SLOT(saveGainMode(const QString &)));
    connect(&rx_ctl, SIGNAL(linearityGainChanged(int)), this, SLOT(setLinearityGain(int)));
    connect(&rx_ctl, SIGNAL(sensitivityGainChanged(int)), this, SLOT(setSensitivityGain(int)));
    connect(&rx_ctl, SIGNAL(lnaGainChanged(int)), this, SLOT(setLnaGain(int)));
    connect(&rx_ctl, SIGNAL(mixerGainChanged(int)), this, SLOT(setMixerGain(int)));
    connect(&rx_ctl, SIGNAL(vgaGainChanged(int)), this, SLOT(setVgaGain(int)));
    connect(&rx_ctl, SIGNAL(agcToggled(bool)), this, SLOT(setAgc(bool)));
    connect(&rx_ctl, SIGNAL(biasToggled(bool)), this, SLOT(setBiasTee(bool)));
}

SdrDeviceAirspyBase::~SdrDeviceAirspyBase()
{
    if (status.is_running)
        stopRx();

    if (status.is_open)
        close();

    if (status.is_loaded)
        driver.unload();

    ring_buffer_cplx_delete(sample_buffer);
}

int SdrDeviceAirspyBase::open()
{
    int     result;

    if (status.is_running || status.is_open)
        return SDR_DEVICE_EBUSY;

    if (!status.is_loaded)
    {
        if (loadDriver())
            return SDR_DEVICE_ELIB;
        status.is_loaded = true;
    }

    qDebug() << "Opening Airspy device";
    result = airspy_open(&device);
    if (result != AIRSPY_SUCCESS)
    {
        qCritical("airspy_open() failed with code %d (%s)", result,
                  airspy_error_name((enum airspy_error)result));
        return SDR_DEVICE_EOPEN;
    }

    status.is_open = true;
    rx_ctl.setEnabled(true);

    applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::close(void)
{
    int     ret;

    if (!status.is_open)
        return SDR_DEVICE_ERROR;

    qDebug() << "Closing Airspy device";
    ret = airspy_close(device);
    if (ret)
        qCritical() << "airspy_close() returned" << ret;

    status.is_open = false;
    rx_ctl.setEnabled(false);

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::readSettings(const QSettings &s)
{
    bool    conv_ok;
    int     int_val;

    settings.gain_mode = s.value(CFG_KEY_GAIN_MODE, DEFAULT_GAIN_MODE).toString();

    int_val = s.value(CFG_KEY_LIN_GAIN, DEFAULT_LIN_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.linearity_gain = int_val;

    int_val = s.value(CFG_KEY_SENS_GAIN, DEFAULT_SENS_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.sensitivity_gain = int_val;

    int_val = s.value(CFG_KEY_LNA_GAIN, DEFAULT_LNA_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.lna_gain = int_val;

    int_val = s.value(CFG_KEY_MIX_GAIN, DEFAULT_MIX_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.mixer_gain = int_val;

    int_val = s.value(CFG_KEY_VGA_GAIN, DEFAULT_VGA_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.vga_gain = int_val;

    settings.bias_on = s.value(CFG_KEY_BIAS, DEFAULT_BIAS).toBool();

    if (status.is_open)
        applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::saveSettings(QSettings &s)
{
    if (settings.gain_mode == DEFAULT_GAIN_MODE)
        s.remove(CFG_KEY_GAIN_MODE);
    else
        s.setValue(CFG_KEY_GAIN_MODE, settings.gain_mode);

    if (settings.linearity_gain == DEFAULT_LIN_GAIN)
        s.remove(CFG_KEY_LIN_GAIN);
    else
        s.setValue(CFG_KEY_LIN_GAIN, settings.linearity_gain);

    if (settings.sensitivity_gain == DEFAULT_SENS_GAIN)
        s.remove(CFG_KEY_SENS_GAIN);
    else
        s.setValue(CFG_KEY_SENS_GAIN, settings.sensitivity_gain);

    if (settings.lna_gain == DEFAULT_LNA_GAIN)
        s.remove(CFG_KEY_LNA_GAIN);
    else
        s.setValue(CFG_KEY_LNA_GAIN,settings.lna_gain);

    if (settings.mixer_gain == DEFAULT_MIX_GAIN)
        s.remove(CFG_KEY_MIX_GAIN);
    else
        s.setValue(CFG_KEY_MIX_GAIN, settings.mixer_gain);

    if (settings.vga_gain == DEFAULT_VGA_GAIN)
        s.remove(CFG_KEY_VGA_GAIN);
    else
        s.setValue(CFG_KEY_VGA_GAIN, settings.vga_gain);

    if (settings.bias_on == DEFAULT_BIAS)
        s.remove(CFG_KEY_BIAS);
    else
        s.setValue(CFG_KEY_BIAS, settings.bias_on);

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::startRx(void)
{
    int result;

    if (!status.is_open)
        return SDR_DEVICE_ERROR;

    if (status.is_running)
        return SDR_DEVICE_OK;

    qDebug() << "Starting Airspy...";

    status.is_running = true;

    result = airspy_start_rx(device, airspy_rx_callback, this);
    if (result != AIRSPY_SUCCESS)
    {
        qCritical("airspy_start_rx() failed with code %d: %s", result,
                  airspy_error_name((enum airspy_error)result));
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::stopRx(void)
{
    int result;

    if (!status.is_running)
        return SDR_DEVICE_OK;

    qDebug() << "Stopping Airspy...";

    status.is_running = false;

    result = airspy_stop_rx(device);
    if (result != AIRSPY_SUCCESS)
    {
        qCritical() << "airspy_stop_rx() failed with code" << result
                    << airspy_error_name((enum airspy_error)result);
            return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

quint32 SdrDeviceAirspyBase::getRxSamples(complex_t * buffer, quint32 count)
{
    std::lock_guard<std::mutex> lock(reader_lock);

    if (!buffer || count == 0)
        return 0;

    if (count > ring_buffer_cplx_count(sample_buffer))
        return 0;

    ring_buffer_cplx_read(sample_buffer, buffer, count);

    return count;
}

QWidget *SdrDeviceAirspyBase::getRxControls(void)
{
    return  &rx_ctl;
}

int SdrDeviceAirspyBase::setRxFrequency(quint64 freq)
{
    int result;

    if (freq < 24e6 || freq > 1750e6)
        return SDR_DEVICE_ERANGE;

    settings.frequency = freq;
    if (!status.is_open)
        return SDR_DEVICE_OK;

    result = airspy_set_freq(device, uint32_t(freq));
    if (result != AIRSPY_SUCCESS)
    {
        qCritical() << "airspy_set_freq failed" << freq
                    << airspy_error_name((enum airspy_error)result);
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::setRxSampleRate(quint32 rate)
{
    int result;

    qInfo() << __func__ << rate;

    if (is_mini)
    {
        if (rate != 3.0e6 && rate != 6.0e6 && rate != 10.0e6)
            return SDR_DEVICE_ERANGE;
    }
    else if (rate != 2.5e6 && rate != 10.0e6)
        return SDR_DEVICE_ERANGE;

    settings.sample_rate = rate;
    if (!status.is_open)
        return SDR_DEVICE_OK;

    result = airspy_set_samplerate(device, rate);
    if (result != AIRSPY_SUCCESS)
    {
        qCritical() << "airspy_set_samplerate failed" << rate
                    << airspy_error_name((enum airspy_error)result);
        return SDR_DEVICE_ERROR;
    }
    setRxBandwidth(settings.bandwidth);

    return SDR_DEVICE_OK;
}

int SdrDeviceAirspyBase::setRxBandwidth(quint32 bw)
{
    int          result;
    int          decim;
    int          size;
    const float *kernel;

    qInfo() << __func__ << bw;

    settings.bandwidth = bw;
    if (!status.is_open)
        return SDR_DEVICE_OK;

    decim = bw ? settings.sample_rate / bw : 1;

    if (decim < 4)
    {
        kernel = KERNEL_2_80;
        size = KERNEL_2_80_LEN;
    }
    else if (decim < 8)
    {
        kernel = KERNEL_4_90;
        size = KERNEL_4_90_LEN;
    }
    else if (decim < 16)
    {
        kernel = KERNEL_8_100;
        size = KERNEL_8_100_LEN;
    }
    else
    {
        kernel = KERNEL_16_110;
        size = KERNEL_16_110_LEN;
    }

    qInfo("Airspy BW = %d, decim = %d, kernel size = %d", bw, decim, size);
    result = airspy_set_conversion_filter_float32(device, kernel, size);
    if (result != AIRSPY_SUCCESS)
    {
        qInfo() << "Error setting airspy conversion filter" << result
                << airspy_error_name((enum airspy_error)result);
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

void SdrDeviceAirspyBase::saveGainMode(const QString &mode)
{
    settings.gain_mode = mode;
}

void SdrDeviceAirspyBase::setLinearityGain(int gain)
{
    int result;

    settings.linearity_gain = gain;
    result = airspy_set_linearity_gain(device, quint8(gain));
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting liearity gain to" << gain << ":"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setSensitivityGain(int gain)
{
    int result;

    settings.sensitivity_gain = gain;
    result = airspy_set_sensitivity_gain(device, quint8(gain));
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting sensitivity gain to" << gain << ":"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setLnaGain(int gain)
{
    int result;

    settings.lna_gain = gain;
    result = airspy_set_lna_gain(device, quint8(gain));
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting LNA gain to" << gain << ":"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setMixerGain(int gain)
{
    int result;

    settings.mixer_gain = gain;
    result = airspy_set_mixer_gain(device, quint8(gain));
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting mixer gain to" << gain << ":"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setVgaGain(int gain)
{
    int result;

    settings.vga_gain = gain;
    result = airspy_set_vga_gain(device, quint8(gain));
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting VGA gain to" << gain << ":"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setAgc(bool enabled)
{
    int result;

    result = airspy_set_lna_agc(device, enabled);
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting LNA AGC:"
                << airspy_error_name((enum airspy_error)result);
    result = airspy_set_mixer_agc(device, enabled);
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting mixer AGC:"
                << airspy_error_name((enum airspy_error)result);
}

void SdrDeviceAirspyBase::setBiasTee(bool enabled)
{
    int result;

    settings.bias_on = enabled;
    result = airspy_set_rf_bias(device, enabled);
    if (result != AIRSPY_SUCCESS)
        qInfo() << "Error setting Biat Tee:"
                << airspy_error_name((enum airspy_error)result);
}

/* used to apply cached settings after device is opened */
void SdrDeviceAirspyBase::applySettings(void)
{
    qDebug() << __func__;
    setRxSampleRate(settings.sample_rate);
    setRxFrequency(settings.frequency);
    rx_ctl.readSettings(settings);
}

#define SYMBOL_EMSG "Error loading symbol address for"
int SdrDeviceAirspyBase::loadDriver(void)
{
    if (!driver.isLoaded())
    {
        qInfo() << "Loading Airspy driver library";
        if (!driver.load())
        {
            qCritical() << "Error loading Airspy driver library";
            return 1;
        }
    }

    airspy_lib_version = reinterpret_cast<void (*)(airspy_lib_version_t *)>(driver.resolve("airspy_lib_version"));
    if (airspy_lib_version == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_lib_version";
        return 1;
    }

    // load and check library version
    airspy_lib_version(&lib_ver);
    qInfo("OK (library is version: %d.%d.%d)", lib_ver.major_version,
          lib_ver.minor_version, lib_ver.revision);

    if (lib_ver.major_version != AIRSPY_VER_MAJOR ||
            lib_ver.minor_version != AIRSPY_VER_MINOR)
    {
        qInfo("NOTE: Backend uses API version %d.%d\n", AIRSPY_VER_MAJOR, AIRSPY_VER_MINOR);
    }

    qInfo() << "Loading symbols from Airspy library";

    airspy_open = reinterpret_cast<int (*)(struct airspy_device **)>(driver.resolve("airspy_open"));
    if (airspy_open == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_open";
        return 1;
    }

    airspy_close = reinterpret_cast<int (*)(void *)>(driver.resolve("airspy_close"));
    if (airspy_close == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_close";
        return 1;
    }

    airspy_set_samplerate = reinterpret_cast<int (*)(void *, uint32_t)>(driver.resolve("airspy_set_samplerate"));
    if (airspy_set_samplerate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_samplerate";
        return 1;
    }

    airspy_set_conversion_filter_float32 =
        reinterpret_cast<int (*)(void *, const float *, uint32_t)>(driver.resolve("airspy_set_conversion_filter_float32"));
    if (airspy_set_conversion_filter_float32 == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_conversion_filter_float32";
        return 1;
    }

    airspy_start_rx = reinterpret_cast<int (*)(void *, airspy_sample_block_cb_fn,void *)>(driver.resolve("airspy_start_rx"));
    if (airspy_start_rx == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_start_rx";
        return 1;
    }

    airspy_stop_rx = reinterpret_cast<int (*)(void *)>(driver.resolve("airspy_stop_rx"));
    if (airspy_stop_rx == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_stop_rx";
        return 1;
    }

    airspy_is_streaming = reinterpret_cast<int (*)(void *)>(driver.resolve("airspy_is_streaming"));
    if (airspy_is_streaming == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_is_streaming";
        return 1;
    }

    airspy_set_sample_type = reinterpret_cast<int (*)(void *, enum airspy_sample_type)>
            (driver.resolve("airspy_set_sample_type"));
    if (airspy_set_sample_type == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_sample_type";
        return 1;
    }

    airspy_set_freq = reinterpret_cast<int (*)(void *, uint32_t)>(driver.resolve("airspy_set_freq"));
    if (airspy_set_freq == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_freq";
        return 1;
    }

    airspy_set_linearity_gain = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_linearity_gain"));
    if (airspy_set_linearity_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_linearity_gain";
        return 1;
    }

    airspy_set_sensitivity_gain = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_sensitivity_gain"));
    if (airspy_set_sensitivity_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_sensitivity_gain";
        return 1;
    }

    airspy_set_lna_gain = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_lna_gain"));
    if (airspy_set_lna_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_lna_gain";
        return 1;
    }

    airspy_set_mixer_gain = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_mixer_gain"));
    if (airspy_set_mixer_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_mixer_gain";
        return 1;
    }

    airspy_set_vga_gain = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_vga_gain"));
    if (airspy_set_vga_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_vga_gain";
        return 1;
    }

    airspy_set_lna_agc = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_lna_agc"));
    if (airspy_set_lna_agc == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_lna_agc";
        return 1;
    }

    airspy_set_mixer_agc = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_mixer_agc"));
    if (airspy_set_mixer_agc == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_mixer_agc";
        return 1;
    }

    airspy_set_rf_bias = reinterpret_cast<int (*)(void *, uint8_t)>(driver.resolve("airspy_set_rf_bias"));
    if (airspy_set_rf_bias == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_set_rf_bias";
        return 1;
    }

    airspy_error_name = reinterpret_cast<char *(*)(enum airspy_error)>(driver.resolve("airspy_error_name"));
    if (airspy_error_name == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "airspy_error_name";
        return 1;
    }

    return 0;
}
