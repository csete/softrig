/*
 * This class implements a FIR  filter using a dual flat coefficient
 * array to eliminate testing for buffer wrap around.
 *
 * Filter coefficients can be from a fixed table or generated from frequency
 * and attenuation specifications using a Kaiser-Bessel windowed sinc algorithm.
 *
 * Originally from CuteSdr and modified for nanosdr.
 */
#pragma once

#include "common/datatypes.h"
#include "filtercoef.h"

#define MAX_NUMCOEF 75

class Fir
{
public:
    Fir();

    /**
     * Initializes a pre-designed FIR filter with real coefficients.
     * @param ncoef The number of coefficients.
     * @param coef  The array containing the filter coefficients.
     * @param Fs    The sample rate in Hz.
     */
    void        init_const_fir(int ncoef, const real_t * coef, real_t Fs);

    /**
     * Initializes a pre-designed FIR filter with complex coefficients.
     * @param ncoef The number of coefficients.
     * @param icoef Array containing the filter coefficients (I).
     * @param qcoef Array containing the filter coefficients (Q).
     * @param Fs    The sample rate in Hz.
     */
    void        init_const_fir(int ncoef, const real_t * icoef,
                               const real_t * qcoef, real_t Fs);

    /**
     * Create a FIR Low Pass filter with scaled amplitude.
     * @param ntaps If non-zero, forces filter design to be this number of taps.
     * @param scale Linear amplitude scale factor.
     * @param Astop Stopband Atenuation in dB (i.e. 40 is 40 dB attenuation).
     * @param Fpass Lowpass passband frequency in Hz.
     * @param Fstop Lowpass stopband frequency in Hz.
     * @param Fs    Sample Rate in Hz.
     * @returns The number of taps.
     *
     *           -------------
     *                        \
     *                         \
     *                          \
     *                           \
     *                            ---------------  Astop
     *                    Fpass   Fstop
     */
    int         init_lpf(int ntaps, real_t scale, real_t Astop, real_t Fpass,
                         real_t Fstop, real_t Fs);

    /**
     * Create a FIR high Pass filter with scaled amplitude.
     * @param ntaps If non-zero, forces filter design to be this number of taps.
     * @param scale Linear amplitude scale factor.
     * @param Astop Stopband atenuation in dB (i.e. 40 is 40 dB attenuation).
     * @param Fpass Highpass passband frequency in Hz.
     * @param Fstop Highpass stopband frequency in Hz.
     * @param Fs    Sample Rate in Hz.
     * @returns The number of taps.
     *
     *                        -------------
     *                       /
     *                      /
     *                     /
     *                    /
     *   Astop   ---------
     *                Fstop   Fpass
     */
    int         init_hpf(int ntaps, real_t scale, real_t Astop, real_t Fpass,
                         real_t Fstop, real_t Fs);

    void        process(int num, real_t * inbuf, real_t * outbuf);
    void        process(int num, real_t * inbuf, complex_t * outbuf);
    void        process(int num, complex_t * inbuf, complex_t * outbuf);

private:
    real_t      Izero(real_t x);
    real_t      m_SampleRate;
    int         m_NumTaps;
    int         m_State;
    real_t      m_Coef[MAX_NUMCOEF*2];
    real_t      m_ICoef[MAX_NUMCOEF*2];
    real_t      m_QCoef[MAX_NUMCOEF*2];
    real_t      m_rZBuf[MAX_NUMCOEF];
    complex_t   m_cZBuf[MAX_NUMCOEF];
};
