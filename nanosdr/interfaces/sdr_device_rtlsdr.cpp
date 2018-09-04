/*
 * RTL-SDR backend
 *
 * Copyright  2014-2018  Alexandru Csete OZ9AEC
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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/datatypes.h"
#include "common/library_loader.h"
#include "common/sdr_data.h"
#include "common/time.h"
#include "sdr_device.h"
#include "sdr_device_rtlsdr_reader.h"


// rtlsdr API
enum rtlsdr_tuner {
	RTLSDR_TUNER_UNKNOWN = 0,
	RTLSDR_TUNER_E4000,
	RTLSDR_TUNER_FC0012,
	RTLSDR_TUNER_FC0013,
	RTLSDR_TUNER_FC2580,
	RTLSDR_TUNER_R820T,
	RTLSDR_TUNER_R828D
};

typedef void (*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

static int (*rtlsdr_open)(void ** dev, uint32_t index);
static int (*rtlsdr_close)(void * dev);
static int (*rtlsdr_set_sample_rate)(void * dev, uint32_t rate);
static uint32_t (*rtlsdr_get_sample_rate)(void * dev);
static int (*rtlsdr_set_center_freq)(void * dev, uint32_t freq);
static uint32_t (*rtlsdr_get_center_freq)(void * dev);
static int (*rtlsdr_set_freq_correction)(void * dev, int ppm);
static enum rtlsdr_tuner (*rtlsdr_get_tuner_type)(void * dev);
static int (*rtlsdr_set_agc_mode)(void * dev, int on);
static int (*rtlsdr_set_tuner_gain)(void * dev, int gain);
static int (*rtlsdr_set_tuner_gain_mode)(void * dev, int manual);
static int (*rtlsdr_get_tuner_gains)(void * dev, int *gains);
static int (*rtlsdr_set_tuner_bandwidth)(void * dev, uint32_t bw);
static int (*rtlsdr_set_direct_sampling)(void * dev, int on);
static int (*rtlsdr_get_direct_sampling)(void * dev);
static int (*rtlsdr_cancel_async)(void * dev);
static int (*rtlsdr_reset_buffer)(void * dev);
static int (*rtlsdr_read_async)(void * dev, rtlsdr_read_async_cb_t cb,
				                void *ctx, uint32_t buf_num, uint32_t buf_len);

class SdrDeviceRtlsdr : public SdrDevice
{
public:
    SdrDeviceRtlsdr(void);
    virtual     ~SdrDeviceRtlsdr();

    // Virtual function implementations
    int         init(float samprate, const char * options);
    int         set_sample_rate(float new_rate);
    int         get_sample_rates(float * rates) const;
    float       get_sample_rate(void) const;
    float       get_dynamic_range(void) const { return 50.f; }
    int         set_freq(uint64_t freq);
    uint64_t    get_freq(void) const;
    int         get_freq_range(freq_range_t * range) const;
    int         set_freq_corr(float ppm);
    int         set_gain(int value);
    int         start(void);
    int         stop(void);
    uint32_t    get_num_bytes(void) const;
    uint32_t    get_num_samples(void) const;
    uint32_t    read_bytes(void * buffer, uint32_t bytes);
    uint32_t    read_samples(complex_t * buffer, uint32_t samples);
    int         type(void) const { return SDR_DEVICE_RTLSDR; };

private:
    void        free_memory(void);
    int         load_librtlsdr(void);

private:
    lib_handle_t       lib;
    rtlsdr_reader_t   *reader;
    void           *dev;
    int            *gains;
    int             num_gains;
    bool            is_loaded;
    bool            is_initialized;
};

SdrDevice *sdr_device_create_rtlsdr()
{
    return new SdrDeviceRtlsdr();
}

SdrDeviceRtlsdr::SdrDeviceRtlsdr(void)
{
    reader = 0;
    is_loaded = false;
    is_initialized = false;
    num_gains = 0;
}

SdrDeviceRtlsdr::~SdrDeviceRtlsdr()
{
    int     ret;

    if (!is_initialized)
        return;

    ret = rtlsdr_close(dev);
    if (ret)
        fprintf(stderr, "ERROR: rtlsdr_close() returned %d\n", ret);

    free_memory();

    if (is_loaded)
        close_library(lib);
}

int SdrDeviceRtlsdr::load_librtlsdr()
{
    if (is_loaded)
        return SDR_DEVICE_OK;

    fputs("Loading RTLSDR library... ", stderr);
    lib = load_library("rtlsdr");
    if (lib == NULL)
    {
        fputs("Error loading library\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    fputs("OK (unknown version)\nLoading symbols... ", stderr);

    rtlsdr_open = (int (*)(void **, uint32_t)) get_symbol(lib, "rtlsdr_open");
    if (rtlsdr_open == NULL)
    {
        fputs("Error loading symbol address for \n", stderr); // FIXME: add error func to base class
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_close = (int (*)(void *)) get_symbol(lib, "rtlsdr_close");
    if (rtlsdr_close == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_close\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_sample_rate = (int (*)(void *, uint32_t)) get_symbol(lib, "rtlsdr_set_sample_rate");
    if (rtlsdr_set_sample_rate == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_sample_rate\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_get_sample_rate = (uint32_t (*)(void *)) get_symbol(lib, "rtlsdr_get_sample_rate");
    if (rtlsdr_get_sample_rate == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_get_sample_rate\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_center_freq = (int (*)(void *, uint32_t)) get_symbol(lib, "rtlsdr_set_center_freq");
    if (rtlsdr_set_center_freq == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_center_freq\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_get_center_freq = (uint32_t (*)(void *)) get_symbol(lib, "rtlsdr_get_center_freq");
    if (rtlsdr_get_center_freq == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_get_center_freq\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_freq_correction = (int (*)(void *, int)) get_symbol(lib, "rtlsdr_set_freq_correction");
    if (rtlsdr_set_freq_correction == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_freq_correction\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_get_tuner_type = (enum rtlsdr_tuner (*)(void *)) get_symbol(lib, "rtlsdr_get_tuner_type");
    if (rtlsdr_get_tuner_type == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_get_tuner_type\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_agc_mode = (int (*)(void *, int)) get_symbol(lib, "rtlsdr_set_agc_mode");
    if (rtlsdr_set_agc_mode == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_agc_mode\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_tuner_gain = (int (*)(void *, int)) get_symbol(lib, "rtlsdr_set_tuner_gain");
    if (rtlsdr_set_tuner_gain == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_tuner_gain\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_tuner_gain_mode = (int (*)(void *, int)) get_symbol(lib, "rtlsdr_set_tuner_gain_mode");
    if (rtlsdr_set_tuner_gain_mode == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_tuner_gain_mode\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_get_tuner_gains = (int (*)(void *, int *)) get_symbol(lib, "rtlsdr_get_tuner_gains");
    if (rtlsdr_get_tuner_gains == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_get_tuner_gains\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_tuner_bandwidth = (int (*)(void *, uint32_t)) get_symbol(lib, "rtlsdr_set_tuner_bandwidth");
    if (rtlsdr_set_tuner_bandwidth == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_tuner_bandwidth\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_set_direct_sampling = (int (*)(void *, int)) get_symbol(lib, "rtlsdr_set_direct_sampling");
    if (rtlsdr_set_direct_sampling == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_set_direct_sampling\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_get_direct_sampling = (int (*)(void *)) get_symbol(lib, "rtlsdr_get_direct_sampling");
    if (rtlsdr_get_direct_sampling == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_get_direct_sampling\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_cancel_async = (int (*)(void *)) get_symbol(lib, "rtlsdr_cancel_async");
    if (rtlsdr_cancel_async == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_cancel_async\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_reset_buffer = (int (*)(void *)) get_symbol(lib, "rtlsdr_reset_buffer");
    if (rtlsdr_reset_buffer == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_reset_buffer\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    rtlsdr_read_async = (int (*)(void *, rtlsdr_read_async_cb_t, void *,
                         uint32_t, uint32_t)) get_symbol(lib, "rtlsdr_read_async");
    if (rtlsdr_read_async == NULL)
    {
        fputs("Error loading symbol address for rtlsdr_read_async\n", stderr);
        return SDR_DEVICE_ELIB;
    }

    is_loaded = true;
    fputs("OK\n", stderr);

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::init(float samprate, const char * options)
{
    (void)  options;
    int     ret;

    if (is_initialized)
        return SDR_DEVICE_EBUSY;

    free_memory();

    ret = load_librtlsdr();
    if (ret != SDR_DEVICE_OK)
        return ret;

    ret = rtlsdr_open(&dev, 0);     // FIXME: device index
    if (ret)
    {
        fprintf (stderr,"ERROR: rtlsdr_open() returned %d\n", ret);       
        return SDR_DEVICE_ERROR;
    }

    fprintf(stderr, "  Tuner type: %d\n", rtlsdr_get_tuner_type(dev));

    if (rtlsdr_set_center_freq(dev, 435000000))
        fputs("Error setting rtlsdr center frequency\n", stderr);

    if (set_sample_rate(samprate))
        fprintf(stderr, "Error setting rtlsdr sample rate to %.2f\n", samprate);

    // FIXME: Both AGCs enabled might give more gain than manual?
    // manual gain modes
    if (rtlsdr_set_agc_mode(dev, 0))
        fputs("Error disabling RTL2832 AGC.\n", stderr);
    if (rtlsdr_set_tuner_gain_mode(dev, 1))
        fputs("Error setting manual gain mode for rtlsdr tuner.\n", stderr);

    // get available gain values
    num_gains = rtlsdr_get_tuner_gains(dev, NULL);
    if (num_gains > 0)
    {
        gains = (int *) malloc(num_gains * sizeof(int));
        ret = rtlsdr_get_tuner_gains(dev, gains);
        if (ret != num_gains)
            fprintf(stderr, "Number of gains don't match %d vs. %d\n", ret,
                    num_gains);
        // set gain to max
        if (rtlsdr_set_tuner_gain(dev, gains[num_gains-1]))
            fputs("Error setting tuner gain.\n", stderr);
    }

    // set auto bandwidth
    if (rtlsdr_set_tuner_bandwidth(dev, 0))
        fputs("Error setting auto-bandwidth for rtlsdr tuner.\n", stderr);

    // FIXME: E4000
    // int rtlsdr_set_tuner_if_gain(rtlsdr_dev_t *dev, int stage, int gain);

    // create rtlsdr reader helper
    reader = rtlsdr_reader_create(dev, lib);
    if (reader == NULL)
    {
        rtlsdr_close(dev);
        return SDR_DEVICE_ERROR;
    }

    is_initialized = true;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::set_sample_rate(float new_rate)
{
    if (rtlsdr_set_sample_rate(dev, new_rate))
        return SDR_DEVICE_EINVAL;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::get_sample_rates(float * rates) const
{
    if (rates == 0)
        return 12;

    // we only report ratiometric rates.
    rates[0] = 240e3f;
    rates[1] = 300e3f;
    rates[2] = 960e3f;
    rates[3] = 1152e3f;
    rates[4] = 1200e3f;
    rates[5] = 1440e3f;
    rates[6] = 1600e3f;
    rates[7] = 1800e3f;
    rates[8] = 1920e3f;
    rates[9] = 2400e3f;
    rates[10] = 2880e3f;
    rates[11] = 3200e3f;

    return 12;
}

float SdrDeviceRtlsdr::get_sample_rate(void) const
{
    return rtlsdr_get_sample_rate(dev);
}

int SdrDeviceRtlsdr::set_freq(uint64_t freq)
{
    int     result;

    if (rtlsdr_set_center_freq(dev, freq))
    {
        fprintf(stderr, "SdrDeviceRtlsdr::set_freq(%" PRIu64 ") failed\n", freq);
        return SDR_DEVICE_ERANGE;
    }

    sdr_device_debug("SdrDeviceRtlsdr::set_freq(%" PRIu64 ")\n", freq);
    if (freq < 24e6 && !rtlsdr_get_direct_sampling(dev))
    {
        if ((result = rtlsdr_set_direct_sampling(dev, 2)))
            fprintf(stderr, "Note: rtlsdr_set_direct_sampling returned %d\n",
                    result);
    }
    else if (freq >= 24e6 && rtlsdr_get_direct_sampling(dev))
    {
        if ((result = rtlsdr_set_direct_sampling(dev, 0)))
            fprintf(stderr, "Note: rtlsdr_set_direct_sampling returned %d\n",
            result);
    }

    return SDR_DEVICE_OK;
}

uint64_t SdrDeviceRtlsdr::get_freq(void) const
{
    return rtlsdr_get_center_freq(dev);
}

int SdrDeviceRtlsdr::get_freq_range(freq_range_t * range) const
{
    range->step = 1;

    fputs("*** FIXME: SdrDeviceRtlsdr::get_freq_range ignores direct sampling\n",
          stderr);

    switch (rtlsdr_get_tuner_type(dev))
    {
    case RTLSDR_TUNER_E4000:
        range->min = 52e6;
        range->max = 2200e6;
        break;

    case RTLSDR_TUNER_FC0012:
        range->min = 22e6;
        range->max = 948e6;
        break;

	case RTLSDR_TUNER_FC0013:
        range->min = 22e6;
        range->max = 1100e6;
        break;

	case RTLSDR_TUNER_FC2580:
        range->min = 146e6;
        range->max = 924e6;
        break;

	case RTLSDR_TUNER_R820T:
	case RTLSDR_TUNER_R828D:
        range->min = 24e6;
        range->max = 1800e6;
        break;

    case RTLSDR_TUNER_UNKNOWN:
    default:
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::set_freq_corr(float ppm)
{
    return rtlsdr_set_freq_correction(dev, ppm) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::set_gain(int value)
{
    int     idx;

    if (value < 0 || value > 100)
        return SDR_DEVICE_ERANGE;

    idx = (value * (num_gains - 1)) / 100;
    if (rtlsdr_set_tuner_gain(dev, gains[idx]))
        return SDR_DEVICE_ERROR;

    return SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::start(void)
{
    return rtlsdr_reader_start(reader) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

int SdrDeviceRtlsdr::stop(void)
{
    return rtlsdr_reader_stop(reader) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

uint32_t SdrDeviceRtlsdr::get_num_bytes(void) const
{
    return rtlsdr_reader_get_num_bytes(reader);
}

uint32_t SdrDeviceRtlsdr::get_num_samples(void) const
{
    // internal buffer holds u8 I/Q samples
    return rtlsdr_reader_get_num_bytes(reader) / 2;
}

uint32_t SdrDeviceRtlsdr::read_bytes(void * buffer, uint32_t bytes)
{
    return rtlsdr_reader_read_bytes(reader, buffer, bytes);
}

uint32_t SdrDeviceRtlsdr::read_samples(complex_t * buffer, uint32_t samples)
{
    uint8_t     buf[480000];
    real_t     *workbuf = (real_t *) buffer;
    int         bytes_read;
    int         i;

    if (samples > 480000)
        return 0;

    bytes_read = rtlsdr_reader_read_bytes(reader, buf, 2 * samples);

    for (i = 0; i < bytes_read; i++)
        workbuf[i] = ((real_t)buf[i] - 127.4f) / 127.5f; // FIXME: LUT

    return bytes_read / 2;
}

void SdrDeviceRtlsdr::free_memory(void)
{
    if (num_gains > 0)
    {
        free(gains);
        num_gains = 0;
    }

    if (reader)
        rtlsdr_reader_destroy(reader);
}

