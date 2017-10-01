/*
 * FFT thread class
 *
 * Copyright 2015  Alexandru Csete OZ9AEC
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
#include <unistd.h>

#include "common/datatypes.h"
#include "common/time.h"
#include "fft_thread.h"


FftThread::FftThread()
{
    reset_stats();
    running = false;

    fft_out = 0;
    have_fft_out = false;

    fputs("FFT thread created\n", stderr);
}

FftThread::~FftThread()
{
    stop();
    if (fft_out)
        delete[] fft_out;
    fputs("FFT thread destroyed\n", stderr);
}

int FftThread::init(uint32_t fft_size, unsigned int fft_rate)
{
    settings.fft_size = fft_size;
    settings.fft_rate = fft_rate;
    delta_t_ms = 1000 / fft_rate;

    if (fft.init(fft_size))
        return 1;

    if (fft_out)
        delete[] fft_out;

    fft_out = new complex_t[fft_size];
    have_fft_out = false;

    return 0;
}

void FftThread::start()
{
    if (start_thread())
    {
        running = true;
        fputs("FFT thread started\n", stderr);
    }
    else
    {
        running = false;
        fputs("Error starting FFT thread\n", stderr);
    }
}

void FftThread::stop()
{
    if (!running)
        return;

    running = false;
    if (exit_thread())
        fputs("Error stopping FFT thread\n", stderr);
    else
        fputs("FFT thread stopped\n", stderr);
}


void FftThread::add_fft_input(uint32_t num_samples, complex_t * input_data)
{
    fft.add_input_samples(num_samples, input_data);
    stats.samples_in += num_samples;
}

uint32_t FftThread::get_fft_output(complex_t * output_data)
{
    if (!have_fft_out)
    {
        stats.underruns++;
        return 0;
    }

    // FIXME: size is constant
    memcpy(output_data, fft_out, settings.fft_size * sizeof(complex_t));
    stats.samples_out += settings.fft_size;
    have_fft_out = false;

    return settings.fft_size;
}


void FftThread::reset_stats()
{
    stats.samples_in = 0;
    stats.samples_out = 0;
    stats.underruns = 0;
}

void FftThread::print_stats()
{
    fprintf(stderr, "FFT stats (IOU): %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
            stats.samples_in, stats.samples_out, stats.underruns);
}

void FftThread::thread_func()
{
    uint_fast64_t       tnow_ms, tprev_ms;

    tprev_ms = 0;

    while (running)
    {
        usleep(1000);

        // is it time to run FFT?
        tnow_ms = time_ms();
        if (tnow_ms - tprev_ms < delta_t_ms)
            continue;

        // are there enough samples?
        if (fft.get_output_samples(fft_out) == 0)
            continue;

        have_fft_out = true;
        tprev_ms = tnow_ms;
    }

    pthread_exit(NULL);
}
