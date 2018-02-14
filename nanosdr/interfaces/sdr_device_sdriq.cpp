/*
 * RFSpace SDR-IQ backend
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
#include "common/sdr_data.h"
#include "common/time.h"
#include "sdr_device.h"
#include "sdriq/sdriq.h"


class SdrDeviceSdriq : public SdrDevice
{
public:
    SdrDeviceSdriq(void);
    virtual     ~SdrDeviceSdriq();

    // Virtual function implementations
    int         init(float samprate, const char * options);
    int         set_sample_rate(float new_rate);
    float       get_sample_rate(void) const;
    int         get_sample_rates(float * rates) const;
    float       get_dynamic_range(void) const
    {
        // 84 dB (14 bit ADC) + 36 dB potential processing gain from AD6620
        return 120.f;
    }

    int         set_freq(uint64_t freq);
    uint64_t    get_freq(void) const;
    int         get_freq_range(freq_range_t * range) const;
    int         set_freq_corr(float ppm);
    int         get_gain_stages(uint8_t * gains) const;
    uint16_t    get_gain_stages_bf(void) const;
    int         set_gain(uint8_t stage, uint8_t value);
    int         start(void);
    int         stop(void);
    uint32_t    get_num_bytes(void) const;
    uint32_t    get_num_samples(void) const;
    uint32_t    read_bytes(void * buffer, uint32_t bytes);
    uint32_t    read_samples(complex_t * buffer, uint32_t samples);
    int         type(void) const { return SDR_DEVICE_SDRIQ; };

private:
    void        free_memory(void);

private:
    sdriq_t    *sdr;            // handle to SDR-IQ driver object
    int16_t    *buf;            // internal buffer used for s16->fc conversion
    uint32_t    buflen;         // internal buffer length in samples

    uint32_t    sample_rate;
    uint32_t    current_freq;   // SDR-IQ max freq is 33 MHz (don't need 64 bit)
    bool        initialized;
};

SdrDevice * sdr_device_create_sdriq()
{
    return new SdrDeviceSdriq();
}

SdrDeviceSdriq::SdrDeviceSdriq(void)
{
    sdr = 0;
    buf = 0;
    buflen = 0;
    sample_rate = 0;
    current_freq = 0;
    initialized = false;
}

SdrDeviceSdriq::~SdrDeviceSdriq()
{
    if (!initialized)
        return;

    free_memory();

    sdriq_stop(sdr);
    sdriq_close(sdr);
    sdriq_free(sdr);
}

int SdrDeviceSdriq::init(float samprate, const char * options)
{
    (void)  options;
    int     err;

    if (initialized)
        return SDR_DEVICE_EBUSY;

    sdr = sdriq_new();
    if (sdr == NULL)
        return SDR_DEVICE_ERROR;

    err = sdriq_open(sdr);
    if (err)
    {
        fprintf(stderr, "Error opening SDR-IQ device (%d)\n", err);
        sdriq_free(sdr);
        return SDR_DEVICE_EOPEN;
    }

    err = set_sample_rate(samprate);
    if (err)
    {
        fprintf(stderr, "Failed to set SDR_IQ sample rate (%d)\n", err);
        return SDR_DEVICE_ESAMPRATE;
    }

    if (set_freq(10000000))
    {
        fputs("Failed to set SDR_IQ frequency\n", stderr);
        return SDR_DEVICE_ERROR;
    }

    if (sdriq_set_fixed_rf_gain(sdr, 0))
    {
        fputs("Failed to set SDR-IQ RF gain\n", stderr);
        return SDR_DEVICE_ERROR;
    }

    if (sdriq_set_fixed_if_gain(sdr, 24))
    {
        fputs("Failed to set SDR-IQ IF gain\n", stderr);
        return SDR_DEVICE_ERROR;
    }

    initialized = true;
    return SDR_DEVICE_OK;
}

int SdrDeviceSdriq::set_sample_rate(float new_rate)
{
    sample_rate = (uint32_t) new_rate;

    // FIXME: can't set sample rate while SDR-IQ is running
    // FIXME: check error code
    int ret = sdriq_set_sample_rate(sdr, sample_rate);
    if (ret)
    {
        fprintf(stderr, "*** Failed to set sample rate to %.4f (%d)\n",
                new_rate, ret);
        fprintf(stderr, "*** Actual rate: %" PRIu32 "\n",
                sdriq_get_sample_rate(sdr));

        if (ret == -1)
            return SDR_DEVICE_EINVAL;

        return SDR_DEVICE_ERROR;
    }

    // resize buffer to hold 100 ms worth of samples
    if (buf)
        delete[] buf;

    buflen = ceilf(0.1f * new_rate);
    buf = new int16_t[2 * buflen];

    return SDR_DEVICE_OK;
}

float SdrDeviceSdriq::get_sample_rate(void) const 
{
    return sdriq_get_sample_rate(sdr);
}

int SdrDeviceSdriq::get_sample_rates(float * rates) const
{
    if (rates == 0)
        return 7;

    rates[0] = 8138.f;
    rates[1] = 16276.f;
    rates[2] = 37793.f;
    rates[3] = 55556.f;
    rates[4] = 111111.f;
    rates[5] = 158730.f;
    rates[6] = 196078.f;

    return 7;
}

int SdrDeviceSdriq::set_freq(uint64_t freq)
{
    current_freq = freq;

    int ret = sdriq_set_freq(sdr, current_freq);
    if (ret)
    {
        fprintf(stderr, "sdriq_set_freq(%" PRIu32 ") failed:%d\n",
                current_freq, ret);
        return SDR_DEVICE_ERANGE;
    }

    sdr_device_debug("SdrDeviceSdriq::set_freq(%" PRIu32 ")\n", current_freq);
    return SDR_DEVICE_OK;
}

uint64_t SdrDeviceSdriq::get_freq(void) const
{
    return current_freq;
}

int SdrDeviceSdriq::get_freq_range(freq_range_t * range) const
{
    range->min = 0;
    range->max = 33333333;
    range->step = 1;
    return SDR_DEVICE_OK;
}

int SdrDeviceSdriq::set_freq_corr(float ppm)
{
    // Fc = F + F * PPM / 1e6
    float   rate = 66666667.f + 66666667.e-6f * ppm;

    if (sdriq_set_input_rate(sdr, (uint32_t) rate))
    {
        fputs("*** Failed to set SDR-IQ input rate\n", stderr);
        return SDR_DEVICE_ERROR;
    }

    return SDR_DEVICE_OK;
}

int SdrDeviceSdriq::get_gain_stages(uint8_t * gains) const
{
    if (gains == 0)
        return 2;

    gains[0] = SDR_DEVICE_RX_LNA_GAIN;
    gains[1] = SDR_DEVICE_RX_IF_GAIN;
    return 2;
}

uint16_t SdrDeviceSdriq::get_gain_stages_bf(void) const
{
    return (uint16_t)
        (1 << SDR_DEVICE_RX_LNA_GAIN) |
        (1 << SDR_DEVICE_RX_IF_GAIN);
}

int SdrDeviceSdriq::set_gain(uint8_t stage, uint8_t value)
{
    int         ret = SDR_DEVICE_OK;
    int8_t      gain = 0;

    if (value > 100)
        return SDR_DEVICE_ERANGE;

    if (stage == SDR_DEVICE_RX_LNA_GAIN)
    {
        // [0; 100] -> { -30, -20, -10, 0}
        gain = 10 * (value / 30) - 30;
        ret = sdriq_set_fixed_rf_gain(sdr, gain) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
    }
    else if (stage == SDR_DEVICE_RX_IF_GAIN)
    {
        // [0; 100] -> { 0, 6, 12, 18, 24}
        gain = 6 * (value / 23);
        ret = sdriq_set_fixed_if_gain(sdr, gain) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
    }
    else
        return SDR_DEVICE_EINVAL;

    sdr_device_debug("SdrDeviceSdriq::set_gain(stage:%u,val:%u)  g:%d  res:%d\n",
                     stage, value, gain, ret);

    return ret;
}

int SdrDeviceSdriq::start(void)
{
    return sdriq_start(sdr) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

int SdrDeviceSdriq::stop(void)
{
    return sdriq_stop(sdr) ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

uint32_t SdrDeviceSdriq::get_num_bytes(void) const
{
    return 4 * sdriq_get_num_samples(sdr);
}

uint32_t SdrDeviceSdriq::get_num_samples(void) const
{
    return sdriq_get_num_samples(sdr);
}

uint32_t SdrDeviceSdriq::read_bytes(void * buffer, uint32_t bytes)
{
    return sdriq_get_samples(sdr, (unsigned char *) buffer, bytes / 4) * 4;
}

#define SAMPLE_SCALE (1.0f / 32768.f)

uint32_t SdrDeviceSdriq::read_samples(complex_t * buffer, uint32_t samples)
{
    real_t     *workbuf = (real_t *) buffer;
    uint32_t    samples_read;
    uint32_t    i;

    if (samples > buflen)
        return 0;

    samples_read = sdriq_get_samples(sdr, (unsigned char *) buf, samples);
    for (i = 0; i < 2 * samples_read; i++)
        workbuf[i] = ((real_t)buf[i] + 0.7f) * SAMPLE_SCALE;

    return samples_read;
}

void SdrDeviceSdriq::free_memory(void)
{
    delete[] buf;
    buf = 0;
    buflen = 0;
}

