/*
 * Decimate by power-of-2 using half band filters.
 *
 * Originally from CuteSdr and modified for nanosdr.
 *
 * Copyright 2010  Moe Wheatley AE4JY
 * Copyright 2015  Alexandru Csete OZ9AEC
 * All rights reserved.
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
 *
 */
#include <stdio.h>

#include "common/bithacks.h"
#include "common/datatypes.h"
#include "decimator.h"
#include "filtercoef_hbf_70.h"
#include "filtercoef_hbf_100.h"
#include "filtercoef_hbf_140.h"

#define MAX_HALF_BAND_BUFSIZE 32768

Decimator::Decimator()
{
    int         i;

    decim = 0;
    atten = 0;

    for (i = 0; i < MAX_STAGES; i++)
        filter_table[i] = 0;
}

Decimator::~Decimator()
{
    delete_filters();
}

unsigned int Decimator::init(unsigned int _decim, unsigned int _att)
{
    if (_decim == decim && _att == atten)
        return _decim;

    if (_decim < 2 || _decim > MAX_DECIMATION || !is_power_of_two(_decim))
        return 1;

    delete_filters();

    atten = _att;

    if (atten <= 70)
    {
        decim = init_filters_70(_decim);
    }
    else if (atten <= 100)
    {
        decim = init_filters_100(_decim);
    }
    else
    {
        decim = init_filters_140(_decim);
    }

    return decim;
}

int Decimator::process(int num, complex_t * samples)
{
    int         i = 0;
    int         n = num;

    while(filter_table[i])
        n = filter_table[i++]->DecBy2(n, samples, samples);

    return n;
}

void Decimator::delete_filters()
{
    int         i;

    for(i = 0; i < MAX_STAGES; i++)
    {
        if (filter_table[i])
        {
            delete filter_table[i];
            filter_table[i] = 0;
        }
    }
}

int Decimator::init_filters_70(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
/*     7-tap filters are not quite sufficient
        if (decimation >= 8)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_70_7_LENGTH, HBF_70_7);
            fprintf(stderr, "  DEC %d: HBF_70_7\n", n);
        }
        else
*/
        if (decimation >= 4)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_70_11);
            fprintf(stderr, "  DEC %d: HBF_70_11 (unrolled)\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_70_39_LENGTH, HBF_70_39);
            fprintf(stderr, "  DEC %d: HBF_70_39\n", n);
        }

        decimation /= 2;
    }

    return (1 << n);
}

int Decimator::init_filters_100(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
/*   7-tap filters are not quite sufficient
        if (decimation >= 16)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_100_7_LENGTH, HBF_100_7);
            fprintf(stderr, "  DEC %d: HBF_100_7\n", n);
        }
        else
*/
        if (decimation >= 8)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_100_11);
            fprintf(stderr, "  DEC %d: HBF_100_11 (unrolled)\n", n);
        }
        else if (decimation == 4)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_100_19_LENGTH, HBF_100_19);
            fprintf(stderr, "  DEC %d: HBF_100_19\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_100_59_LENGTH, HBF_100_59);
            fprintf(stderr, "  DEC %d: HBF_100_59\n", n);
        }

        decimation /= 2;
    }
    
    return (1 << n);
}

int Decimator::init_filters_140(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
        if (decimation >= 16)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_140_11);
            fprintf(stderr, "  DEC %d: HBF_140_11 (unrolled)\n", n);
        }
        else if (decimation == 8)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_15_LENGTH, HBF_140_15);
            fprintf(stderr, "  DEC %d: HBF_140_15\n", n);
        }
        else if (decimation == 4)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_27_LENGTH, HBF_140_27);
            fprintf(stderr, "  DEC %d: HBF_140_27\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_87_LENGTH, HBF_140_87);
            fprintf(stderr, "  DEC %d: HBF_140_87\n", n);
        }

        decimation /= 2;
    }

    return (1 << n);
}


Decimator::CHalfBandDecimateBy2::CHalfBandDecimateBy2(int len, const real_t * pCoef)
    : m_FirLength(len), m_pCoef(pCoef)
{
    complex_t   CPXZERO = {0.0, 0.0};
    int i;

    // create buffer for FIR implementation
    m_pHBFirBuf = new complex_t[MAX_HALF_BAND_BUFSIZE];
    for (i = 0; i < MAX_HALF_BAND_BUFSIZE; i++)
        m_pHBFirBuf[i] = CPXZERO;
}

int Decimator::CHalfBandDecimateBy2::DecBy2(int InLength, complex_t* pInData, complex_t* pOutData)
{
    int     numoutsamples = 0;
    int     i;
    int     j;

    // input length must be even and greater than or equal the number of taps
    if (InLength < m_FirLength)
        return InLength / 2;     // FIXME: What to do?

    // copy input samples into buffer starting at position m_FirLength-1
    for (i = 0, j = m_FirLength - 1; i < InLength; i++)
        m_pHBFirBuf[j++] = pInData[i];

    // perform decimation FIR filter on even samples
    for (i = 0; i < InLength; i += 2)
    {
        complex_t   acc;

        acc.re = (m_pHBFirBuf[i].re * m_pCoef[0]);
        acc.im = (m_pHBFirBuf[i].im * m_pCoef[0]);
        for (j = 0; j < m_FirLength; j += 2)
        {
            acc.re += (m_pHBFirBuf[i+j].re * m_pCoef[j]);
            acc.im += (m_pHBFirBuf[i+j].im * m_pCoef[j]);
        }

        // center coefficient
        acc.re += (m_pHBFirBuf[i+(m_FirLength-1)/2].re * m_pCoef[(m_FirLength-1)/2]);
        acc.im += (m_pHBFirBuf[i+(m_FirLength-1)/2].im * m_pCoef[(m_FirLength-1)/2]);
        pOutData[numoutsamples++] = acc;

    }

    // need to copy last m_FirLength-1 input samples in buffer to beginning of
    // buffer for FIR wrap around management
    for (i = 0,j = InLength - m_FirLength + 1; i < m_FirLength - 1; i++)
        m_pHBFirBuf[i] = pInData[j++];

    return numoutsamples;
}


Decimator::CHalfBand11TapDecimateBy2::CHalfBand11TapDecimateBy2(const real_t * coef)
{
    complex_t   CPXZERO = {0.0,0.0};

    H0 = coef[0];
    H2 = coef[2];
    H4 = coef[4];
    H5 = coef[5];
    H6 = coef[6];
    H8 = coef[8];
    H10 = coef[10];

    d0 = CPXZERO;
    d1 = CPXZERO;
    d2 = CPXZERO;
    d3 = CPXZERO;
    d4 = CPXZERO;
    d5 = CPXZERO;
    d6 = CPXZERO;
    d7 = CPXZERO;
    d8 = CPXZERO;
    d9 = CPXZERO;
}

//////////////////////////////////////////////////////////////////////
//Decimate by 2 Fixed 11 Tap Halfband filter class implementation
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps(11)
// InLength must be an even number
// Loop unrolled for speed ~15nS/samp
//////////////////////////////////////////////////////////////////////
int Decimator::CHalfBand11TapDecimateBy2::DecBy2(int InLength,
                                                 complex_t * pInData,
                                                 complex_t * pOutData)
{
    int i;

    complex_t   tmpout[9]; // allows using the same buffer for oIn and pOut

    tmpout[0].re = H0 * d0.re + H2 * d2.re + H4 * d4.re + H5 * d5.re
                + H6 * d6.re + H8 * d8.re + H10 * pInData[0].re;
    tmpout[0].im = H0 * d0.im + H2 * d2.im + H4 * d4.im + H5 * d5.im
                + H6 * d6.im + H8 * d8.im + H10 * pInData[0].im;

    tmpout[1].re = H0 * d2.re + H2 * d4.re + H4 * d6.re + H5 * d7.re
                + H6 * d8.re + H8 * pInData[0].re + H10 * pInData[2].re;
    tmpout[1].im = H0 * d2.im + H2 * d4.im + H4 * d6.im + H5 * d7.im
                + H6 * d8.im + H8 * pInData[0].im + H10 * pInData[2].im;

    tmpout[2].re = H0 * d4.re + H2 * d6.re + H4 * d8.re + H5 * d9.re
                + H6 * pInData[0].re + H8 * pInData[2].re + H10 * pInData[4].re;
    tmpout[2].im = H0 * d4.im + H2 * d6.im + H4 * d8.im + H5 * d9.im
                + H6 * pInData[0].im + H8 * pInData[2].im + H10 * pInData[4].im;

    tmpout[3].re = H0 * d6.re + H2 * d8.re + H4 * pInData[0].re
                + H5 * pInData[1].re + H6 * pInData[2].re + H8 * pInData[4].re
                + H10 * pInData[6].re;
    tmpout[3].im = H0 * d6.im + H2 * d8.im + H4 * pInData[0].im
                + H5 * pInData[1].im + H6 * pInData[2].im + H8 * pInData[4].im
                + H10*pInData[6].im;

    tmpout[4].re = H0 * d8.re + H2 * pInData[0].re + H4 * pInData[2].re
                + H5 * pInData[3].re + H6 * pInData[4].re + H8 * pInData[6].re
                + H10 * pInData[8].re;
    tmpout[4].im = H0 * d8.im + H2 * pInData[0].im + H4 * pInData[2].im
                + H5 * pInData[3].im + H6 * pInData[4].im + H8 * pInData[6].im
                + H10 * pInData[8].im;

    tmpout[5].re = H0 * pInData[0].re + H2 * pInData[2].re + H4 * pInData[4].re
                + H5 * pInData[5].re + H6 * pInData[6].re + H8 * pInData[8].re
                + H10 * pInData[10].re;
    tmpout[5].im = H0 * pInData[0].im + H2 * pInData[2].im + H4 * pInData[4].im
                + H5 * pInData[5].im + H6 * pInData[6].im + H8 * pInData[8].im
                + H10 * pInData[10].im;

    tmpout[6].re = H0 * pInData[2].re + H2 * pInData[4].re + H4 * pInData[6].re
                + H5 * pInData[7].re + H6 * pInData[8].re + H8 * pInData[10].re
                + H10 * pInData[12].re;
    tmpout[6].im = H0 * pInData[2].im + H2 * pInData[4].im + H4 * pInData[6].im
                + H5 * pInData[7].im + H6 * pInData[8].im + H8 * pInData[10].im
                + H10 * pInData[12].im;

    tmpout[7].re = H0 * pInData[4].re + H2 * pInData[6].re + H4 * pInData[8].re
                + H5 * pInData[9].re + H6 * pInData[10].re + H8 * pInData[12].re
                + H10 * pInData[14].re;
    tmpout[7].im = H0 * pInData[4].im + H2 * pInData[6].im + H4 * pInData[8].im
                + H5 * pInData[9].im + H6 * pInData[10].im + H8 * pInData[12].im
                + H10 * pInData[14].im;

    tmpout[8].re = H0 * pInData[6].re + H2 * pInData[8].re + H4 * pInData[10].re
                + H5 * pInData[11].re + H6 * pInData[12].re
                + H8 * pInData[14].re + H10 * pInData[16].re;
    tmpout[8].im = H0 * pInData[6].im + H2 * pInData[8].im + H4 * pInData[10].im
                + H5 * pInData[11].im + H6 * pInData[12].im
                + H8 * pInData[14].im + H10 * pInData[16].im;

    // remaining input samples
    complex_t   *pIn = &pInData[8];
    complex_t   *pOut = &pOutData[9];
    for (i = 0; i < (InLength-11-6 )/2; i++)
    {
        (*pOut).re = H0 * pIn[0].re + H2 * pIn[2].re + H4 * pIn[4].re
                + H5 * pIn[5].re + H6 * pIn[6].re + H8 * pIn[8].re
                + H10 * pIn[10].re;
        (*pOut++).im = H0 * pIn[0].im + H2 * pIn[2].im + H4 * pIn[4].im
                + H5 * pIn[5].im + H6 * pIn[6].im + H8 * pIn[8].im
                + H10 * pIn[10].im;
        pIn += 2;
    }

    pOutData[0] = tmpout[0];
    pOutData[1] = tmpout[1];
    pOutData[2] = tmpout[2];
    pOutData[3] = tmpout[3];
    pOutData[4] = tmpout[4];
    pOutData[5] = tmpout[5];
    pOutData[6] = tmpout[6];
    pOutData[7] = tmpout[7];
    pOutData[8] = tmpout[8];

    // update delay buffer
    pIn = &pInData[InLength-1];
    d9 = *pIn--;
    d8 = *pIn--;
    d7 = *pIn--;
    d6 = *pIn--;
    d5 = *pIn--;
    d4 = *pIn--;
    d3 = *pIn--;
    d2 = *pIn--;
    d1 = *pIn--;
    d0 = *pIn;

    return InLength / 2;
}
