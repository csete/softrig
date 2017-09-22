/*
 * FM demodulator with de-emphasis an audio filter
 */
#pragma once

#include "common/datatypes.h"
#include "fir.h"

class NfmDemod
{
public:
    NfmDemod();

    int  process(int num, const complex_t * inbuf, real_t * outbuf);

    void set_sample_rate(real_t new_rate);
    void set_voice_bandwidth(real_t bw);

private:
    void process_deemph_filter(int num, real_t * buf);

    Fir         lpf;
    Fir         hpf;
    real_t      sample_rate;
    real_t      out_gain;
    real_t      freq_err_dc;
    real_t      dc_alpha;
    real_t      nco_phase;
    real_t      nco_freq;
    real_t      nco_lo_limit;
    real_t      nco_hi_limit;
    real_t      pll_alpha;
    real_t      pll_beta;

    real_t      deemph_ave;
    real_t      deemph_alpha;
};
