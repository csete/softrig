/*
 * FM demodulator for NOAA APT (17 kHz deviation + 3 kHz Doppler)
 */
#pragma once

#include "common/datatypes.h"

class AptDemod
{
public:
    AptDemod();

    int     process(int num, const complex_t * inbuf, real_t * outbuf);
    void    set_sample_rate(real_t new_rate);

private:
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
};
