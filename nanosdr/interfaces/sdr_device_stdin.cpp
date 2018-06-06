/*
 * stdin backend (for testing only)
 *
 * Copyright  2016-2018  Alexandru Csete OZ9AEC
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
#include <string.h>
#include <unistd.h>

#include "common/datatypes.h"
#include "common/sdr_data.h"
#include "common/time.h"
#include "sdr_device.h"


/*
 * Input reader for stdin. Free running, i.e. there is no throttling to match
 * the sample rate.
 *
 * TODO:
 *  - Currently only complex S16LE format is supported.
 *  - Add open/close methods?
 */
class SdrDeviceStdin : public SdrDevice
{
public:
    SdrDeviceStdin(void);
    virtual     ~SdrDeviceStdin();

    /* optarg is filename or stdint */
    int         init(float samprate, const char * options);
    int         set_sample_rate(float new_rate);
    int         get_sample_rates(float * rates) const { return 0; }
    float       get_sample_rate(void) const {return 0.f; }
    float       get_dynamic_range(void) const { return 120.f; }
    int         set_freq(uint64_t freq);
    uint64_t    get_freq(void) const { return current_freq; }
    int         get_freq_range(freq_range_t * range) const;
    int         set_freq_corr(float ppm);
    int         set_gain(int value) { return SDR_DEVICE_EINVAL; }
    int         start(void);
    int         stop(void);
    uint32_t    get_num_bytes(void) const { return wk_buflen * 2; }
    uint32_t    get_num_samples(void) const { return wk_buflen; }
    uint32_t    read_bytes(void * buffer, uint32_t bytes);
    uint32_t    read_samples(complex_t * buffer, uint32_t samples);
    int         type(void) const { return SDR_DEVICE_STDIN; };

private:
    uint64_t        current_freq;

    // internal working buffer used for S16->fc convresion
    int16_t        *wk_buf;
    uint32_t        wk_buflen;

    // statistics
    uint64_t        bytes_read;     // bytes read between start() and stop()

    void            free_memory(void);
};


SdrDevice * sdr_device_create_stdin()
{
    return new SdrDeviceStdin();
}

SdrDeviceStdin::SdrDeviceStdin()
{
    wk_buf = 0;
    wk_buflen = 0;
}

SdrDeviceStdin::~SdrDeviceStdin()
{
    free_memory();
}

int SdrDeviceStdin::init(float samprate, const char * options)
{
    fprintf(stderr, "\n**********************************************\n");
    fprintf(stderr, "  SdrDeviceStdin::init\n");
    fprintf(stderr, "  %s\n", options);
    fprintf(stderr, "  FOR TESTING PURPOSES ONLY\n");
    fprintf(stderr, "**********************************************\n\n");

    // re-initialise dynamic parameters
    current_freq = 0;
    return set_sample_rate(samprate);
}

int SdrDeviceStdin::set_sample_rate(float new_rate)
{
    free_memory();

    wk_buflen = 2 * new_rate;
    wk_buf = new int16_t[wk_buflen];

    return SDR_DEVICE_OK;
}

int SdrDeviceStdin::set_freq(uint64_t freq)
{
    current_freq = freq;
    return SDR_DEVICE_OK;
}

int SdrDeviceStdin::get_freq_range(freq_range_t * range) const
{
    range->min = 0;
    range->max = 100e9;
    range->step = 1;
    return SDR_DEVICE_OK;
}

int SdrDeviceStdin::set_freq_corr(float ppm)
{
    (void) ppm;
    return SDR_DEVICE_OK;
}

int SdrDeviceStdin::start(void)
{
    fputs("Starting input reader: stdin\n", stderr);
    bytes_read = 0;

    return SDR_DEVICE_OK;
}

int SdrDeviceStdin::stop(void)
{
    fprintf(stderr, "Stopping input reader\n");
    fprintf(stderr, "   Bytes read: %" PRIu64 "\n", bytes_read);

    return SDR_DEVICE_OK;
}

uint32_t SdrDeviceStdin::read_bytes(void * buffer, uint32_t bytes)
{
    if (bytes > 2 * wk_buflen)
        return 0;

    size_t      bytes_in = fread(buffer, 1, bytes, stdin);
    if (bytes_in < bytes)
    {
        return 0;
    }

    bytes_read += bytes_in;
    return bytes_in;
}

#define SAMPLE_SCALE (1.0f / 32768.f)
uint32_t SdrDeviceStdin::read_samples(complex_t * buffer, uint32_t samples)
{
    if (samples > wk_buflen)
        return 0;

    real_t     *out_buf = (real_t *) buffer;
    size_t      samples_in = fread(wk_buf, 4, samples, stdin);

    if (samples_in < samples)
        return 0;

    for (uint32_t i = 0; i < 2 * samples; i++)
        out_buf[i] = ((real_t)wk_buf[i] + 0.5f) * SAMPLE_SCALE;

    bytes_read += 4 * samples;
    return samples;
}

void SdrDeviceStdin::free_memory(void)
{
    delete[] wk_buf;
    wk_buflen = 0;
}
