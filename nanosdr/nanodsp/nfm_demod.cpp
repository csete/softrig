/*
 * FM demodulator with noise squelch and audio bandpass filter.
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
#include <stdio.h>
#include "common/datatypes.h"
#include "nfm_demod.h"

#define FMPLL_RANGE     10000.0     // maximum deviation limit of PLL
#define VOICE_BW        3000.0
#define MAX_FMOUT       1.0
#define FMPLL_BW        VOICE_BW    // natural frequency ~loop bandwidth
#define FMPLL_ZETA      0.707       // PLL Loop damping factor
#define FMDC_ALPHA      0.001       // time constant for DC removal filter
#define DEMPHASIS_TIME  80e-6

NfmDemod::NfmDemod()
{
    sample_rate = 0.0;
    freq_err_dc = 0.0;
    nco_phase = 0.0;
    nco_freq = 0.0;
    out_gain = 1.0;

    set_sample_rate(48000.0);
}

void NfmDemod::set_sample_rate(real_t new_rate)
{
    if (sample_rate == new_rate)
        return;

    sample_rate = new_rate;

    real_t norm = K_2PI / sample_rate;  // to normalize Hz to radians

    // initialize the PLL
    nco_lo_limit = -FMPLL_RANGE * norm;      // clamp PLL NCO
    nco_hi_limit = FMPLL_RANGE * norm;
    pll_alpha = 2.0*FMPLL_ZETA*FMPLL_BW * norm;
    pll_beta = (pll_alpha * pll_alpha) / (4.0 * FMPLL_ZETA * FMPLL_ZETA);

    out_gain = MAX_FMOUT / nco_hi_limit;  // audio output level gain

    fprintf(stderr, "  NFM out_gain: %.2f\n", out_gain);

    // DC removal filter time constant
    dc_alpha = (1.0 - MEXP(-1.0/(sample_rate * FMDC_ALPHA)));

    deemph_alpha = (1.0 - MEXP(-1.0 / (sample_rate * DEMPHASIS_TIME)));
    deemph_ave = 0.0;

    lpf.init_lpf(0, 1.0, 50.0, VOICE_BW, 1.6 * VOICE_BW, sample_rate);
    hpf.init_hpf(0, 1.0, 50.0, 350.0, 250.0, sample_rate); // FIXME
}

void NfmDemod::set_voice_bandwidth(real_t bw)
{
    // FIXME: Update FMPLL_BW
    lpf.init_lpf(0, 1.0, 50.0, bw, 1.6 * bw, sample_rate);
}

int NfmDemod::process(int num, const complex_t * inbuf, real_t * outbuf)
{
    complex_t       tmp;
    real_t          sin, cos;
    real_t          phzerror;

    for (int i = 0; i < num; i++)
    {
        sin = MSIN(nco_phase);
        cos = MCOS(nco_phase);

        // complex multiply input sample by NCO
        tmp.re = cos * inbuf[i].re - sin * inbuf[i].im;
        tmp.im = cos * inbuf[i].im + sin * inbuf[i].re;

        // find current sample phase after being shifted by NCO frequency
        phzerror = -MATAN2(tmp.im, tmp.re);

        // create new NCO frequency term
        nco_freq += (pll_beta * phzerror);        //  radians per sampletime

        // clamp NCO frequency so doesn't get out of lock range
        if(nco_freq > nco_hi_limit)
            nco_freq = nco_hi_limit;
        else if(nco_freq < nco_lo_limit)
            nco_freq = nco_lo_limit;

        // update NCO phase with new value
        nco_phase += (nco_freq + pll_alpha * phzerror);

        // LP filter the NCO frequency term to get DC offset value
        freq_err_dc += dc_alpha * (nco_freq - freq_err_dc);

        // subtract out DC term to get FM audio
        outbuf[i] = (nco_freq - freq_err_dc) * out_gain;
    }

    nco_phase = MFMOD(nco_phase, K_2PI);  // keep radian counter bounded

    // low pass filter audio if squelch is open
//    process_deemph_filter(num, outbuf);
//    lpf.process(num, outbuf, outbuf);
//    hpf.process(num, outbuf, outbuf);

    return num;
}

void NfmDemod::process_deemph_filter(int num, real_t * buf)
{
    int     i;
    
    for(i = 0; i < num; i++)
    {
        deemph_ave += deemph_alpha * (buf[i] - deemph_ave);
        buf[i] = deemph_ave;
    }
}


