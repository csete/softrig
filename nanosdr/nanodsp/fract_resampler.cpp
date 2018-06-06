/*
 * Fractional resampler using windowed sinc interpolation
 *
 * Originally from CuteSdr, modified for nanosdr.
 *
 * Copyright 2010-2013  Moe Wheatley AE4JY
 * Copyright 2018       Alexandru Csete OZ9AEC
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
#include <assert.h>
#include "fract_resampler.h"


// Number of input sample periods (zero crossings - 1) in sinc function (even)
// Lower value reduces alias free bandwidth
#define SINC_PERIODS 28

// Number of points in sinc table in each sample period
// Lower value increases noise floor
#define SINC_PERIOD_PTS 10000   

#define SINC_LENGTH     ((SINC_PERIODS) * SINC_PERIOD_PTS + 1)

FractResampler::FractResampler()
{
    sinc_table = 0;
    input_buffer = 0;
    max_input_length = 0;
}

FractResampler::~FractResampler()
{
    if (sinc_table)
        delete sinc_table;
    if (input_buffer)
        delete input_buffer;
}

void FractResampler::init(int max_input)
{
    real_t  fi;
    real_t  window;
    int     i;

    max_input_length = max_input;

    // ensure buffer has room for FIR wrap around
    max_input += SINC_PERIODS;

    if (sinc_table == 0)
        sinc_table = new real_t[SINC_LENGTH];
    if (input_buffer)
        delete input_buffer;

    input_buffer = new complex_t[max_input];
    for (i = 0; i < max_input; i++)
    {
        input_buffer[i].re = 0.0;
        input_buffer[i].im = 0.0;
    }

    for (i = 0; i < SINC_LENGTH; i++)
    {
        // Blackman-Harris window
        window = (0.35875 -
                  0.48829 * MCOS((K_2PI * i) / (SINC_LENGTH - 1)) +
                  0.14128 * MCOS((2.0 * K_2PI * i) / (SINC_LENGTH - 1)) -
                  0.01168 * MCOS((3.0 * K_2PI * i) / (SINC_LENGTH - 1)));

        fi = K_PI * (real_t)(i - SINC_LENGTH / 2) / (real_t) SINC_PERIOD_PTS;
        if (i != SINC_LENGTH / 2)
            sinc_table[i] = window * MSIN(fi) / fi;
        else
            sinc_table[i] = 1.0;
    }

    float_time = 0.0;
}

int FractResampler::resample(int input_length, real_t rate, complex_t * inbuf,
                             complex_t * outbuf)
{
    complex_t   acc;
    int     i, j;
    int     integer_time;
    int     output_samples = 0;

    assert(input_length <= max_input_length);

    j = SINC_PERIODS;
    for (i = 0; i < input_length; i++)
        input_buffer[j++] = inbuf[i];

    // calculate output samples by looping until end of input buffer is reached;
    // the output position is incremented in fractional time of input sample time
    integer_time = (int)float_time;
    while (integer_time < input_length)
    {
        // convolution of sinc with input samples;
        // sinc function is centered at the output fractional time position
        acc.re = 0.0;
        acc.im = 0.0;
        for (i = 1; i <= SINC_PERIODS; i++)
        {
            j = integer_time + i;
            int sindx = (int)(((real_t)j - float_time) * (real_t)SINC_PERIOD_PTS);
            acc.re += (input_buffer[j].re * sinc_table[sindx]);
            acc.im += (input_buffer[j].im * sinc_table[sindx]);
        }
        outbuf[output_samples++] = acc;

        // time increment is the resampling rate
        float_time += rate;
        integer_time = (int)float_time;
    }
    float_time -= (real_t)input_length;

    // keep last SINC_PERIODS input samples (FIR wrap)
    j = input_length;
    for (i = 0; i < SINC_PERIODS; i++)
        input_buffer[i] = input_buffer[j++];

    return output_samples;
}

int FractResampler::resample(int input_length, real_t rate, real_t * inbuf,
                             real_t * outbuf)
{
    real_t  acc;
    int     i, j;
    int     integer_time;       // integer input time accumulator
    int     output_samples = 0;

    assert(input_length <= max_input_length);

    j = SINC_PERIODS;
    for (i = 0; i < input_length; i++)
        input_buffer[j++].re = inbuf[i];

    // calculate output samples by looping until end of input buffer is reached;
    // the output position is incremented in fractional time of input sample time
    integer_time = (int)float_time;
    while (integer_time < input_length)
    {
        // convolution of sinc with input samples;
        // sinc function is centered at the output fractional time position
        acc = 0.0;
        for (i = 1; i <= SINC_PERIODS; i++)
        {
            j = integer_time + i;
            int sindx = (int)(((real_t)j - float_time) * (real_t)SINC_PERIOD_PTS);
            acc += (input_buffer[j].re * sinc_table[sindx]);
        }
        outbuf[output_samples++] = acc;

        // time increment is the resampling rate
        float_time += rate;
        integer_time = (int)float_time;
    }
    float_time -= (real_t)input_length;

    // keep last SINC_PERIODS input samples (FIR wrap)
    j = input_length;
    for (i = 0; i < SINC_PERIODS; i++)
        input_buffer[i].re = input_buffer[j++].re;

    return output_samples;
}
