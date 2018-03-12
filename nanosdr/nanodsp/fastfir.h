/*
 * This class implements a FIR bandpass filter using a FFT convolution algorithm
 * The filter is complex and is specified with 3 parameters: sample frequency,
 * high cut-off and low cut-off frequency
 */
#pragma once

#include "common/datatypes.h"
#include "cute_fft.h"

class FastFIR
{
public:
    FastFIR();
    virtual    ~FastFIR();

    /*
     * Setup filter parameters
     *   low_cut   Low cutoff frequency of filter in Hz
     *   high_cut  High cutoff frequency of filter in Hz
     *   cw_offs   The CW tone frequency.
     *   fs        Sample rate in Hz.
     *
     * Cutoff frequencies range from -SampleRate/2 to +SampleRate/2
     * high_cut must be greater than low_cut
     */
    void        setup(real_t low_cut, real_t high_cut, real_t cw_offs, real_t fs);
    void        set_sample_rate(real_t new_rate);

    /*
     * Process complex samples
     *   num      The number of complex samples in the input buffer.
     *   inbuf    Input buffer.
     *   outbuf   Outbut buffer.
     *
     * Returns the number of complex samples placed in the output buffer.
     *
     * The number of samples returned in general will not be equal to the number
     * of input samples due to FFT block size processing.
     */
    int         process(int num, complex_t * inbuf, complex_t * outbuf);

private:
    inline void cpx_mpy(int N, complex_t * m, complex_t * src, complex_t * dest);
    void        free_memory();

    real_t      locut;
    real_t      hicut;
    real_t      offset;
    real_t      samprate;

    unsigned int    inbuf_inpos;
    real_t     *window;         // window coefficients
    complex_t  *fftbuf;         // FFT buffer
    complex_t  *fftovrbuf;      // FFT overlap buffer
    complex_t  *filter_coef;    // Filter coefficients

    CuteFft     m_Fft;
};
