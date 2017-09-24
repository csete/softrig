#pragma once

#include <stdint.h>
#include <speex/speex_resampler.h> // must come after stdint.h

#include "common/datatypes.h"
#include "nanodsp/agc.h"
#include "nanodsp/amdemod.h"
#include "nanodsp/fastfir.h"
#include "nanodsp/filter/decimator.h"
#include "nanodsp/nfm_demod.h"
#include "nanodsp/smeter.h"
#include "nanodsp/ssbdemod.h"
#include "nanodsp/translate.h"

// FIXME: should be defined as CTL params
#define RX_DEMOD_NONE   0
#define RX_DEMOD_SSB    1
#define RX_DEMOD_AM     2
#define RX_DEMOD_NFM    3

class Receiver
{
public:
    Receiver();
    virtual ~Receiver();

    void init(real_t in_rate, real_t out_rate, real_t dyn_range,
              uint32_t frame_length);
    void set_tuning_offset(real_t offset);
    void set_filter(real_t low_cut, real_t high_cut, real_t offset);
    void set_agc(int threshold, int slope, int decay);
    void set_demod(uint8_t new_demod);
    void set_sql(real_t level)
    {
        sql_level = level;
    }

    int process(int input_length, complex_t * input, real_t * output);

    real_t  get_signal_strength(void) const;

private:
    void free_memory(void);

private:
    FastFIR     filter;
    Decimator   decim;
    SMeter      meter;
    CAgc        agc;
    NfmDemod    nfm;
    AmDemod     am;
    SsbDemod    ssb;
    Translate   bfo;            // used to provide CW offset in single user mode

    SpeexResamplerState *resampler; 

    real_t      sql_level;
    real_t      input_rate;
    real_t      quad_rate;
    real_t      output_rate;

    unsigned int    quad_decim;

    uint8_t     demod;
    uint32_t    buflen;
    complex_t  *cplx_buf0;
    complex_t  *cplx_buf1;
    complex_t  *cplx_buf2;
    real_t     *real_buf1;
};

