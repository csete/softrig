/*
 * Frequency translator.
 *
 * Originally from CuteSdr, modified for nanosdr.
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
#include <stdlib.h>
#include "translate.h"


Translate::Translate()
{
    sample_rate = 96000.0;
    nco_freq = 0.0;
    cw_offset = 0.0;
    nco_inc = 0.0;
    osc_cos = 1.0;
    osc_sin = 0.0;

    osc1.re = 1.0;
    osc1.im = 0.0;
}


void Translate::set_nco_frequency(real_t freq_hz)
{
    nco_freq = freq_hz + cw_offset;
    nco_inc = K_2PI * nco_freq / sample_rate;
    osc_cos = MCOS(nco_inc);
    osc_sin = MSIN(nco_inc);
}

void Translate::set_cw_offset(real_t offset_hz)
{
    real_t real_nco = nco_freq - cw_offset;
    cw_offset = offset_hz;
    set_nco_frequency(real_nco);
}

void Translate::set_sample_rate(real_t rate)
{
    if(sample_rate != rate)
    {
        real_t real_nco = nco_freq - cw_offset;
        sample_rate = rate;
        set_nco_frequency(real_nco);
    }
}

void Translate::process(int length, complex_t * data)
{
    complex_t  dtmp;
    complex_t  osc;
    real_t     osc_gn;
    int        i;

    for (i = 0; i < length; i++)
    {
        dtmp.re = data[i].re;
        dtmp.im = data[i].im;

        osc.re = osc1.re * osc_cos - osc1.im * osc_sin;
        osc.im = osc1.im * osc_cos + osc1.re * osc_sin;
        //osc_gn = 1.95 - (osc1.re * osc1.re + osc1.im * osc1.im);
        osc_gn = 1.99 - (osc1.re * osc1.re + osc1.im * osc1.im);
        osc1.re = osc_gn * osc.re;
        osc1.im = osc_gn * osc.im;

        // complex multiply by shift frequency
        data[i].re = ((dtmp.re * osc.re) - (dtmp.im * osc.im));
        data[i].im = ((dtmp.re * osc.im) + (dtmp.im * osc.re));
    }
}
