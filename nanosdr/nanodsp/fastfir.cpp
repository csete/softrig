/*
 * This class implements a FIR bandpass filter using a FFT convolution algorithm
 * The filter is complex and is specified with 3 parameters: sample frequency,
 * high cut-off and low cut-off frequency
 *
 * Originally from CuteSdr and modified for nanosdr.
 *
 * Copyright 2010-2013  Moe Wheatley AE4JY
 * Copyright 2014       Alexandru Csete OZ9AEC
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
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include "fastfir.h"

#define CONV_FFT_SIZE 2048  // must be power of 2
#define CONV_FIR_SIZE 1025  // must be <= FFT size. Make 1/2 + 1 if you want
                            // output to be in power of 2

#define CONV_INBUF_SIZE (CONV_FFT_SIZE+CONV_FIR_SIZE-1)


FastFIR::FastFIR()
{
    int     i;

    window = NULL;
    fftbuf = NULL;
    fftovrbuf = NULL;
    filter_coef = NULL;

    window = new real_t[CONV_FIR_SIZE];
    filter_coef = new complex_t[CONV_FFT_SIZE];
    fftbuf = new complex_t[CONV_FFT_SIZE];
    fftovrbuf = new complex_t[CONV_FIR_SIZE];

    if (!window || !filter_coef || !fftbuf || !fftovrbuf)
        return;

    inbuf_inpos = (CONV_FIR_SIZE - 1);
    for (i = 0; i < CONV_FFT_SIZE; i++)
    {
        fftbuf[i].re = 0.0;
        fftbuf[i].im = 0.0;
    }
#if 1
    // Blackman-Nuttall window function for windowed sinc low pass filter design
    for (i = 0; i < CONV_FIR_SIZE; i++)
    {
        window[i] = (0.3635819
            - 0.4891775 * MCOS((K_2PI * i) / (CONV_FIR_SIZE - 1))
            + 0.1365995 * MCOS((2.0 * K_2PI * i) / (CONV_FIR_SIZE - 1))
            - 0.0106411 * MCOS((3.0 * K_2PI * i) / (CONV_FIR_SIZE - 1)));
        fftovrbuf[i].re = 0.0;
        fftovrbuf[i].im = 0.0;
    }
#endif
#if 0
    // Blackman-Harris window function for windowed sinc low pass filter design
    for (i = 0; i < CONV_FIR_SIZE; i++)
    {
        window[i] = (0.35875
            - 0.48829 * MCOS((K_2PI * i) / (CONV_FIR_SIZE - 1))
            + 0.14128 * MCOS((2.0 * K_2PI * i) / (CONV_FIR_SIZE - 1))
            - 0.01168 * MCOS((3.0 * K_2PI * i) / (CONV_FIR_SIZE - 1)));
        fftovrbuf[i].re = 0.0;
        fftovrbuf[i].im = 0.0;
    }
#endif
#if 0
    // Nuttall window function for windowed sinc low pass filter design
    for (i = 0; i < CONV_FIR_SIZE; i++)
    {
        window[i] = (0.355768
            - 0.487396 * MCOS((K_2PI * i) / (CONV_FIR_SIZE - 1))
            + 0.144232 * MCOS((2.0 * K_2PI * i) / (CONV_FIR_SIZE - 1))
            - 0.012604 * MCOS((3.0 * K_2PI * i) / (CONV_FIR_SIZE - 1)));
        fftovrbuf[i].re = 0.0;
        fftovrbuf[i].im = 0.0;
    }
#endif
    m_Fft.setup(CONV_FFT_SIZE);
    locut = -1.0;
    hicut = 1.0;
    offset = 1.0;
    samprate = 1.0;
}

FastFIR::~FastFIR()
{
    free_memory();
}

void FastFIR::free_memory()
{
    if (window)
    {
        delete[] window;
        window = NULL;
    }
    if (fftovrbuf)
    {
        delete[] fftovrbuf;
        fftovrbuf = NULL;
    }
    if (filter_coef)
    {
        delete[] filter_coef;
        filter_coef = NULL;
    }
    if (fftbuf)
    {
        delete[] fftbuf;
        fftbuf = NULL;
    }
}

void FastFIR::setup(real_t low_cut, real_t high_cut, real_t cw_offs, real_t fs)
{
    if ((low_cut >= high_cut) ||
        (low_cut >= fs / 2.0) ||
        (low_cut <= -fs / 2.0) ||
        (high_cut >= fs / 2.0) ||
        (high_cut <= -fs / 2.0))
    {
        fputs("Filter Parameter error\n", stderr);
        return;
    }

    if ((low_cut == locut) && (high_cut == hicut) &&
        (cw_offs == offset) && (fs == samprate))
    {
        return;
    }

    // store new parameters
    locut = low_cut;
    hicut = high_cut;
    offset = cw_offs;
    samprate = fs;
    locut += cw_offs;
    hicut += cw_offs;

    // normalized filter parameters
    real_t  nFL = locut / fs;
    real_t  nFH = hicut / fs;
    real_t  nFc = (nFH - nFL) / 2.0;         // prototype LP filter cutoff
    real_t  nFs = K_2PI * (nFH + nFL) / 2.0; // 2*PI times required frequency shift
    real_t  fCenter = 0.5 * (real_t)(CONV_FIR_SIZE-1);

    for (int i = 0; i < CONV_FFT_SIZE; i++)
    {
        filter_coef[i].re = 0.0;
        filter_coef[i].im = 0.0;
    }

    // create LP FIR windowed sinc, sin(x)/x complex LP filter coefficients
    for (int i = 0; i < CONV_FIR_SIZE; i++)
    {
        real_t x = (real_t)i - fCenter;
        real_t z;

        if ((real_t) i == fCenter)    // deal with odd size filter singularity where sin(0)/0==1
            z = 2.0 * nFc;
        else
            z = (real_t)MSIN(K_2PI * x * nFc) / (K_PI * x) * window[i];

        // shift lowpass filter coefficients in frequency by (hicut+lowcut)/2 to
        // form bandpass filter anywhere in range
        // (also scales by 1/FFTsize since inverse FFT routine scales by FFTsize)
        filter_coef[i].re = z * MCOS(nFs * x) / (real_t)CONV_FFT_SIZE;
        filter_coef[i].im = z * MSIN(nFs * x) / (real_t)CONV_FFT_SIZE;
    }

    // convert FIR coefficients to frequency domain by taking forward FFT
    m_Fft.fwd_fft(filter_coef);
}

void FastFIR::set_sample_rate(real_t new_rate)
{
    setup(locut, hicut, offset, new_rate);
}

int FastFIR::process(int num, complex_t * inbuf, complex_t * outbuf)
{
    int     i = 0;
    int     j;
    int     len = num;
    int     outpos = 0;

    if (!num)
        return 0;

    while (len--)
    {
        j = inbuf_inpos - (CONV_FFT_SIZE - CONV_FIR_SIZE + 1) ;
        if (j >= 0)
            // keep copy of last CONV_FIR_SIZE-1 samples for overlap save
            fftovrbuf[j] = inbuf[i];

        fftbuf[inbuf_inpos++] = inbuf[i++];
        if (inbuf_inpos >= CONV_FFT_SIZE)
        {
            m_Fft.fwd_fft(fftbuf);
            cpx_mpy(CONV_FFT_SIZE, filter_coef, fftbuf, fftbuf);
            m_Fft.rev_fft(fftbuf);

            for (j = CONV_FIR_SIZE - 1; j < CONV_FFT_SIZE; j++)
                // copy FFT output into OutBuf minus CONV_FIR_SIZE-1 samples at beginning
                outbuf[outpos++] = fftbuf[j];

            for (j = 0; j < CONV_FIR_SIZE - 1; j++)
                // copy overlap buffer into start of fft input buffer
                fftbuf[j] = fftovrbuf[j];

            // reset input position to data start position of fft input buffer
            inbuf_inpos = CONV_FIR_SIZE - 1;
        }
    }

    // return number of output samples processed and placed in OutBuf
    return outpos;
}

/*
 * Complex multiply N point array m with src and place in dest.
 * src and dest can be the same buffer.
 */
inline void FastFIR::cpx_mpy(int N, complex_t * m, complex_t * src,
                             complex_t * dest)
{
    for (int i = 0; i < N; i++)
    {
        real_t  sr = src[i].re;
        real_t  si = src[i].im;
        dest[i].re = m[i].re * sr - m[i].im * si;
        dest[i].im = m[i].re * si + m[i].im * sr;
    }
}
