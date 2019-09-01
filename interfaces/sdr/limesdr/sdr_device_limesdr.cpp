/*
 * LimeSDR backend
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2019 Alexandru Csete OZ9AEC
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
#include <QMessageBox>
#include <QString>

#include "nanosdr/common/ring_buffer_cplx.h"
#include "sdr_device_limesdr.h"
#include "sdr_device_limesdr_api.h"

#define DEFAULT_RX_GAIN     50
#define DEFAULT_LPF_ON      true
#define DEFAULT_GFIR_ON     false

#define CFG_KEY_RX_GAIN     "limesdr/rx_gain"
#define CFG_KEY_LPF_ON      "limesdr/lpf_on"
#define CFG_KEY_GFIR_ON     "limesdr/gfir_on"


SdrDevice *sdr_device_create_limesdr()
{
    return new SdrDeviceLimesdr();
}

SdrDeviceLimesdr::SdrDeviceLimesdr(QObject *parent) : SdrDevice(parent),
    driver("LimeSuite", this),
    device(nullptr),
    reader_thread(nullptr),
    keep_running(false)
{
    clearStatus(status);
    clearStats(stats);

    settings.rx_frequency = 100e6;
    settings.rx_sample_rate = 3840000;
    settings.rx_bandwidth = 0;
    settings.rx_gain = DEFAULT_RX_GAIN;
    settings.rx_channel = 0;
    settings.rx_lpf = DEFAULT_LPF_ON;
    settings.rx_gfir = DEFAULT_GFIR_ON;

    reader_buffer = ring_buffer_cplx_create();
    ring_buffer_cplx_init(reader_buffer, settings.rx_sample_rate / 2);

    rx_ctl.setEnabled(false);
    connect(&rx_ctl, SIGNAL(gainChanged(unsigned int)), this, SLOT(setRxGain(unsigned int)));
    connect(&rx_ctl, SIGNAL(lpfToggled(bool)), this, SLOT(enableRxLpf(bool)));
    connect(&rx_ctl, SIGNAL(gfirToggled(bool)), this, SLOT(enableRxGfir(bool)));
    connect(&rx_ctl, SIGNAL(calibrate(void)), this, SLOT(calibrateRx(void)));
}

SdrDeviceLimesdr::~SdrDeviceLimesdr()
{
    if (status.rx_is_running)
        stopRx();

    if (status.device_is_open)
        close();

    if (status.driver_is_loaded)
        driver.unload();

    ring_buffer_delete(reader_buffer);
}

int SdrDeviceLimesdr::open()
{
    if (status.rx_is_running || status.device_is_open)
        return SDR_DEVICE_EBUSY;

    if (!status.driver_is_loaded)
    {
        if (loadDriver())
            return SDR_DEVICE_ELIB;
        status.driver_is_loaded = true;
    }

    qDebug() << "Opening LimeSDR device";
    if (LMS_Open(&device, nullptr, nullptr) != LMS_SUCCESS)
    {
        qCritical() << "Failed to open LimeSDR device";
        return SDR_DEVICE_EOPEN;
    }

    qDebug() << "Configuring LMS chip for operation";
    if (LMS_Init(device) != LMS_SUCCESS)
    {
        qCritical() << "Failed to initialize LimeSDR device";
        LMS_Close(device);
        return SDR_DEVICE_EINIT;
    }

    status.device_is_open = true;
    readDeviceLimits();
    rx_ctl.setEnabled(true);
    applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::close(void)
{
    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    if (LMS_DestroyStream(device, &rx_stream) != LMS_SUCCESS)
        qCritical() << "Failed to destroy RX stream";

    qDebug() << "Closing LimeSDR device";
    if (LMS_Close(device) != LMS_SUCCESS)
        qCritical() << "Failed to close LimeSDR device";

    status.device_is_open = false;
    rx_ctl.setEnabled(false);

    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::readSettings(const QSettings &s)
{
    bool    conv_ok;
    unsigned int    int_val;

    int_val = s.value(CFG_KEY_RX_GAIN, DEFAULT_RX_GAIN).toUInt(&conv_ok);
    if (conv_ok)
        settings.rx_gain = int_val;

    settings.rx_lpf = s.value(CFG_KEY_LPF_ON, DEFAULT_LPF_ON).toBool();
    settings.rx_gfir = s.value(CFG_KEY_GFIR_ON, DEFAULT_GFIR_ON).toBool();

    if (status.device_is_open)
        applySettings();

    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::saveSettings(QSettings &s)
{
    if (settings.rx_gain == DEFAULT_RX_GAIN)
        s.remove(CFG_KEY_RX_GAIN);
    else
        s.setValue(CFG_KEY_RX_GAIN, settings.rx_gain);

    if (settings.rx_lpf == DEFAULT_LPF_ON)
        s.remove(CFG_KEY_LPF_ON);
    else
        s.setValue(CFG_KEY_LPF_ON, settings.rx_lpf);

    if (settings.rx_gfir == DEFAULT_GFIR_ON)
        s.remove(CFG_KEY_GFIR_ON);
    else
        s.setValue(CFG_KEY_GFIR_ON, settings.rx_gfir);

    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::startRx(void)
{
    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    if (status.rx_is_running)
        return SDR_DEVICE_OK;

    qDebug() << "Enabling RX channel 0";
    if (LMS_EnableChannel(device, LMS_CH_RX, settings.rx_channel, true) != LMS_SUCCESS)
    {
        qCritical() << "Failed to enable RX channel 0";
        goto startRxError;
    }

    rx_stream.channel = 0;
    rx_stream.fifoSize = 1024 * 1024;
    rx_stream.throughputVsLatency = 1.0;    // optimize for max throughput
    rx_stream.isTx = false;
    rx_stream.dataFmt = lms_stream_t::LMS_FMT_F32;
    if (LMS_SetupStream(device, &rx_stream) != LMS_SUCCESS)
    {
        qCritical() << "Failed to set up RX stream";
        goto startRxError;
    }

    setRxSampleRate(settings.rx_sample_rate);
    qDebug() << "Starting RX stream";
    if (LMS_StartStream(&rx_stream) != LMS_SUCCESS)
    {
        qCritical() << "Failed to start RX stream";
        goto startRxError;
    }

    LMS_SetGaindB(device, LMS_CH_RX, settings.rx_channel, 50);

    qDebug() << "Starting LimeSDR reader thread";
    if (!reader_thread)
    {
        keep_running = true;
        reader_thread = new(std::nothrow) std::thread(&SdrDeviceLimesdr::readerThread, this);
        if (reader_thread == nullptr)
        {
            qCritical() << "Failed to create LimeSDR reader thread";
            keep_running = false;
            goto startRxError;
        }
    }

    status.rx_is_running = true;
    setRxGain(settings.rx_gain);

    return SDR_DEVICE_OK;

startRxError:
    close();
    return SDR_DEVICE_ERROR;
}

int SdrDeviceLimesdr::stopRx(void)
{
    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    if (reader_thread)
    {
        qDebug() << "Stopping LimeSDR reader thread";
        keep_running = false;
        reader_thread->join();
        delete reader_thread;
        reader_thread = nullptr;
    }

    qDebug() << "Stopping RX stream";
    if (LMS_StopStream(&rx_stream) != LMS_SUCCESS)
        qInfo() << "Failed to stop RX stream";

    if (LMS_EnableChannel(device, LMS_CH_RX, settings.rx_channel, false) != LMS_SUCCESS)
        qInfo() << "Failed to disable RX channel" << settings.rx_channel;

    status.rx_is_running = false;

    return SDR_DEVICE_OK;
}

void SdrDeviceLimesdr::readerThread(void)
{
//    float *buffer[2*153600];

    int     read_size = settings.rx_sample_rate / 10;
    float  *buffer = new(std::nothrow)float[2*read_size];

    if (buffer == nullptr)
    {
        qCritical() << "Could not allocate buffer in reader thread";
        return;
    }

    qDebug() << "LimeSDR reader thread started";
    while (keep_running)
    {
        if (LMS_RecvStream(&rx_stream, buffer, read_size, nullptr, 300) != read_size)
        {
            qCritical() << "Error reading from RX stream";
            continue;
        }
        // FIXME: check overflow
        reader_lock.lock();
        ring_buffer_cplx_write(reader_buffer, (complex_t *)buffer, read_size);
        reader_lock.unlock();
    }
    qDebug() << "LimeSDR reader thread stopped";

    delete [] buffer;
}

quint32 SdrDeviceLimesdr::getRxSamples(complex_t * buffer, quint32 count)
{
    std::lock_guard<std::mutex> lock(reader_lock);

    if (!buffer || count == 0 || count > ring_buffer_cplx_count(reader_buffer))
        return 0;

    ring_buffer_cplx_read(reader_buffer, buffer, count);

    return count;
}

QWidget *SdrDeviceLimesdr::getRxControls(void)
{
    return &rx_ctl;
}

int SdrDeviceLimesdr::setRxFrequency(quint64 freq)
{
    settings.rx_frequency = freq;

    if (!status.device_is_open)
        return SDR_DEVICE_OK;

    if (LMS_SetLOFrequency(device, LMS_CH_RX, settings.rx_channel, freq) != LMS_SUCCESS)
    {
        qCritical() << "Failed to set RX frequency to" << freq;
        // FIXME: read actual frequency
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::setRxSampleRate(quint32 rate)
{
//    if (rate < info.rx_rate.min || rate > info.rx_rate.max)
//        return SDR_DEVICE_ERANGE;

    settings.rx_sample_rate = rate;

    if (!status.device_is_open)
        return SDR_DEVICE_OK;

    qDebug() << "Setting RX sample rate to" << rate;
    if (LMS_SetSampleRate(device, rate, 0) != LMS_SUCCESS) // FIXME: oversampling";
    {
        qCritical() << "Failed to set RX sample rate to" << rate;
        // FIXME: read actual sample rate
        return SDR_DEVICE_ERROR;
    }

    updateBufferSize();
    return SDR_DEVICE_OK;
}

int SdrDeviceLimesdr::setRxBandwidth(quint32 bw)
{
//    if (bw < info.rx_lpf.min || bw > info.rx_lpf.max)
//        return SDR_DEVICE_ERANGE;

    settings.rx_bandwidth = bw;

    if (!status.device_is_open || !settings.rx_lpf)
        return SDR_DEVICE_OK;

    qDebug() << "Setting RX bandwidth";
    if (LMS_SetLPFBW(device, LMS_CH_RX, 0, bw) != LMS_SUCCESS)
    {
        qCritical() << "Failed to set RX bandwidth to" << bw;
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

void SdrDeviceLimesdr::enableRxLpf(bool enabled)
{
    settings.rx_lpf = enabled;

    if (!status.device_is_open)
        return;

    if (LMS_SetLPF(device, LMS_CH_RX, 0, enabled) != LMS_SUCCESS)
    {
        qCritical() << "Failed to" << (enabled ? "enable" : "disable")
                    << "RX LPF";
    }
}

void SdrDeviceLimesdr::enableRxGfir(bool enabled)
{
    settings.rx_gfir = enabled;

    qDebug() << "FIXME: Ignoring GFIR setting";

/*
    if (!status.device_is_open)
        return;

    qDebug() << "SdrDeviceLimesdr::enableRxGfir";
    if (LMS_SetGFIRLPF(device, LMS_CH_RX, 0, enabled, settings.rx_bandwidth) != LMS_SUCCESS)
    {
        qCritical() << "Failed to" << (enabled ? "enable" : "disable")
                    << "RX GFIR";
    }

*/
}

void SdrDeviceLimesdr::setRxGain(unsigned int gain)
{
    settings.rx_gain = gain;

    if (!status.device_is_open)
        return;

    if (LMS_SetGaindB(device, LMS_CH_RX, 0, gain) != LMS_SUCCESS)
    {
        qCritical() << "Error setting RX gain to" << gain;
        LMS_GetGaindB(device, LMS_CH_RX, settings.rx_channel, &settings.rx_gain);
        //rx_ctl.setGain(settings.rx_gain);
    }
}

/* used to apply cached settings after device is opened */
void SdrDeviceLimesdr::applySettings(void)
{
    rx_ctl.readSettings(settings);
    setRxFrequency(settings.rx_frequency);
    setRxSampleRate(settings.rx_sample_rate);
    setRxBandwidth(settings.rx_bandwidth);
    setRxGain(settings.rx_gain);
    enableRxLpf(settings.rx_lpf);
    enableRxGfir(settings.rx_gfir);
}

void SdrDeviceLimesdr::calibrateRx(void)
{
    qDebug() << "Calibrating receiver...";
    if (LMS_Calibrate(device, LMS_CH_RX, settings.rx_channel,
                      settings.rx_bandwidth, 0) != LMS_SUCCESS)
    {
        qInfo() << "Failed to calibrate receiver path";
    }
    qDebug() << "Calibration done";
}

void SdrDeviceLimesdr::calibrateTx(void)
{
}

void SdrDeviceLimesdr::updateBufferSize(void)
{
    std::lock_guard<std::mutex> lock(reader_lock);
    quint32 new_size = settings.rx_sample_rate / 2; // 500 msec

    if (new_size == ring_buffer_cplx_size(reader_buffer))
        return;

    ring_buffer_cplx_clear(reader_buffer);
    ring_buffer_cplx_resize(reader_buffer, new_size);
}

void SdrDeviceLimesdr::readDeviceLimits(void)
{
    if (!status.device_is_open)
        return;

    info.rx_channels = LMS_GetNumChannels(device, LMS_CH_RX);
    if (info.rx_channels == -1)
        qCritical() << "Failed to read number of RX channels";

    info.tx_channels = LMS_GetNumChannels(device, LMS_CH_TX);
    if (info.tx_channels == -1)
        qCritical() << "Failed to read number of TX channels";

    if (LMS_GetLOFrequencyRange(device, LMS_CH_RX, &info.rx_lo) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read RX LO range";
        info.rx_lo.min = 10.e6;
        info.rx_lo.max = 3.5e6;
        info.rx_lo.step = 1.0;
    }

    if (LMS_GetLOFrequencyRange(device, LMS_CH_TX, &info.tx_lo) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read TX LO range";
        info.tx_lo.min = 10.e6;
        info.tx_lo.max = 3.5e6;
        info.tx_lo.step = 1.0;
    }

    if (LMS_GetLPFBWRange(device, LMS_CH_RX, &info.rx_lpf) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read RX LPF range";
        info.rx_lpf.min = 1.4001e6;
        info.rx_lpf.max = 130.e6;
        info.rx_lpf.step = 1.0;
    }

    if (LMS_GetLPFBWRange(device, LMS_CH_TX, &info.tx_lpf) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read TX LPF range";
        info.tx_lpf.min = 5.e6;
        info.tx_lpf.max = 130.e6;
        info.tx_lpf.step = 1.0;
    }

    if (LMS_GetSampleRateRange(device, LMS_CH_RX, &info.rx_rate) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read RX sample rate range";
        info.rx_rate.min = 240000;
        info.rx_rate.max = 61440000;
        info.rx_rate.step = 1.0;
    }

    if (LMS_GetSampleRateRange(device, LMS_CH_TX, &info.tx_rate) != LMS_SUCCESS)
    {
        qCritical() << "Failed to read TX sample rate range";
        info.tx_rate.min = 240000;
        info.tx_rate.max = 61440000;
        info.tx_rate.step = 1.0;
    }

    printDeviceLimits();
}

void SdrDeviceLimesdr::printDeviceLimits(void)
{
    qInfo("LimeSDR device info:\n"
          "    RX channels: %d\n"
          "    RX LO range: %.0f MHz - %.0f MHz\n"
          "    RX LP range: %.3f MHz - %.3f MHz\n"
          "    TX channels: %d\n"
          "    TX LO range: %.0f MHz - %.0f MHz\n"
          "    TX LP range: %.3f MHz - %.3f MHz\n"
          "\n"
          "Sample rates:\n"
          "    RX: %.3f - %.3f kHz\n"
          "    TX: %.3f - %.3f kHz\n",
          info.rx_channels, 1.e-6 * info.rx_lo.min, 1.e-6 * info.rx_lo.max,
          1.e-6 * info.rx_lpf.min, 1.e-6 * info.rx_lpf.max,
          info.tx_channels, 1.e-6 * info.tx_lo.min, 1.e-6 * info.tx_lo.max,
          1.e-6 * info.tx_lpf.min, 1.e-6 * info.tx_lpf.max,
          1.e-3 * info.rx_rate.min, 1.e-3 * info.rx_rate.max,
          1.e-3 * info.tx_rate.min, 1.e-3 * info.tx_rate.max);
}

#define SYMBOL_EMSG "Error loading symbol address for"
int SdrDeviceLimesdr::loadDriver(void)
{
    if (!driver.isLoaded())
    {
        qDebug() << "Loading LimeSDR driver library";
        if (!driver.load())
        {
            qCritical() << "Error loading LimeSDR driver library";
            return 1;
        }
    }

    LMS_GetLibraryVersion = reinterpret_cast<const char* (*)(void)>(driver.resolve("LMS_GetLibraryVersion"));
    if (LMS_GetLibraryVersion == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetDeviceList";
        return 1;
    }
    qInfo() << "LimeSDR driver library version is" << LMS_GetLibraryVersion();

    qDebug() << "Loading symbols from LimeSDR library";

    LMS_GetDeviceList = reinterpret_cast<int (*)(lms_info_str_t *)>(driver.resolve("LMS_GetDeviceList"));
    if (LMS_GetDeviceList == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetDeviceList";
        return 1;
    }

    LMS_Open = reinterpret_cast<int (*)(lms_device_t **, const lms_info_str_t, void *)>(driver.resolve("LMS_Open"));
    if (LMS_Open == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_Open";
        return 1;
    }

    LMS_Close = reinterpret_cast<int (*)(lms_device_t *)>(driver.resolve("LMS_Close"));
    if (LMS_Close == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_Close";
        return 1;
    }

    LMS_Init = reinterpret_cast<int (*)(lms_device_t *)>(driver.resolve("LMS_Init"));
    if (LMS_Init == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_Init";
        return 1;
    }

    LMS_EnableChannel = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, bool)>(driver.resolve("LMS_EnableChannel"));
    if (LMS_EnableChannel == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_EnableChannel";
        return 1;
    }

    LMS_SetLOFrequency = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, float_type)>(driver.resolve("LMS_SetLOFrequency"));
    if (LMS_SetLOFrequency == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetLOFrequency";
        return 1;
    }

    LMS_SetNCOFrequency = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, const float_type *, float_type )>(driver.resolve("LMS_SetNCOFrequency"));
    if (LMS_SetNCOFrequency == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetNCOFrequency";
        return 1;
    }

    LMS_SetGaindB = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, unsigned)>(driver.resolve("LMS_SetGaindB"));
    if (LMS_SetGaindB == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetGaindB";
        return 1;
    }

    LMS_GetGaindB = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, unsigned *)>(driver.resolve("LMS_GetGaindB"));
    if (LMS_GetGaindB == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetGaindB";
        return 1;
    }

    LMS_SetSampleRate = reinterpret_cast<int (*)(lms_device_t *, float_type, size_t)>(driver.resolve("LMS_SetSampleRate"));
    if (LMS_SetSampleRate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetSampleRate";
        return 1;
    }

    LMS_SetLPF = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, bool)>(driver.resolve("LMS_SetLPF"));
    if (LMS_SetLPF == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetLPF";
        return 1;
    }

    LMS_SetLPFBW = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, float_type)>(driver.resolve("LMS_SetLPFBW"));
    if (LMS_SetLPFBW == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetLPFBW";
        return 1;
    }

    LMS_SetGFIRLPF = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, bool, float_type)>(driver.resolve("LMS_SetGFIRLPF"));
    if (LMS_SetGFIRLPF == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetGFIRLPF";
        return 1;
    }

    LMS_SetupStream = reinterpret_cast<int (*)(lms_device_t *, lms_stream_t *)>(driver.resolve("LMS_SetupStream"));
    if (LMS_SetupStream == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_SetupStream";
        return 1;
    }

    LMS_DestroyStream = reinterpret_cast<int (*)(lms_device_t *, lms_stream_t *)>(driver.resolve("LMS_DestroyStream"));
    if (LMS_DestroyStream == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_DestroyStream";
        return 1;
    }

    LMS_StartStream = reinterpret_cast<int (*)(lms_stream_t *)>(driver.resolve("LMS_StartStream"));
    if (LMS_StartStream == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_StartStream";
        return 1;
    }

    LMS_StopStream = reinterpret_cast<int (*)(lms_stream_t *)>(driver.resolve("LMS_StopStream"));
    if (LMS_StopStream == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_StopStream";
        return 1;
    }

    LMS_RecvStream = reinterpret_cast<int (*)(lms_stream_t *, void *, size_t, lms_stream_meta_t *, unsigned)>(driver.resolve("LMS_RecvStream"));
    if (LMS_RecvStream == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_RecvStream";
        return 1;
    }

    LMS_Calibrate = reinterpret_cast<int (*)(lms_device_t *, bool, size_t, double, unsigned)>(driver.resolve("LMS_Calibrate"));
    if (LMS_Calibrate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_Calibrate";
        return 1;
    }

    LMS_GetNumChannels = reinterpret_cast<int (*)(lms_device_t *, bool)>(driver.resolve("LMS_GetNumChannels"));
    if (LMS_GetNumChannels == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetNumChannels";
        return 1;
    }

    LMS_GetLOFrequencyRange = reinterpret_cast<int (*)(lms_device_t *, bool, lms_range_t *)>(driver.resolve("LMS_GetLOFrequencyRange"));
    if (LMS_GetLOFrequencyRange == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetLOFrequencyRange";
        return 1;
    }

    LMS_GetLPFBWRange = reinterpret_cast<int (*)(lms_device_t *, bool, lms_range_t *)>(driver.resolve("LMS_GetLPFBWRange"));
    if (LMS_GetLPFBWRange == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetLPFBWRange";
        return 1;
    }

    LMS_GetSampleRateRange = reinterpret_cast<int (*)(lms_device_t *, bool, lms_range_t *)>(driver.resolve("LMS_GetSampleRateRange"));
    if (LMS_GetSampleRateRange == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "LMS_GetSampleRateRange";
        return 1;
    }

    return 0;
}
