/*
 * Bladerf backend
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
#include <chrono>
#include <new>      // std::nothrow
#include <thread>

#include <QDebug>

#include "common/ring_buffer_cplx.h"
#include "sdr_device_bladerf.h"
#include "sdr_device_bladerf_api.h"


#define DEFAULT_RX_FREQ     100e6
#define DEFAULT_RX_RATE     960e3
#define DEFAULT_RX_BW       0
#define DEFAULT_RX_GAIN     0
#define DEFAULT_USB_RESET   false

#define CFG_KEY_RX_GAIN     "bladerf/rx_gain"
#define CFG_KEY_USB_RESET   "bladerf/usb_reset_on_open"

SdrDevice *sdr_device_create_bladerf()
{
    return new SdrDeviceBladerf();
}

SdrDeviceBladerf::SdrDeviceBladerf(QObject *parent) :
    SdrDevice(parent),
    driver("bladeRF", this),
    device(nullptr),
    reader_thread(nullptr),
    keep_running(false)
{
    clearStatus(status);
    clearStats(stats);

    // initialize settings
    settings.rx_frequency = DEFAULT_RX_FREQ;
    settings.rx_sample_rate = DEFAULT_RX_RATE;
    settings.rx_bandwidth = DEFAULT_RX_BW;
    settings.rx_gain = DEFAULT_RX_GAIN;
    settings.usb_reset_on_open = DEFAULT_USB_RESET;

    reader_buffer = ring_buffer_cplx_create();
    ring_buffer_cplx_init(reader_buffer, quint32(0.5 * settings.rx_sample_rate));

    // connect rx_ctl signals to slots
    rx_ctl.setEnabled(false);
    connect(&rx_ctl, SIGNAL(gainChanged(int)), this, SLOT(setRxGain(int)));
    connect(&rx_ctl, SIGNAL(biasChanged(bladerf_channel, bool)),
            this, SLOT(setBias(bladerf_channel, bool)));
}

SdrDeviceBladerf::~SdrDeviceBladerf()
{
    if (status.rx_is_running)
        stopRx();

    if (status.device_is_open)
        close();

    if (status.driver_is_loaded)
        driver.unload();

    ring_buffer_cplx_delete(reader_buffer);
}

int SdrDeviceBladerf::open()
{
    int     result;

    if (status.rx_is_running || status.device_is_open)
        return SDR_DEVICE_EBUSY;

    if (!status.driver_is_loaded)
    {
        if (loadDriver())
            return SDR_DEVICE_ELIB;
        status.driver_is_loaded = true;
    }

    qDebug() << "Opening BladeRF device";
    bladerf_set_usb_reset_on_open(settings.usb_reset_on_open);
    result = bladerf_open(&device, nullptr);
    if (result)
    {
        qCritical() << "Error opening BladeRF device:" << bladerf_strerror(result);
        return SDR_DEVICE_EOPEN;
    }

//    readDeviceInfo();
//    printDeviceInfo();

    status.device_is_open = true;
    applySettings();

    rx_ctl.setEnabled(true);

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::close(void)
{
    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    setBias(BLADERF_CHANNEL_RX(0), false);
    setBias(BLADERF_CHANNEL_RX(1), false);

    qDebug() << "Closing BladeRF device";
    bladerf_close(device);

    status.device_is_open = false;
    rx_ctl.setEnabled(false);

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::readSettings(const QSettings &s)
{
    bool    conv_ok;
    int     int_val;

    int_val = s.value(CFG_KEY_RX_GAIN, DEFAULT_RX_GAIN).toInt(&conv_ok);
    if (conv_ok)
        settings.rx_gain = int_val;

    settings.usb_reset_on_open = s.value(CFG_KEY_USB_RESET, DEFAULT_USB_RESET).toBool();

    if (status.device_is_open)
        applySettings();

    rx_ctl.readSettings(settings);

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::saveSettings(QSettings &s)
{
    if (settings.rx_gain == DEFAULT_RX_GAIN)
        s.remove(CFG_KEY_RX_GAIN);
    else
        s.setValue(CFG_KEY_RX_GAIN, settings.rx_gain);

    if (settings.usb_reset_on_open == DEFAULT_USB_RESET)
        s.remove(CFG_KEY_USB_RESET);
    else
        s.setValue(CFG_KEY_USB_RESET, settings.usb_reset_on_open);

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::startRx(void)
{
    int result;

    if (!status.device_is_open)
        return SDR_DEVICE_ERROR;

    if (status.rx_is_running)
        return SDR_DEVICE_OK;

    qDebug() << "Starting BladeRF receiver";
    ring_buffer_cplx_clear(reader_buffer);

    result = bladerf_sync_config(device, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11,
                                 16, 16384, 8, 3500);
    if (result)
    {
        qCritical() << "Failed to configure synchronous transfer:" << bladerf_strerror(result);
        return SDR_DEVICE_ERROR;
    }

    result = bladerf_enable_module(device, BLADERF_CHANNEL_RX(0), true); // FIXME: Not CHANNEL_RX(0), see aPI docs
    if (result)
    {
        qCritical() << "Failed to enable receiver module:" << bladerf_strerror(result);
        return SDR_DEVICE_ERROR;
    }

    result = bladerf_set_gain_mode(device, BLADERF_CHANNEL_RX(0), BLADERF_GAIN_MGC);
    if (result)
        qWarning() << "Failed to enable manual gain mode:" << bladerf_strerror(result);

    qDebug() << "Starting BladeRF reader thread";
    if (!reader_thread)
    {
        keep_running = true;
        reader_thread = new(std::nothrow) std::thread(&SdrDeviceBladerf::readerThread, this);
        if (reader_thread == nullptr)
        {
            qCritical() << "Failed to create BladeRF reader thread";
            keep_running = false;
            goto startRxError;
        }
    }

    status.rx_is_running = true;

    return SDR_DEVICE_OK;

startRxError:
    close();
    return SDR_DEVICE_ERROR;
}

int SdrDeviceBladerf::stopRx(void)
{
    int result;

    if (!status.rx_is_running)
        return SDR_DEVICE_OK;

    if (reader_thread)
    {
        qDebug() << "Stopping BladeRF reader thread";
        keep_running = false;
        reader_thread->join();
        delete reader_thread;
        reader_thread = nullptr;
    }

    qDebug() << "Stopping BladeRF";
    result = bladerf_enable_module(device, BLADERF_CHANNEL_RX(0), false);
    if (result)
        qCritical() << "Failed to disable receiver module:" << bladerf_strerror(result);

    status.rx_is_running = false;

    return SDR_DEVICE_OK;
}

void SdrDeviceBladerf::readerThread(void)
{
    int             result;
    unsigned int    i;
    unsigned int    num_samples = settings.rx_sample_rate / 20;
    int16_t        *buffer = new(std::nothrow)int16_t[2 * num_samples];
    float          *rbuf   = new(std::nothrow)float[2 * num_samples];

    if (buffer == nullptr)
    {
        qCritical() << "Could not allocate buffer in reader thread";
        return;
    }

    qDebug() << "BladeRF reader thread started";
    while (keep_running)
    {
        // read data
        result = bladerf_sync_rx(device, buffer, num_samples, nullptr, 5000);
        if (result)
        {
            qCritical() << "Error reading from BladeRF:" << bladerf_strerror(result);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

#define SAMPLE_SCALE (1.0f / 2048.0f);

        for (i = 0; i < num_samples; i++)
        {
            rbuf[2*i] = float(buffer[2*i]) * SAMPLE_SCALE;
            rbuf[2*i+1] = float(buffer[2*i+1]) * SAMPLE_SCALE;
        }
        reader_lock.lock();
        ring_buffer_cplx_write(reader_buffer, (complex_t *)rbuf, num_samples);
        reader_lock.unlock();
        // FIXME: check overflow

    }
    qDebug() << "BladeRF reader thread stopped";

    delete [] buffer;
    delete [] rbuf;
}

quint32 SdrDeviceBladerf::getRxSamples(complex_t * buffer, quint32 count)
{
    std::lock_guard<std::mutex> lock(reader_lock);

    if (!buffer || count == 0)
        return 0;

    if (count > ring_buffer_cplx_count(reader_buffer))
        return 0;

    ring_buffer_cplx_read(reader_buffer, buffer, count);

    return count;
}

QWidget *SdrDeviceBladerf::getRxControls(void)
{
    return  &rx_ctl;
}

int SdrDeviceBladerf::setRxFrequency(quint64 freq)
{
    int result;

    if (freq < 47e6 || freq > 6e9)
        return SDR_DEVICE_ERANGE;

    settings.rx_frequency = freq;
    if (!status.device_is_open)
        return SDR_DEVICE_OK;

    result = bladerf_set_frequency(device, BLADERF_CHANNEL_RX(0), freq);
    if (result)
    {
        qCritical() << "Failed to set RX frequency rate to" << freq
                    << " Reason:" << bladerf_strerror(result);
        return SDR_DEVICE_ERROR;
    }

    qDebug() << " ### SdrDeviceBladerf::setRxFrequency" << freq;

/* FIXME: gain range is frequency dependent
    {
        const struct bladerf_range *gain_range = new(std::nothrow)(struct bladerf_range);
        result = bladerf_get_gain_range(device, BLADERF_CHANNEL_RX(0), &gain_range);
        if (result)
            qWarning() << "Failed to get gain range:" << bladerf_strerror(result);
        else
            qInfo() << "Gain range:" << gain_range->min << gain_range->max << gain_range->step << gain_range->scale;

    }
*/

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::setRxSampleRate(quint32 rate)
{
    bladerf_sample_rate actual_rate;
    int                 result;

    if (rate > 61440000)
        return SDR_DEVICE_ERANGE;

    settings.rx_sample_rate = rate;
    if (!status.device_is_open)
        return SDR_DEVICE_OK;

    updateRxBufferSize();
    result = bladerf_set_sample_rate(device, BLADERF_CHANNEL_RX(0),
                                     settings.rx_sample_rate, &actual_rate);
    if (result)
    {
        qCritical() << "Failed to set RX sample rate to" << rate
                    << " Reason:" << bladerf_strerror(result);
        return SDR_DEVICE_ERROR;
    }
    if (settings.rx_sample_rate != actual_rate)
    {
        qWarning() << "Requested sample rate is" << settings.rx_sample_rate
                   << "- actual rate is" << actual_rate;
        settings.rx_sample_rate = actual_rate;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceBladerf::setRxBandwidth(quint32 bw)
{
    bladerf_bandwidth   actual_bw;
    int                 result;

    settings.rx_bandwidth = bw;
    if (!status.device_is_open)
        return SDR_DEVICE_OK;

    result = bladerf_set_bandwidth(device, BLADERF_CHANNEL_RX(0),
                                   settings.rx_bandwidth, &actual_bw);
    if (result)
    {
        qCritical() << "Failed to set RX bandwidth to" << bw
                    << " Reason:" << bladerf_strerror(result);
        return SDR_DEVICE_ERROR;
    }
    if (settings.rx_bandwidth != actual_bw)
    {
        qWarning() << "Requested bandwidth is" << settings.rx_bandwidth
                   << "- actual rate is" << actual_bw;
        settings.rx_bandwidth = actual_bw;
    }

    return SDR_DEVICE_OK;
}

void SdrDeviceBladerf::setRxGain(int gain)
{
    int result;

    settings.rx_gain = gain;
    if (!status.device_is_open)
        return;

    result = bladerf_set_gain(device, BLADERF_CHANNEL_RX(0), gain);
    if (result)
        qWarning() << "Failed to set RX gain to" << gain
                    << " Reason:" << bladerf_strerror(result);
}

void SdrDeviceBladerf::setBias(bladerf_channel ch, bool enable)
{
    int result;

    result = bladerf_set_bias_tee(device, ch, enable);
    if (result)
        qWarning() << "Failed to set channel" << ch << "bias tee to" << (enable ? "ON" : "OFF");
}

void SdrDeviceBladerf::updateRxBufferSize(void)
{
    std::lock_guard<std::mutex> lock(reader_lock);
    quint32 new_size = quint32(0.5f * settings.rx_sample_rate); // 500 msec

    if (new_size == 0 || new_size == ring_buffer_cplx_size(reader_buffer))
        return;

    ring_buffer_cplx_clear(reader_buffer);
    ring_buffer_cplx_resize(reader_buffer, new_size);
}

void SdrDeviceBladerf::applySettings()
{
    setRxFrequency(settings.rx_frequency);
    setRxSampleRate(settings.rx_sample_rate);
    setRxBandwidth(settings.rx_bandwidth);
    setRxGain(settings.rx_gain);
}

void SdrDeviceBladerf::readDeviceInfo(void)
{
    int     result;

    if (!status.device_is_open)
        return;

    device_info.board_name = bladerf_get_board_name(device);
    device_info.dev_speed = bladerf_device_speed(device);
    result = bladerf_get_serial(device, device_info.serial);
    if (result)
        qCritical() << "Failed to read BladeRF serial number:"
                    << bladerf_strerror(result);

    result = bladerf_fw_version(device, &device_info.fw_version);
    if (result)
        qCritical() << "Failed to read BladeRF firmware version:"
                    << bladerf_strerror(result);

    result = bladerf_is_fpga_configured(device);
    if (result < 0)
    {
        qCritical() << "Failed to read BladeRF FPGA status:"
                    << bladerf_strerror(result);
    }
    else
    {
        qInfo("BladeRF FPGA is %s configured", result ? "" : "not");
    }

    result = bladerf_get_fpga_size(device, &device_info.fpga_size);
    if (result)
        qCritical() << "Failed to read BladeRF FPGA size:"
                    << bladerf_strerror(result);

    result = bladerf_fpga_version(device, &device_info.fpga_version);
    if (result)
        qCritical() << "Failed to read BladeRF FPGA version:"
                    << bladerf_strerror(result);
}

void SdrDeviceBladerf::printDeviceInfo(void)
{
    qInfo("%s S/N %s\n"
          "    Firmware ver: %d.%d.%d\n"
          "        FPGA ver: %d.%d.%d\n"
          "       FPGA size: %d\n"
          "     Super speed: %s",
          device_info.board_name, device_info.serial,
          device_info.fw_version.major, device_info.fw_version.minor, device_info.fw_version.patch,
          device_info.fpga_version.major, device_info.fpga_version.minor, device_info.fpga_version.patch,
          device_info.fpga_size,
          device_info.dev_speed == BLADERF_DEVICE_SPEED_SUPER ? "Yes" : "No"
          );
}

#define SYMBOL_EMSG "Error loading symbol address for"
int SdrDeviceBladerf::loadDriver(void)
{
    struct bladerf_version  version;


    if (!driver.isLoaded())
    {
        qInfo() << "Loading BladeRF driver library";
        if (!driver.load())
        {
            qCritical() << "Error loading BladeRF driver library";
            return 1;
        }
    }

    bladerf_version = reinterpret_cast<void (*)(struct bladerf_version *)>(driver.resolve("bladerf_version"));
    if (bladerf_version == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_version";
        return 1;
    }

    bladerf_version(&version);
    qInfo("BladeRF driver is version %d.%d.%d; backend uses API version %X",
          version.major, version.minor, version.patch, LIBBLADERF_API_VERSION);
    // TODO: Check versions are okay

    bladerf_open = reinterpret_cast<int (*)(struct bladerf **, const char *)>(driver.resolve("bladerf_open"));
    if (bladerf_open == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_open";
        return 1;
    }

    bladerf_close = reinterpret_cast<void (*)(struct bladerf *)>(driver.resolve("bladerf_close"));
    if (bladerf_close == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_close";
        return 1;
    }

    bladerf_set_usb_reset_on_open = reinterpret_cast<void (*)(bool)>(driver.resolve("bladerf_set_usb_reset_on_open"));
    if (bladerf_set_usb_reset_on_open == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_usb_reset_on_open";
        return 1;
    }

    bladerf_get_serial = reinterpret_cast<int (*)(struct bladerf *, char *)>(driver.resolve("bladerf_get_serial"));
    if (bladerf_get_serial == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_get_serial";
        return 1;
    }

    bladerf_get_fpga_size = reinterpret_cast<int (*)(struct bladerf *, bladerf_fpga_size *)>(driver.resolve("bladerf_get_fpga_size"));
    if (bladerf_get_fpga_size == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_get_fpga_size";
        return 1;
    }

    bladerf_fw_version = reinterpret_cast<int (*)(struct bladerf *, struct bladerf_version *)>(driver.resolve("bladerf_fw_version"));
    if (bladerf_fw_version == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_fw_version";
        return 1;
    }

    bladerf_is_fpga_configured = reinterpret_cast<int (*)(struct bladerf *)>(driver.resolve("bladerf_is_fpga_configured"));
    if (bladerf_is_fpga_configured == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_is_fpga_configured";
        return 1;
    }

    bladerf_fpga_version = reinterpret_cast<int (*)(struct bladerf *, struct bladerf_version *)>(driver.resolve("bladerf_fpga_version"));
    if (bladerf_fpga_version == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_fpga_version";
        return 1;
    }

    bladerf_device_speed = reinterpret_cast<bladerf_dev_speed (*)(struct bladerf *)>(driver.resolve("bladerf_device_speed"));
    if (bladerf_device_speed == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_device_speed";
        return 1;
    }

    bladerf_get_board_name = reinterpret_cast<const char *(*)(struct bladerf *)>(driver.resolve("bladerf_get_board_name"));
    if (bladerf_get_board_name == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_get_board_name";
        return 1;
    }

    bladerf_set_frequency = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bladerf_frequency)>(driver.resolve("bladerf_set_frequency"));
    if (bladerf_set_frequency == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_frequency";
        return 1;
    }

    bladerf_set_sample_rate = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bladerf_sample_rate, bladerf_sample_rate *)>(driver.resolve("bladerf_set_sample_rate"));
    if (bladerf_set_sample_rate == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_sample_rate";
        return 1;
    }

    bladerf_set_bandwidth = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bladerf_bandwidth, bladerf_bandwidth *)>(driver.resolve("bladerf_set_bandwidth"));
    if (bladerf_set_bandwidth == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_bandwidth";
        return 1;
    }

    bladerf_set_gain = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bladerf_gain)>(driver.resolve("bladerf_set_gain"));
    if (bladerf_set_gain == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_gain";
        return 1;
    }

    bladerf_set_gain_mode = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bladerf_gain_mode)>(driver.resolve("bladerf_set_gain_mode"));
    if (bladerf_set_gain_mode == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_gain_mode";
        return 1;
    }

    bladerf_get_gain_range = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, const struct bladerf_range **)>(driver.resolve("bladerf_get_gain_range"));
    if (bladerf_get_gain_range == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_get_gain_range";
        return 1;
    }


    bladerf_enable_module = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel, bool)>(driver.resolve("bladerf_enable_module"));
    if (bladerf_enable_module == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_enable_module";
        return 1;
    }

    bladerf_sync_config = reinterpret_cast<int(*)(struct bladerf *, bladerf_channel_layout, bladerf_format, unsigned int, unsigned int, unsigned int, unsigned int)>(driver.resolve("bladerf_sync_config"));
    if (bladerf_sync_config == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_sync_config";
        return 1;
    }

    bladerf_sync_rx = reinterpret_cast<int(*)(struct bladerf *, void *, unsigned int, struct bladerf_metadata *, unsigned int)>(driver.resolve("bladerf_sync_rx"));
    if (bladerf_sync_rx == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_sync_rx";
        return 1;
    }


    bladerf_log_set_verbosity = reinterpret_cast<void (*)(bladerf_log_level)>(driver.resolve("bladerf_log_set_verbosity"));
    if (bladerf_log_set_verbosity == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_log_set_verbosity";
        return 1;
    }

    bladerf_strerror = reinterpret_cast<const char * (*)(int)>(driver.resolve("bladerf_strerror"));
    if (bladerf_strerror == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_strerror";
        return 1;
    }

    bladerf_set_bias_tee = reinterpret_cast<int (*)(struct bladerf *, bladerf_channel, bool)>(driver.resolve("bladerf_set_bias_tee"));
    if (bladerf_set_bias_tee == nullptr)
    {
        qCritical() << SYMBOL_EMSG << "bladerf_set_bias_tee";
        return 1;
    }


    return 0;
}
