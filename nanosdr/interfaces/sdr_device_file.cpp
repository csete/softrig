/*
 * I/Q file backend (for testing only)
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
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/datatypes.h"
#include "common/ring_buffer.h"
#include "common/sdr_data.h"
#include "common/time.h"
#include "nanodsp/translate.h"
#include "sdr_ctl.h"
#include "sdr_device.h"

#define     FRAME_LENGTH_SEC            0.1f
#define     FRAME_LENGTH_MSEC           100
#define     MAX_FRAMES_IN_BUFFER        5       // frames stored in buffer


/*
 * Input reader from file with frequency translator.
 *
 * The frequency translation is done to simulate tuning and is mostly for
 * testing purposes.
 *
 * TODO:
 *  - Currently only complex S16LE format is supported.
 *  - Add open/close methods?
 */
class SdrDeviceFile : public SdrDevice
{
public:
    SdrDeviceFile(void);
    virtual     ~SdrDeviceFile();

    /* optarg is filename or stdint */
    int         init(float samprate, const char * options);
    int         set_sample_rate(float new_rate);
    int         get_sample_rates(float * rates) const { return 0; }
    float       get_sample_rate(void) const;
    float       get_dynamic_range(void) const { return 120.f; }
    int         set_freq(uint64_t freq);
    uint64_t    get_freq(void) const;
    int         get_freq_range(freq_range_t * range) const;
    int         set_freq_corr(float ppm);
    int         get_gain_stages(uint8_t * gains) const { return 0; }
    uint16_t    get_gain_stages_bf(void) const {return 0; }
    int         set_gain(uint8_t stage, uint8_t value) { return SDR_DEVICE_EINVAL; }
    int         start(void);
    int         stop(void);
    uint32_t    get_num_bytes(void) const;
    uint32_t    get_num_samples(void) const;
    uint32_t    read_bytes(void * buffer, uint32_t bytes);
    uint32_t    read_samples(complex_t * buffer, uint32_t samples);
    int         type(void) const { return SDR_DEVICE_FILE; };

private:
    Translate       ft;
    bool            is_running;
    float           sample_rate;
    uint32_t        bytes_per_frame;    // Bytes read in each cycle
    real_t          tuning_offset;
    uint64_t        initial_freq;       // Initial "RF" frequency
    uint64_t        tlast_ms;           // Last time we read data in msec
    pthread_t       reader_thread_id;

    char           *file_name;
    FILE           *fp;
    uint8_t        *input_buffer;
    ring_buffer_t  *rb;

    // internal working buffer used for S16->fc convresion
    int16_t        *wk_buf;
    uint32_t        wk_buflen;

    // statistics
    uint64_t        bytes_read;     // bytes read between start() and stop()
    uint32_t        overflows;      // number of buffer overflows

    static void    *reader_thread(void * data);
    void            free_memory(void);
    int             process_ctl_get_range(sdr_ctl_t * ctl);
};

SdrDevice * sdr_device_create_file()
{
    return new SdrDeviceFile();
}


/** Input reader thread. */
void *SdrDeviceFile::reader_thread(void * data)
{
    SdrDeviceFile   *reader = (SdrDeviceFile *)data;
    uint64_t        tnow_ms;
    size_t          bytes_in;

    while (reader->is_running)
    {
        tnow_ms = time_ms();
        if (tnow_ms - reader->tlast_ms < FRAME_LENGTH_MSEC)
        {
            usleep(10000);
            continue;
        }

        reader->tlast_ms = tnow_ms;

        bytes_in = fread(reader->input_buffer, 1, reader->bytes_per_frame,
                         reader->fp);

        if (bytes_in == reader->bytes_per_frame)
        {
            if (ring_buffer_is_full(reader->rb))
                reader->overflows++;

            ring_buffer_write(reader->rb, reader->input_buffer, bytes_in);
            reader->bytes_read += bytes_in;
        }
        else if (feof(reader->fp))
        {
            ring_buffer_clear(reader->rb);
            fprintf(stderr, "Input reached EOF => Rewind.\n");
            rewind(reader->fp);
            if (ftell(reader->fp) == -1)
            {
                // probably not a seekable file (e.g. stdin)
                fprintf(stderr, "Input file not seekable.\n");
                reader->is_running = false;
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error reading input.\n");
        }

    }

    fprintf(stderr, "Exiting input reader thread.\n");
    pthread_exit(NULL);
}


SdrDeviceFile::SdrDeviceFile(void)
{
    initial_freq = 0;
    tuning_offset = 0.f;
}

SdrDeviceFile::~SdrDeviceFile()
{
    free_memory();
}

int SdrDeviceFile::init(float samprate, const char * options)
{
    if (is_running)
        return SDR_DEVICE_EBUSY;

    fprintf(stderr, "\n**********************************************\n");
    fprintf(stderr, "  SdrDeviceFile::init\n");
    fprintf(stderr, "  %s\n", options);
    fprintf(stderr, "  FOR TESTING PURPOSES ONLY\n");
    fprintf(stderr, "**********************************************\n\n");

    // re-initialise dynamic parameters
    free_memory();

    initial_freq = 0;
    tlast_ms = 0;

    file_name = strdup(options);
    // check that file exists
    if (strcasecmp(file_name, "stdin") && access(options, F_OK))
        return SDR_DEVICE_ENOTFOUND;

    // sample rate must be initialized before ringbuf because of bytes_per_frame
    if (set_sample_rate(samprate))
        return SDR_DEVICE_EINVAL;

    input_buffer = new uint8_t[bytes_per_frame];
    rb = (ring_buffer_t *) malloc(sizeof(ring_buffer_t));
    ring_buffer_init(rb, MAX_FRAMES_IN_BUFFER * bytes_per_frame);

    return SDR_DEVICE_OK;
}

int SdrDeviceFile::set_sample_rate(float new_rate)
{
    if (new_rate <= 0.f)
        return SDR_DEVICE_EINVAL;

    sample_rate = new_rate;
    bytes_per_frame = (uint32_t)(sample_rate * FRAME_LENGTH_SEC) * 4 + 4;
    fprintf(stderr, "SdrDeviceFile: bytes_per_frame = %d\n", bytes_per_frame);

    ft.set_sample_rate(sample_rate);

    // resize buffer to hold 100 ms worth of samples
    if (wk_buf)
        delete[] wk_buf;

    wk_buflen = MAX_FRAMES_IN_BUFFER * bytes_per_frame / 4;
    wk_buf = new int16_t[2 * wk_buflen];

    return SDR_DEVICE_OK;
}

float SdrDeviceFile::get_sample_rate(void) const
{
    return sample_rate;
}

int SdrDeviceFile::set_freq(uint64_t freq)
{
    if (initial_freq != 0)
    {
        tuning_offset = (real_t)initial_freq - (real_t)freq;
        ft.set_nco_frequency(tuning_offset);
    }
    else
    {
        initial_freq = freq;
    }

    return SDR_DEVICE_OK;
}

uint64_t SdrDeviceFile::get_freq(void) const
{
    return initial_freq - tuning_offset;
}

int SdrDeviceFile::get_freq_range(freq_range_t * range) const
{
    range->min = 0;
    range->max = 100e9;
    range->step = 1;
    return SDR_DEVICE_OK;
}

int SdrDeviceFile::set_freq_corr(float ppm)
{
    // TODO
    // Fc = F + F * PPM / 1e6
    fprintf(stderr, " *** set_freq_corr() not implemented\n");

    return SDR_DEVICE_OK;
}

int SdrDeviceFile::start(void)
{
    int     ret;

    fprintf(stderr, "Starting input reader: %s\n", file_name);

    bytes_read = 0;
    overflows = 0;

    if (strcasecmp(file_name, "stdin") == 0)
    {
        fp = stdin;
    }
    else
    {
        fp = fopen(file_name, "rb");
        if (fp == 0)
            return SDR_DEVICE_ERROR;
    }

    // create reader thread
    is_running = true;
    ret = pthread_create(&reader_thread_id, NULL, &SdrDeviceFile::reader_thread, this);
    if (ret)
    {
        fprintf(stderr, "Error creating input reader thread: %d\n", ret);
        is_running = false;
        fclose(fp);
    }

    return is_running ? SDR_DEVICE_OK : SDR_DEVICE_ERROR;
}

int SdrDeviceFile::stop(void)
{
    int     ret;

    fprintf(stderr, "Stopping input reader\n");
    fprintf(stderr, "   Bytes read: %" PRIu64 "\n", bytes_read);
    fprintf(stderr, "    Overflows: %" PRIu32 "\n", overflows);

    // stop reader thread
    is_running = false;
    if ((ret = pthread_join(reader_thread_id, NULL)))
        fprintf(stderr, "Error stopping input reader thread: %d\n", ret);

    fclose(fp);

    return ret ? SDR_DEVICE_ERROR : SDR_DEVICE_OK;
}

uint32_t SdrDeviceFile::get_num_bytes(void) const
{
    return ring_buffer_count(rb);
}

uint32_t SdrDeviceFile::get_num_samples(void) const
{
    //fprintf(stderr, "Samples: %u\n", ring_buffer_count(rb) / 4);
    return ring_buffer_count(rb) / 4;
}

uint32_t SdrDeviceFile::read_bytes(void * buffer, uint32_t bytes)
{
    if (bytes > ring_buffer_count(rb))
        return 0;

    ring_buffer_read(rb, (unsigned char *) buffer, bytes);

    return bytes;
}

#define SAMPLE_SCALE (1.0f / 32768.f)
uint32_t SdrDeviceFile::read_samples(complex_t * buffer, uint32_t samples)
{
    real_t     *out_buf = (real_t *) buffer;
    uint32_t    i;

    if (samples > wk_buflen || ring_buffer_count(rb) < samples * 4)
    {
        fprintf(stderr, "/// No smaples? \\\\\\\n");
        return 0;
    }

    ring_buffer_read(rb, (unsigned char *) wk_buf, samples * 4);
    for (i = 0; i < 2 * samples; i++)
        out_buf[i] = ((real_t)wk_buf[i] + 0.5f) * SAMPLE_SCALE;

    ft.process(samples, buffer);

    return samples;
}

void SdrDeviceFile::free_memory(void)
{
    free(file_name);
    ring_buffer_delete(rb);
    delete[] input_buffer;
    delete[] wk_buf;
    wk_buflen = 0;
}

