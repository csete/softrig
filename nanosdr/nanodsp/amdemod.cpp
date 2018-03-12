/*
 * AM demodulator.
 * Originally from CuteSdr.
 *
 * Copyright  2010  Moe Wheatley AE4JY
 * Copyright  2015  Alexandru Csete OZ9AEC
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

#include "amdemod.h"
#include "common/datatypes.h"

// ALPHA for DC removal IIR
// See http://www.dsprelated.com/freebooks/filters/DC_Blocker.html
#define DC_ALPHA 0.995

AmDemod::AmDemod()
{
    sample_rate = 48000.f;
	z1 = 0.f;
	audio_filter.init_lpf(0, 1., 60., 5000., 5000. * 1.8, sample_rate);
}

void AmDemod::setup(real_t input_rate, real_t bandwidth)
{
    sample_rate = input_rate;
    z1 = 0.0;
	audio_filter.init_lpf(0, 1.0, 50.0, bandwidth, bandwidth * 1.8, sample_rate);
}

int AmDemod::process(int num, complex_t * data_in, real_t * data_out)
{
    complex_t   in;
    real_t      mag;
    real_t      z0;
    int         i;

	for (i = 0; i < num; i++)
	{
		in = data_in[i];
     	mag = MSQRT(in.re * in.re + in.im * in.im);

		// High pass filter for DC removal using IIR filter
		// H(z) = (1 - z^-1)/(1 - ALPHA * z^-1)
		z0 = mag + (z1 * DC_ALPHA);
		data_out[i] = (z0 - z1);
		z1 = z0;
	}

	// post demod audio filter to limit high frequency noise
	audio_filter.process(num, data_out, data_out);

	return num;
}
