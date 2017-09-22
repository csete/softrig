/*
 * This class implements a FIR  filter using a dual flat coefficient
 * array to eliminate testing for buffer wrap around.
 *
 * Filter coefficients can be from a fixed table or generated from frequency
 * and attenuation specifications using a Kaiser-Bessel windowed sinc algorithm.
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
#include "fir.h"

#define MAX_HALF_BAND_BUFSIZE 8192

Fir::Fir()
{
    m_NumTaps = 1;
    m_State = 0;
}

void Fir::init_const_fir(int ncoef, const real_t * coef, real_t Fs)
{
    m_SampleRate = Fs;

    if (ncoef > MAX_NUMCOEF)
        m_NumTaps = MAX_NUMCOEF;
    else
        m_NumTaps = ncoef;

    for (int i = 0; i < m_NumTaps; i++)
    {
        m_Coef[i] = coef[i];
        m_Coef[m_NumTaps+i] = coef[i];  // create duplicate for calculation efficiency
    }
    for (int i = 0; i < m_NumTaps; i++)
    {
        m_rZBuf[i] = 0.0;
        m_cZBuf[i].re = 0.0;
        m_cZBuf[i].im = 0.0;
    }
    m_State = 0;
}

void Fir::init_const_fir(int ncoef, const real_t * icoef, const real_t * qcoef,
                          real_t Fs)
{
    m_SampleRate = Fs;

    if (ncoef > MAX_NUMCOEF)
        m_NumTaps = MAX_NUMCOEF;
    else
        m_NumTaps = ncoef;

    for (int i = 0; i < m_NumTaps; i++)
    {
        m_ICoef[i] = icoef[i];
        m_ICoef[m_NumTaps+i] = icoef[i];
        m_QCoef[i] = qcoef[i];
        m_QCoef[m_NumTaps+i] = qcoef[i];
    }

    for (int i = 0; i < m_NumTaps; i++)
    {
        m_rZBuf[i] = 0.0;
        m_cZBuf[i].re = 0.0;
        m_cZBuf[i].im = 0.0;
    }
    m_State = 0;
}

int Fir::init_lpf(int ntaps, real_t scale, real_t Astop, real_t Fpass,
                         real_t Fstop, real_t Fs)
{
    int         n;
    real_t      Beta;
    real_t      normFpass = Fpass / Fs;
    real_t      normFstop = Fstop / Fs;
    real_t      normFcut = (normFstop + normFpass) / 2.0;   // 6 dB cutoff

    m_SampleRate = Fs;

    // Kaiser-Bessel window shape factor, Beta, from stopband attenuation
    if (Astop < 20.96)
        Beta = 0;
    else if (Astop >= 50.0)
        Beta = .1102 * (Astop - 8.71);
    else
        Beta = .5842 * MPOW((Astop - 20.96), 0.4) + .07886 * (Astop - 20.96);

    if (ntaps)
        m_NumTaps = ntaps;
    else
        // Estimate number of filter taps required based on filter specs
        m_NumTaps = (Astop - 8.0) / (2.285 * K_2PI * (normFstop - normFpass)) + 1;

    if (m_NumTaps > MAX_NUMCOEF)
        m_NumTaps = MAX_NUMCOEF;
    if (m_NumTaps < 3)
        m_NumTaps = 3;

    real_t      fCenter = .5 * (real_t)(m_NumTaps - 1);
    real_t      izb = Izero(Beta);

    for (n = 0; n < m_NumTaps; n++)
    {
        real_t  x = (real_t)n - fCenter;
        real_t  c;

        // ideal Sinc() LP filter with normFcut
        if ((real_t)n == fCenter)   // deal with odd size filter singularity where sin(0)/0 == 1
            c = 2.0 * normFcut;
        else
            c = MSIN(K_2PI * x * normFcut) / (K_PI * x);

        // calculate Kaiser window and multiply to get coefficient
        x = ((real_t)n - ((real_t)m_NumTaps - 1.0) / 2.0) /
            (((real_t)m_NumTaps - 1.0) / 2.0);
        m_Coef[n] = scale * c * Izero(Beta * MSQRT(1 - (x * x))) / izb;
    }

    // make a 2x length array for FIR flat calculation efficiency
    for (n = 0; n < m_NumTaps; n++)
        m_Coef[n + m_NumTaps] = m_Coef[n];

    // copy into complex coef buffers
    for (n = 0; n < m_NumTaps * 2; n++)
    {
        m_ICoef[n] = m_Coef[n];
        m_QCoef[n] = m_Coef[n];
    }

    // Initialize the FIR buffers and state
    for (n = 0; n < m_NumTaps; n++)
    {
        m_rZBuf[n] = 0.0;
        m_cZBuf[n].re = 0.0;
        m_cZBuf[n].im = 0.0;
    }
    m_State = 0;

    return m_NumTaps;
}

int Fir::init_hpf(int ntaps, real_t scale, real_t Astop, real_t Fpass,
                   real_t Fstop, real_t Fs)
{
    int     n;
    real_t  Beta;

    m_SampleRate = Fs;

    real_t  normFpass = Fpass / Fs;
    real_t  normFstop = Fstop / Fs;
    real_t  normFcut = (normFstop + normFpass) / 2.0;  // 6 dB cutoff

    // Kaiser-Bessel window shape factor, Beta, from stopband attenuation
    if (Astop < 20.96)
        Beta = 0;
    else if (Astop >= 50.0)
        Beta = .1102 * (Astop - 8.71);
    else
        Beta = .5842 * MPOW( (Astop-20.96), 0.4) + .07886 * (Astop - 20.96);

    if (ntaps)
        m_NumTaps = ntaps;
    else
        // estimate number of filter taps required based on filter specs
        m_NumTaps = (Astop - 8.0) / (2.285 * K_2PI * (normFpass - normFstop)) + 1;

    if (m_NumTaps > MAX_NUMCOEF - 1)
        m_NumTaps = MAX_NUMCOEF - 1;
    if (m_NumTaps < 3)
        m_NumTaps = 3;

    m_NumTaps |= 1;     // FIXME: Why force to next odd number?
    
    real_t  izb = Izero(Beta);
    real_t  fCenter = .5 * (real_t)(m_NumTaps - 1);

    for (n = 0; n < m_NumTaps; n++)
    {
        real_t  x = (real_t)n - (real_t)(m_NumTaps - 1) / 2.0;
        real_t  c;

        // create ideal Sinc() HP filter with normFcut
        if ((real_t)n == fCenter)
            c = 1.0 - 2.0 * normFcut;
        else
            c = MSIN(K_PI * x) / (K_PI * x) - MSIN(K_2PI * x * normFcut) / (K_PI*x);

        // create Kaiser window and multiply to get coefficient
        x = ((real_t)n - ((real_t)m_NumTaps - 1.0) / 2.0) /
            (((real_t)m_NumTaps - 1.0) / 2.0);
        m_Coef[n] = scale * c * Izero(Beta * MSQRT(1 - (x*x))) / izb;
    }

    // make a 2x length array for FIR flat calculation efficiency
    for (n = 0; n < m_NumTaps; n++)
        m_Coef[n + m_NumTaps] = m_Coef[n];

    // copy into complex coef buffers
    for (n = 0; n < m_NumTaps * 2; n++)
    {
        m_ICoef[n] = m_Coef[n];
        m_QCoef[n] = m_Coef[n];
    }

    // Initialize the FIR buffers and state
    for (n = 0; n < m_NumTaps; n++)
    {
        m_rZBuf[n] = 0.0;
        m_cZBuf[n].re = 0.0;
        m_cZBuf[n].im = 0.0;
    }
    m_State = 0;

    return m_NumTaps;
}

/*
 * Process num inbuf[] samples and place in outbuf[]
 *
 * Note the Coefficient array is twice the length and has a duplicated set
 * in order to eliminate testing for buffer wrap in the inner loop
 * e.g. if 3 tap FIR with coefficients{21,-43,15} is made into a array of 6entries
 *   {21, -43, 15, 21, -43, 15 }
 * REAL version
 */
void Fir::process(int num, real_t * inbuf, real_t * outbuf)
{
    real_t      acc;
    real_t     *Zptr;
    const real_t *Hptr;

    for (int i = 0; i < num; i++)
    {
        m_rZBuf[m_State] = inbuf[i];
        Hptr = &m_Coef[m_NumTaps - m_State];
        Zptr = m_rZBuf;
        acc = (*Hptr++ * *Zptr++);  // do the 1st MAC
        for (int j = 1; j < m_NumTaps; j++)
            acc += (*Hptr++ * *Zptr++); // do the remaining MACs
        if (--m_State < 0)
            m_State += m_NumTaps;
        outbuf[i] = acc;
    }
}

/*
 * Process num inbuf[] samples and place in outbuf[].
 * Note the Coefficient array is twice the length and has a duplicated set
 * in order to eliminate testing for buffer wrap in the inner loop
 * ex: if 3 tap FIR with coefficients{21,-43,15} is mae into a array of 6 entries
 *    {21, -43, 15, 21, -43, 15 }
 * REAL input COMPLEX output version (for Hilbert filter pair)
 */
void Fir::process(int num, real_t * inbuf, complex_t * outbuf)
{
    complex_t   acc;
    complex_t  *Zptr;
    real_t     *HIptr;
    real_t     *HQptr;

    for (int i = 0; i < num; i++)
    {
        m_cZBuf[m_State].re = inbuf[i];
        m_cZBuf[m_State].im = inbuf[i];
        HIptr = m_ICoef + m_NumTaps - m_State;
        HQptr = m_QCoef + m_NumTaps - m_State;
        Zptr = m_cZBuf;
        acc.re = (*HIptr++ * (*Zptr).re);       // do the first MAC
        acc.im = (*HQptr++ * (*Zptr++).im);
        for (int j = 1; j < m_NumTaps; j++)
        {
            acc.re += (*HIptr++ * (*Zptr).re);      // do the remaining MACs
            acc.im += (*HQptr++ * (*Zptr++).im);
        }
        if (--m_State < 0)
            m_State += m_NumTaps;
        outbuf[i] = acc;
    }
}

/*
 * Process num inbuf[] samples and place in outbuf[].
 * Note the Coefficient array is twice the length and has a duplicated set
 * in order to eliminate testing for buffer wrap in the inner loop
 * ex: if 3 tap FIR with coefficients{21,-43,15} is mae into a array of 6 entries
 *   {21, -43, 15, 21, -43, 15 }
 * COMPLEX version
 */
void Fir::process(int num, complex_t * inbuf, complex_t * outbuf)
{
    complex_t   acc;
    complex_t  *Zptr;
    real_t     *HIptr;
    real_t     *HQptr;

    for (int i = 0; i < num; i++)
    {
        m_cZBuf[m_State] = inbuf[i];
        HIptr = m_ICoef + m_NumTaps - m_State;
        HQptr = m_QCoef + m_NumTaps - m_State;
        Zptr = m_cZBuf;
        acc.re = (*HIptr++ * (*Zptr).re);       // do the first MAC
        acc.im = (*HQptr++ * (*Zptr++).im);
        for (int j = 1; j < m_NumTaps; j++)
        {
            acc.re += (*HIptr++ * (*Zptr).re);      // do the remaining MACs
            acc.im += (*HQptr++ * (*Zptr++).im);
        }
        if (--m_State < 0)
            m_State += m_NumTaps;
        outbuf[i] = acc;
    }
}

/*
 * Private helper function to Compute Modified Bessel function I0(x)
 *     using a series approximation.
 * I0(x) = 1.0 + { sum from k=1 to infinity ---->  [(x/2)^k / k!]^2 }
 */
real_t Fir::Izero(real_t x)
{
    real_t  x2 = x/2.0;
    real_t  sum = 1.0;
    real_t  ds = 1.0;
    real_t  di = 1.0;
    real_t  errorlimit = 1e-9;
    real_t  tmp;
    do
    {
        tmp = x2/di;
        tmp *= tmp;
        ds *= tmp;
        sum += ds;
        di += 1.0;
    } while (ds >= errorlimit * sum);

    return sum;
}

