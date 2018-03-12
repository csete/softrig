/*
 * Decimate by power-of-2 using half band filters.
 */
#pragma once

#define MAX_DECIMATION          512
#define MAX_STAGES              9

#include "common/datatypes.h"

class Decimator
{
public:
    Decimator();
    virtual    ~Decimator();

    /*
     * Initialise the decimator.
     * _decim is the decimation factor. Must be power of 2 and less than 512.
     * _att is the desired stop band attenuation in dB.
     *
     * Returns The actual decimation.
     *
     * Given the decimation and desired stop band attenuation, this function
     * will construct a chain of half band filters that will be used for
     * decimation.
     */
    unsigned int    init(unsigned int _decim, unsigned int _att);
    int             process(int num, complex_t * samples);

private:

    /* Abstract base class for decimate-by-2 stages */
    class CDec2
    {
    public:
        virtual ~CDec2() {}
        virtual int DecBy2(int InLength, complex_t* pInData, complex_t* pOutData) = 0;
    };

    /* Generic decimate-by-2 implementation using half band filters */
    class CHalfBandDecimateBy2 : public CDec2
    {
    public:
        CHalfBandDecimateBy2(int len, const real_t* pCoef);
        ~CHalfBandDecimateBy2()
        {
            if (m_pHBFirBuf)
                delete m_pHBFirBuf;
        }

        int     DecBy2(int InLength, complex_t * pInData, complex_t * pOutData);

        complex_t      *m_pHBFirBuf;
        int             m_FirLength;
        const real_t   *m_pCoef;
    };

    /* Decimate-by-2 implementation using 11-tap half band filter */
    class CHalfBand11TapDecimateBy2 : public CDec2
    {
    public:
        CHalfBand11TapDecimateBy2(const real_t * coef);
        ~CHalfBand11TapDecimateBy2() {}
        int DecBy2(int InLength, complex_t * pInData, complex_t * pOutData);

        // coefficients
        real_t      H0, H2, H4, H5, H6, H8, H10;

        // delay buffer
        complex_t   d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    };

private:
    int         init_filters_70(unsigned int decimation);
    int         init_filters_100(unsigned int decimation);
    int         init_filters_140(unsigned int decimation);
    void        delete_filters();
    CDec2      *filter_table[MAX_STAGES];

    unsigned int        atten;
    unsigned int        decim;
};
