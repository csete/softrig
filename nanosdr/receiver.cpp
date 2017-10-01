/*
 * SDR receiver implementation
 *
 * Copyright 2017 Alexandru Csete OZ9AEC
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
#include <stdio.h>
#include <stdlib.h>
#include <speex/speex_resampler.h> // must come after stdint.h

#include "common/bithacks.h"
#include "common/datatypes.h"
#include "common/sdr_data.h"
#include "common/time.h"
#include "nanodsp/agc.h"
#include "nanodsp/amdemod.h"
#include "nanodsp/fastfir.h"
#include "nanodsp/filter/decimator.h"
#include "nanodsp/smeter.h"
#include "nanodsp/ssbdemod.h"
#include "nanodsp/translate.h"
#include "receiver.h"


Receiver::Receiver()
{
    sql_level = -160.f;
    input_rate = 96000.0f;
    quad_decim = 2;
    quad_rate = input_rate / quad_decim;
    output_rate = 48000.0f;
    demod = SDR_DEMOD_SSB;
    resampler = 0;
    cplx_buf0 = 0;
    cplx_buf1 = 0;
    cplx_buf2 = 0;
    real_buf1 = 0;
}

Receiver::~Receiver()
{
    free_memory();
}

void Receiver::free_memory()
{
    if (resampler)
        speex_resampler_destroy(resampler);

    delete[] cplx_buf0;
    delete[] cplx_buf1;
    delete[] cplx_buf2;
    delete[] real_buf1;
}

void Receiver::init(real_t in_rate, real_t out_rate, real_t dyn_range,
                    uint32_t frame_length)
{
    int error;

    fprintf(stderr,
            "Initializing receiver (dynamic range %.2f dB)...\n",
            dyn_range);

    buflen = 2 * frame_length; // FIXME: frame_length -> ms and use quad_rate
    input_rate = in_rate;
    output_rate = out_rate;

    // find a decimation rate based on preferred quad rate = 48 ksps
    quad_decim = next_power_of_two(input_rate / 48000);
    if (quad_decim == 1 && input_rate > 48000)
        quad_decim = 2;

    quad_decim = decim.init(quad_decim, dyn_range);
    quad_rate = input_rate / (float) quad_decim;

    fprintf(stderr,
            "Receiver sample rates:\n"
            "   input rate: %.2f Hz\n"
            "   decimation: %d\n"
            "    quad rate: %.2f Hz\n"
            "  output rate: %.2f Hz\n",
            input_rate, quad_decim, quad_rate, output_rate);

    if (frame_length % quad_decim)
        fprintf(stderr,
            "*** WARNING: frame_length is not an integer multiple of the decimation:\n"
            "                 %u %% %u = %u\n",
            frame_length, quad_decim, frame_length % quad_decim);

    // free buffers and object that need to be re-initialized
    free_memory();

    // update working buffers
    cplx_buf0 = new complex_t[buflen];
    cplx_buf1 = new complex_t[buflen];
    cplx_buf2 = new complex_t[buflen];
    real_buf1 = new real_t[buflen];

    // initialize DSP blocks
    filter.setup(-2800.0, -100.0, 0.0, quad_rate);
    agc.setup(true, false, -80, 0, 2, 500, quad_rate);
    am.setup(quad_rate, 4000);
    nfm.set_sample_rate(quad_rate);
    bfo.set_sample_rate(quad_rate);

    // re-initialize resampler
    fputs("*** FIXME: Check resampler rates (should be INT)\n", stderr);
    fputs("*** FIXME: Check resampler quality setting\n", stderr);
    resampler = speex_resampler_init(1, quad_rate, output_rate, 3, &error);
    if (error || !resampler)
        fprintf(stderr, "%s\n", speex_resampler_strerror(error));
        // FIXME: now what?
}


void Receiver::set_tuning_offset(real_t offset)
{
    (void)  offset;
}

void Receiver::set_agc(int threshold, int slope, int decay)
{
    agc.setup(true, false, threshold, 50, slope, decay, quad_rate);
}

void Receiver::set_filter(real_t low_cut, real_t high_cut)
{
    fprintf(stderr, "   FILT   LO:%.0f   HI:%.0f\n", low_cut, high_cut);
    filter.setup(low_cut, high_cut, 0.f, quad_rate); // NB: fffset is ignored
}

void Receiver::set_cw_offset(real_t offset)
{
    bfo.set_cw_offset(offset);
}

void Receiver::set_demod(sdr_demod_t new_demod)
{
    switch (new_demod)
    {
    case SDR_DEMOD_NONE:
    case SDR_DEMOD_SSB:
    case SDR_DEMOD_AM:
    case SDR_DEMOD_FM:
        demod = new_demod;
        break;
    default:
        fprintf(stderr, "set_demod called with unsupported demodulator: %d\n",
                new_demod);
        break;
    }
}

/*
 * returns the number of audio output samples or -1 if signal strength
 * is below squelch level.
 */
int Receiver::process(int input_length, complex_t * input, real_t * output)
{
    spx_uint32_t    sin, sout;
    int         filt_samples;
    int         quad_samples;
    int         error;

    //xlate->process(input_length, input);
    quad_samples = decim.process(input_length, input);
    if (quad_samples == 0)
        return 0;

    filt_samples = filter.process(quad_samples, input, cplx_buf1);
    if (filt_samples == 0)
        return 0;

    // run s-meter and check squelch
    if (meter.process(filt_samples, cplx_buf1) < sql_level)
        return -1;

    switch (demod)
    {
    case SDR_DEMOD_SSB:
    default:
        agc.process(filt_samples, cplx_buf1, cplx_buf2);
        // FIXME: only if offset != 0
        bfo.process(filt_samples, cplx_buf2);
        ssb.process(filt_samples, cplx_buf2, real_buf1);
        break;

    case SDR_DEMOD_AM:
        agc.process(filt_samples, cplx_buf1, cplx_buf2);
        am.process(filt_samples, cplx_buf2, real_buf1);
        break;

    case SDR_DEMOD_FM:
        nfm.process(filt_samples, cplx_buf1, real_buf1);
        break;
    }

    sin = filt_samples;
    sout = filt_samples;
    error = speex_resampler_process_float(resampler, 0, real_buf1, &sin,
                                          output, &sout);
    if (error)
        fprintf(stderr, "%s\n", speex_resampler_strerror(error));

    return sout;
}

real_t Receiver::get_signal_strength(void) const
{
    return meter.get_signal_power();
}
