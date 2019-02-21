/*
 * This class implements the automatic gain control function.
 *
 */
#pragma once

#include "common/datatypes.h"

#define MAX_DELAY_BUF 4096

class CAgc
{
public:
    CAgc();
    virtual    ~CAgc();

    /*
     * Setup AGC parameters.
     *
     * Parameters:
     *   AgcOn      AGC is ON or OFF.
     *   UseHang    Use a hang timer or gradual decay.
     *   Threshold  AGC "knee" in dB. Nominal range is -160 to 0 dB.
     *   ManualGain Manual gain in dB used when AGC is OFF. Nominal range is 0
     *              to 100 dB.
     *   Slope      Reduction in output (dB) at knee from maximum output level.
     *              Nominal range is 0 to 10 dB.
     *   Decay      AGC decay in milliseconds. Nominal range between 20 and 5000 ms.
     *   SampleRate The sample rate.
     */
    void        setup(bool AgcOn, bool UseHang, int Threshold, int ManualGain,
                      int Slope, int Decay, real_t SampleRate);

    void        process(int num, complex_t * inbuf, complex_t * outbuf);
    void        process(int num, real_t * inbuf, real_t * outbuf);

private:
    bool        m_AgcOn;
    bool        m_UseHang;
    int         m_Threshold;
    int         m_ManualGain;
    int         m_Slope;
    int         m_Decay;
    real_t      m_SampleRate;
    real_t      m_SlopeFactor;
    real_t      m_ManualAgcGain;
    real_t      m_DecayAve;
    real_t      m_AttackAve;
    real_t      m_AttackRiseAlpha;
    real_t      m_AttackFallAlpha;
    real_t      m_DecayRiseAlpha;
    real_t      m_DecayFallAlpha;
    real_t      m_FixedGain;
    real_t      m_Knee;
    real_t      m_GainSlope;
    real_t      m_Peak;

    int         m_SigDelayPtr;
    int         m_MagBufPos;
    int         m_DelaySize;
    int         m_DelaySamples;
    int         m_WindowSamples;
    int         m_HangTime;
    int         m_HangTimer;

    complex_t   m_SigDelayBuf[MAX_DELAY_BUF];
    real_t      m_MagBuf[MAX_DELAY_BUF];
};
