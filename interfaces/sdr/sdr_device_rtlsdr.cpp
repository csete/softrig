/*
 * RTL-SDR backend
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2018-2019 Alexandru Csete OZ9AEC
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
#include <new>      // std::nothrow
#include <QDebug>

#include "sdr_device_rtlsdr.h"
#include "sdr_device_rtlsdr_api.h"


#define DEFAULT_GAIN    297
#define DEFAULT_DS_MODE RXCTL_DS_MODE_AUTO_Q
#define DEFAULT_AGC     false
#define DEFAULT_BIAS    false

#define CFG_KEY_MANUAL_GAIN     "rtlsdr/manual_gain"
#define CFG_KEY_DS_MODE         "rtlsdr/ds_mode"
#define CFG_KEY_AGC_ENABLED     "rtlsdr/agc_enabled"
#define CFG_KEY_BIAS_ENABLED    "rtlsdr/bias_enabled"

SdrDevice *sdr_device_create_rtlsdr()
{
    return new SdrDeviceRtlsdr();
}

SdrDeviceRtlsdr::SdrDeviceRtlsdr(QObject *parent) : SdrDevice(parent),
    driver("rtlsdr", this),
    device(nullptr),
    rx_ctl(nullptr),
    reader_thread(nullptr),
    reader_running(false),
    has_set_bw(false)
{
    clearStatus(status);
    clearStats(stats);

    reader_buffer = ring_buffer_create();
    ring_buffer_init(reader_buffer, 16384);

    settings.frequency = 100e6;
    settings.sample_rate = 2400000;
    settings.bandwidth = 0;
    settings.gain = DEFAULT_GAIN;
    settings.ds_mode = DEFAULT_DS_MODE;
    settings.agc_on = DEFAULT_AGC;
    settings.bias_on = DEFAULT_BIAS;

    connect(&rx_ctl, SIGNAL(gainChanged(int)), this, SLOT(setRxGain(int)));
    connect(&rx_ctl, SIGNAL(biasToggled(bool)), this, SLOT(setBias(bool)));
    connect(&rx_ctl, SIGNAL(agcToggled(bool)), this, SLOT(setAgc(bool)));
    connect(&rx_ctl, SIGNAL(dsModeChanged(int)), this, SLOT(setDsMode(int)));
    rx_ctl.setEnabled(false);
}

SdrDeviceRtlsdr::~SdrDeviceRtlsdr()
{
    if (status.is_running)
        stopRx();

    if (status.is_open)
        close();

    if (status.is_loaded)
        driver.unload();

    ring_buffer_delete(reader_buffer);
}

int SdrDeviceRtlsdr::open()
{
    int     ret;

    if (status.is_running || status.is_open)
        return SDR_DEVICE_EBUSY;

    if (!status.is_loaded)
    {
        if (loadDriver())
            return SDR_DEVICE_ELIB;
        status.is_loaded = true;
    }

    qDebug() << "Opening RTL-SDR device";
    ret = rtlsdr_open(&device, 0);
    if (ret)
    {
        qCritical() << "rtlsdr_open() returned" << ret;
        return SDR_DEVICE_EOPEN;
    }

    status.is_open = true;
    rx_ctl.setEnabled(true);

    setupTunerGains();
    applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::close(void)
{
    int     ret;

    if (!status.is_open)
        return SDR_DEVICE_ERROR;

    qDebug() << "Closing RTL-SDR device";
    ret = rtlsdr_close(device);
    if (ret)
        qCritical() << "rtlsdr_close() returned" << ret;

    status.is_open = false;
    rx_ctl.setEnabled(false);

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::readSettings(const QSettings &s)
{
    bool    conv_ok;
    int     int_val;

    int_val = s.value(CFG_KEY_MANUAL_GAIN, DEFAULT_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.gain = int_val;

    int_val = s.value(CFG_KEY_DS_MODE, DEFAULT_DS_MODE).toInt(&conv_ok);
    if (conv_ok)
        settings.ds_mode = int_val;

    settings.agc_on = s.value(CFG_KEY_AGC_ENABLED, DEFAULT_AGC).toBool();
    settings.bias_on = s.value(CFG_KEY_BIAS_ENABLED, DEFAULT_BIAS).toBool();

    if (status.is_open)
        applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::saveSettings(QSettings &s)
{
    if (settings.gain == DEFAULT_GAIN)
        s.remove(CFG_KEY_MANUAL_GAIN);
    else
        s.setValue(CFG_KEY_MANUAL_GAIN, settings.gain);

    if (settings.ds_mode == DEFAULT_DS_MODE)
        s.remove(CFG_KEY_DS_MODE);
    else
        s.setValue(CFG_KEY_DS_MODE, settings.ds_mode);

    if (settings.agc_on == DEFAULT_AGC)
        s.remove(CFG_KEY_AGC_ENABLED);
    else
        s.setValue(CFG_KEY_AGC_ENABLED, settings.agc_on);

    if (settings.bias_on == DEFAULT_BIAS)
        s.remove(CFG_KEY_BIAS_ENABLED);
    else
        s.setValue(CFG_KEY_BIAS_ENABLED, settings.bias_on);

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::startRx(void)
{
    if (!status.is_open)
        return SDR_DEVICE_ERROR;

    if (status.is_running)
        return SDR_DEVICE_OK;

    status.is_running = true;
    startReaderThread();

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::stopRx(void)
{
    if (!status.is_running)
        return SDR_DEVICE_OK;

    status.is_running = false;
    stopReaderThread();

    return SDR_DEVICE_OK;
}

quint32 SdrDeviceRtlsdr::getRxSamples(complex_t * buffer, quint32 count)
{
    uint8_t buf[480000];  // FIXME
    real_t *workbuf = reinterpret_cast<real_t *>(buffer);
    quint32 byte_count = 2 * count;
    quint32 i;

    std::lock_guard<std::mutex> lock(reader_lock);

    if (!buffer || count == 0)
        return 0;

    if (byte_count > ring_buffer_count(reader_buffer))
        return 0;

    ring_buffer_read(reader_buffer, buf, byte_count);

    for (i = 0; i < byte_count; i++)
        workbuf[i] = (real_t(buf[i]) - 127.4f) / 127.5f;  // FIXME: Use LUT

    return count;
}

QWidget *SdrDeviceRtlsdr::getRxControls(void)
{
    return &rx_ctl;
}

int SdrDeviceRtlsdr::setRxFrequency(quint64 freq)
{
    int result;

    if (!status.is_open)
    {
        settings.frequency = freq;
        return SDR_DEVICE_OK;
    }

    if (ds_mode_auto)
    {
        if (freq < 24e6 && rtlsdr_get_direct_sampling(device) != ds_channel)
        {
            if ((result = rtlsdr_set_direct_sampling(device, ds_channel)))
                qInfo() << "Note: rtlsdr_set_direct_sampling returned" << result;
        }
        else if (freq >= 24e6 && rtlsdr_get_direct_sampling(device) != DS_CHANNEL_NONE)
        {
            if ((result = rtlsdr_set_direct_sampling(device, DS_CHANNEL_NONE)))
                qInfo() << "Note: rtlsdr_set_direct_sampling returned" << result;

            // tuner has been reset so we need to set gain again
            setRxGain(settings.gain);
        }
    }

    if (rtlsdr_set_center_freq(device, uint32_t(freq)))
    {
        qInfo() << "Failed to set RTL-SDR frequency to" << freq;
        settings.frequency = rtlsdr_get_center_freq(device);
        return SDR_DEVICE_ERANGE;
    }
    settings.frequency = freq;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::setRxSampleRate(quint32 rate)
{
    if (!status.is_open)
    {
        settings.sample_rate = rate;
        return SDR_DEVICE_OK;
    }

    if (rtlsdr_set_sample_rate(device, rate))
    {
        qInfo() << "Failed to set RTL-SDR sample rate to" << rate;
        settings.sample_rate = rtlsdr_get_sample_rate(device);
        return SDR_DEVICE_ERANGE;
    }
    settings.sample_rate = rate;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::setRxBandwidth(quint32 bw)
{
    if (!has_set_bw)
        return SDR_DEVICE_ENOTAVAIL;

    if (!status.is_open)
    {
        settings.bandwidth = bw;
        return SDR_DEVICE_OK;
    }

    if (rtlsdr_set_tuner_bandwidth(device, bw))
    {
        qInfo() << "Failed to set RTL-SDR bandwidth to" << bw;
        settings.bandwidth = 0;
        return SDR_DEVICE_ERANGE;
    }
    settings.bandwidth = bw;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::type(void) const
{
    return SDR_DEVICE_RTLSDR;
}

void SdrDeviceRtlsdr::setRxGain(int gain)
{
    settings.gain = gain;
    if (rtlsdr_set_tuner_gain(device, gain))
        qInfo() << "Error setting RTL-SDR tuner gain to" << gain;
}

void SdrDeviceRtlsdr::setBias(bool bias_on)
{
    settings.bias_on = bias_on;
    if (rtlsdr_set_bias_tee(device, bias_on))
        qInfo() << "Error setting RTL-SDR bias tee to" << (bias_on ? "ON" : "OFF");
}

void SdrDeviceRtlsdr::setAgc(bool agc_on)
{
    settings.agc_on = agc_on;
    if (rtlsdr_set_tuner_gain_mode(device, agc_on ? 0 : 1) ||
        rtlsdr_set_agc_mode(device, agc_on ? 1 : 0))
    {
        qInfo() << "Error setting RTL-SDR AGC to" << (agc_on ? "ON" : "OFF");
    }
}

void SdrDeviceRtlsdr::setDsMode(int mode)
{
    settings.ds_mode = mode;
    switch (mode) {
    default:
    case RXCTL_DS_MODE_AUTO_Q:
        ds_mode_auto = true;
        ds_channel = DS_CHANNEL_Q;
        break;
    case RXCTL_DS_MODE_AUTO_I:
        ds_mode_auto = true;
        ds_channel = DS_CHANNEL_I;
        break;
    case RXCTL_DS_MODE_Q:
        ds_mode_auto = false;
        ds_channel = DS_CHANNEL_Q;
        break;
    case RXCTL_DS_MODE_I:
        ds_mode_auto = false;
        ds_channel = DS_CHANNEL_I;
        break;
    case RXCTL_DS_MODE_OFF:
        ds_mode_auto = false;
        ds_channel = DS_CHANNEL_NONE;
        break;
    }

    if (ds_mode_auto)
    {
        // reset frequency to ensure settings are applied
        quint32 freq = rtlsdr_get_center_freq(device);
        setRxFrequency(freq);
    }
    else
    {
        int result = rtlsdr_set_direct_sampling(device, ds_channel);
        if (result)
            qInfo() << "Note: rtlsdr_set_direct_sampling returned" << result;

        if (ds_channel == DS_CHANNEL_NONE)
            // tuner has been reset so we need to set gain again
            setRxGain(settings.gain);
    }
}

void SdrDeviceRtlsdr::readerCallback(unsigned char *buf, uint32_t count, void *ctx)
{
    SdrDeviceRtlsdr *this_backend = reinterpret_cast<SdrDeviceRtlsdr *>(ctx);

    this_backend->reader_lock.lock();
    this_backend->stats.rx_samples += count / 2;
    ring_buffer_write(this_backend->reader_buffer, buf, count);
    if (ring_buffer_is_full(this_backend->reader_buffer))
        this_backend->stats.rx_overruns++;
    this_backend->reader_lock.unlock();
}

void SdrDeviceRtlsdr::readerThread(void)
{
    quint32     samprate;
    quint32     buflen;

    qInfo() << "Entering RTL-SDR reader thread";

    // aim for 20-40 ms buffers but in multiples of 16k
    samprate = rtlsdr_get_sample_rate(device);
    if (samprate < 1e6)
        buflen = 16384;
    else if (samprate < 2e6)
        buflen = 4 * 16384;
    else
        buflen = 6 * 16384;

    ring_buffer_resize(reader_buffer, 4 * buflen);

    // FIXME: return values
    rtlsdr_reset_buffer(device);
    rtlsdr_read_async(device, &SdrDeviceRtlsdr::readerCallback, this, 0, buflen);

    qInfo() << "Exiting RTL-SDR reader thread";
}

void SdrDeviceRtlsdr::startReaderThread(void)
{
    qInfo() << "Starting RTL-SDR reader thread";
    if (!reader_thread)
        reader_thread = new std::thread(&SdrDeviceRtlsdr::readerThread, this);
}

void SdrDeviceRtlsdr::stopReaderThread(void)
{
    qInfo() << "Stopping RTL-SDR reader thread";

    if (reader_thread)
    {
        // FIXME: return values
        rtlsdr_cancel_async(device);
        reader_thread->join();
        delete reader_thread;
        reader_thread = nullptr;
    }
}

void SdrDeviceRtlsdr::setupTunerGains(void)
{
    int    *gains;
    int     count;

    count = rtlsdr_get_tuner_gains(device, nullptr);
    if (count > 0)
    {
        gains = new(std::nothrow) int[count];
        if (!gains)
            return;

        if (rtlsdr_get_tuner_gains(device, gains) != count)
        {
            qCritical() << "rtlsdr_get_tuner_gains() returned different counts on consecutive calls";
        }
        else
        {
            rx_ctl.setTunerGains(gains, count);
        }

        delete[] gains;
    }
}

/* used to apply cached settings after device is opened */
void SdrDeviceRtlsdr::applySettings(void)
{
    setRxFrequency(settings.frequency);
    setRxSampleRate(settings.sample_rate);
    setAgc(settings.agc_on);
    setBias(settings.bias_on);
    setRxGain(settings.gain);
    setDsMode(settings.ds_mode);
    rx_ctl.readSettings(settings);
}

#define SYMBOL_EMSG "Error loading symbol address for"
int SdrDeviceRtlsdr::loadDriver(void)
{
    if (!driver.isLoaded())
    {
        qDebug() << "Loading RTL-SDR driver library";
        if (!driver.load())
        {
            qCritical() << "Error loading RTL-SDR driver library";
            return 1;
        }
    }

    qDebug() << "Loading symbols from RTL-SDR library";
    rtlsdr_open = reinterpret_cast<int (*)(void **, uint32_t)>(driver.resolve("rtlsdr_open"));
    if (rtlsdr_open == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_open";
        return 1;
    }

    rtlsdr_close = reinterpret_cast<int (*)(void *)>(driver.resolve("rtlsdr_close"));
    if (rtlsdr_close == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_close";
        return 1;
    }

    rtlsdr_set_sample_rate = reinterpret_cast<int (*)(void *, uint32_t)>(driver.resolve("rtlsdr_set_sample_rate"));
    if (rtlsdr_set_sample_rate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_sample_rate";
        return 1;
    }

    rtlsdr_get_sample_rate = reinterpret_cast<uint32_t (*)(void *)>(driver.resolve("rtlsdr_get_sample_rate"));
    if (rtlsdr_get_sample_rate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_sample_rate";
        return 1;
    }

    rtlsdr_set_center_freq = reinterpret_cast<int (*)(void *, uint32_t)>(driver.resolve("rtlsdr_set_center_freq"));
    if (rtlsdr_set_center_freq == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_center_freq";
        return 1;
    }

    rtlsdr_get_center_freq = reinterpret_cast<uint32_t (*)(void *)>(driver.resolve("rtlsdr_get_center_freq"));
    if (rtlsdr_get_center_freq == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_center_freq";
        return 1;
    }

    rtlsdr_set_freq_correction = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_freq_correction"));
    if (rtlsdr_set_freq_correction == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_freq_correction";
        return 1;
    }

    rtlsdr_set_tuner_bandwidth = reinterpret_cast<int (*)(void *, uint32_t)>(driver.resolve("rtlsdr_set_tuner_bandwidth"));
    if (rtlsdr_set_tuner_bandwidth != nullptr)
        has_set_bw = true;
    else
        qCritical() << "This version of the RTL-SDR driver does not have set_tuner_bandwidth API";

    rtlsdr_get_tuner_type = reinterpret_cast<enum rtlsdr_tuner (*)(void *)>(driver.resolve("rtlsdr_get_tuner_type"));
    if (rtlsdr_get_tuner_type == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_tuner_type";
        return 1;
    }

    rtlsdr_set_agc_mode = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_agc_mode"));
    if (rtlsdr_set_agc_mode == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_agc_mode";
        return 1;
    }

    rtlsdr_set_tuner_gain = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_tuner_gain"));
    if (rtlsdr_set_tuner_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_tuner_gain";
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_tuner_gain_mode = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_tuner_gain_mode"));
    if (rtlsdr_set_tuner_gain_mode == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_tuner_gain_mode";
        return 1;
    }

    rtlsdr_get_tuner_gains = reinterpret_cast<int (*)(void *, int *)>(driver.resolve("rtlsdr_get_tuner_gains"));
    if (rtlsdr_get_tuner_gains == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_tuner_gains";
        return 1;
    }

    rtlsdr_get_tuner_gain = reinterpret_cast<int (*)(void *)>(driver.resolve("rtlsdr_get_tuner_gain"));
    if (rtlsdr_get_tuner_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_tuner_gain";
        return 1;
    }

    rtlsdr_set_direct_sampling = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_direct_sampling"));
    if (rtlsdr_set_direct_sampling == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_direct_sampling";
        return 1;
    }

    rtlsdr_get_direct_sampling = reinterpret_cast<int (*)(void *)>(driver.resolve("rtlsdr_get_direct_sampling"));
    if (rtlsdr_get_direct_sampling == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_get_direct_sampling";
        return 1;
    }

    rtlsdr_set_bias_tee = reinterpret_cast<int (*)(void *, int)>(driver.resolve("rtlsdr_set_bias_tee"));
    if (rtlsdr_set_direct_sampling == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_set_bias_tee";
        return 1;
    }

    rtlsdr_cancel_async = reinterpret_cast<int (*)(void *)>(driver.resolve("rtlsdr_cancel_async"));
    if (rtlsdr_cancel_async == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_cancel_async";
        return 1;
    }

    rtlsdr_reset_buffer = reinterpret_cast<int (*)(void *)>(driver.resolve("rtlsdr_reset_buffer"));
    if (rtlsdr_reset_buffer == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_reset_buffer";
        return 1;
    }

    rtlsdr_read_async = reinterpret_cast<int (*)(void *, rtlsdr_read_async_cb_t, void *, uint32_t, uint32_t)>(driver.resolve("rtlsdr_read_async"));
    if (rtlsdr_read_async == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "rtlsdr_read_async";
        return 1;
    }

    return 0;
}
