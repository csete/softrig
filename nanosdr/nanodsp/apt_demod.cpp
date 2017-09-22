/*
 * FM demodulator for NOAA APT (17 kHz deviation + 3 kHz Doppler)
 *
 * Copyright 2010-2013  Moe Wheatley
 * Copyright 2016       Alexandru Csete
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
#include "apt_demod.h"

#define FMPLL_RANGE     30000.0     // maximum deviation limit of PLL
#define VOICE_BW        5000.0      // FIXME
#define MAX_FMOUT       0.8
#define FMPLL_BW        VOICE_BW    // natural frequency ~loop bandwidth
#define FMPLL_ZETA      0.707       // PLL Loop damping factor
#define FMDC_ALPHA      0.001       // time constant for DC removal filter

AptDemod::AptDemod()
{
    freq_err_dc = 0.0;
    nco_phase = 0.0;
    nco_freq = 0.0;
    out_gain = 1.0;

    set_sample_rate(96000.0);
}

void AptDemod::set_sample_rate(real_t new_rate)
{
    if (sample_rate == new_rate)
        return;

    sample_rate = new_rate;

    real_t norm = K_2PI / sample_rate;  // to normalize Hz to radians

    // initialize the PLL
    nco_lo_limit = -FMPLL_RANGE * norm;      // clamp PLL NCO
    nco_hi_limit = FMPLL_RANGE * norm;
    pll_alpha = 2.0 * FMPLL_ZETA * FMPLL_BW * norm;
    pll_beta = (pll_alpha * pll_alpha) / (4.0 * FMPLL_ZETA * FMPLL_ZETA);

    out_gain = MAX_FMOUT / nco_hi_limit;  // audio output level gain

    fprintf(stderr, "  APT out_gain: %.2f\n", out_gain);

    // DC removal filter time constant
    dc_alpha = (1.0 - MEXP(-1.0 / (sample_rate * FMDC_ALPHA)));
}

int AptDemod::process(int num, const complex_t * inbuf, real_t * outbuf)
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
        if (nco_freq > nco_hi_limit)
            nco_freq = nco_hi_limit;
        else if (nco_freq < nco_lo_limit)
            nco_freq = nco_lo_limit;

        // update NCO phase with new value
        nco_phase += (nco_freq + pll_alpha * phzerror);

        // LP filter the NCO frequency term to get DC offset value
        freq_err_dc += dc_alpha * (nco_freq - freq_err_dc);

        // subtract out DC term to get FM audio
        outbuf[i] = (nco_freq - freq_err_dc) * out_gain;
    }

    nco_phase = MFMOD(nco_phase, K_2PI);  // keep radian counter bounded

    return num;
}
