/*
 * FFT class for nanosdr.
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
#include <stdint.h>

#include "common/datatypes.h"
#include "common/ring_buffer_cplx.h"
#include "kiss_fft.h"

#include "fft.h"


CFft::CFft()
{
    fft_cfg = NULL;
    fft_size = 0;
    fft_window = NULL;
    fft_work_buffer = NULL;
    fft_input_buffer = NULL;
}

CFft::~CFft()
{
    free_memory();
}

void CFft::free_memory()
{
    if (fft_cfg != NULL)
    {
        kiss_fft_free(fft_cfg);
        fft_cfg = NULL;
    }

    if (fft_window != NULL)
    {
        delete[] fft_window;
        fft_window = NULL;
    }

    if (fft_work_buffer != NULL)
    {
        delete[] fft_work_buffer;
        fft_work_buffer = NULL;
    }

    if (fft_input_buffer != NULL)
    {
        ring_buffer_cplx_delete(fft_input_buffer);
        fft_input_buffer = NULL;
    }
}

int CFft::init(uint32_t size)
{
    unsigned int        i;

    if ((size < FFT_MIN_SIZE) || (size > FFT_MAX_SIZE))
        return -1;
    else if (size == fft_size)
        return 0;

    free_memory();

    fft_size = size;
    fft_cfg = kiss_fft_alloc(fft_size, 0, NULL, NULL);
    if (fft_cfg == NULL)
        return -2;

    // FFT window
    fft_window = new real_t[fft_size];
    real_t  window_gain = 2.0;
    for (i = 0; i < fft_size; i++)  // Hann
        fft_window[i] = window_gain * (0.5 - 0.5 * MCOS((K_2PI * i) / (fft_size - 1)));

    // FFT buffers
    fft_work_buffer = new complex_t[fft_size];
    fft_input_buffer = ring_buffer_cplx_create();
    if (fft_input_buffer == NULL)
        return -2;
    ring_buffer_cplx_init(fft_input_buffer, fft_size);

    return 0;
}

void CFft::add_input_samples(uint32_t num, complex_t * inbuf)
{
    // don't try to write more data than what the FFT buffer can hold
    if (num <= fft_size)
        ring_buffer_cplx_write(fft_input_buffer, inbuf, num);
    else
        ring_buffer_cplx_write(fft_input_buffer, &inbuf[num - fft_size],
                               fft_size);
}

uint32_t CFft::get_output_samples(complex_t * outbuf)
{
    if (ring_buffer_cplx_count(fft_input_buffer) < (uint_fast32_t)fft_size)
        return 0;

    // get data and run FFT
    ring_buffer_cplx_read(fft_input_buffer, fft_work_buffer, fft_size);
    process(fft_work_buffer, outbuf);

    return fft_size;
}

void CFft::process(complex_t * input, complex_t * output)
{
    unsigned int        i;

    const kiss_fft_cpx  *fin  = (kiss_fft_cpx *) input;
    kiss_fft_cpx        *fout = (kiss_fft_cpx *) output;

    // window the FFT data
    for (i = 0; i < fft_size; i++)
    {
        input[i].re = fft_window[i] * input[i].re;
        input[i].im = fft_window[i] * input[i].im;
    }

    kiss_fft(fft_cfg, fin, fout);
}
