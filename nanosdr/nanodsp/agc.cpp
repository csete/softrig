/*
 * This class implements the automatic gain control function.
 * Originally from CuteSdr.
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
 *
 */
#include <stdio.h>

#include "agc.h"

// signal delay line time delay in seconds.
// adjust to cover the impulse response time of filter
#define DELAY_TIMECONST         .015

// Peak Detector window time delay in seconds.
#define WINDOW_TIMECONST        .018

// attack time constant in seconds
// just small enough to let attackave charge up within the DELAY_TIMECONST time
#define ATTACK_RISE_TIMECONST   .002
#define ATTACK_FALL_TIMECONST   .005

// ratio between rise and fall times of Decay time constants
// adjust for best action with SSB
#define DECAY_RISEFALL_RATIO    .3

// hang timer release decay time constant in seconds
#define RELEASE_TIMECONST       .05

// limit output to about 3 dB of max
#define AGC_OUTSCALE 0.5

#define MAX_AMPLITUDE           1.0
#define LOG_MAX_AMP             MLOG10(MAX_AMPLITUDE)

#define MAX_MANUAL_AMPLITUDE    1.0

// constant for calculating log() so that a value of 0 magnitude == -8
// corresponding to -160 dB.
// K = 10^(-8 + log(MAX_AMP))
#define MIN_CONSTANT 1e-8

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAgc::CAgc()
{
    m_AgcOn = true;
    m_UseHang = false;
    m_Threshold = 0;
    m_ManualGain = 0;
    m_SlopeFactor = 0;
    m_Decay = 0;
    m_SampleRate = 0.0;
    m_Peak = 0.0;
    m_MagBufPos = 0;
}

CAgc::~CAgc()
{
}

void CAgc::setup(bool AgcOn,  bool UseHang, int Threshold, int ManualGain,
                 int SlopeFactor, int Decay, real_t SampleRate)
{
    if ((AgcOn == m_AgcOn) && (UseHang == m_UseHang) &&
        (Threshold == m_Threshold) && (ManualGain == m_ManualGain) &&
        (SlopeFactor == m_SlopeFactor) && (Decay == m_Decay) &&
        (SampleRate == m_SampleRate))
    {
        return;
    }

    m_AgcOn = AgcOn;
    m_UseHang = UseHang;
    m_Threshold = Threshold;
    m_ManualGain = ManualGain;
    m_SlopeFactor = SlopeFactor;
    m_Decay = Decay;

    if (m_SampleRate != SampleRate)
    {
        m_SampleRate = SampleRate;
        for (int i = 0; i < MAX_DELAY_BUF; i++)
        {
            m_SigDelayBuf[i].re = 0.0;
            m_SigDelayBuf[i].im = 0.0;
            m_MagBuf[i] = -16.0;
        }
        m_SigDelayPtr = 0;
        m_HangTimer = 0;
        m_Peak = -16.0;
        m_DecayAve = -5.0;
        m_AttackAve = -5.0;
        m_MagBufPos = 0;
    }

    m_ManualAgcGain = MAX_MANUAL_AMPLITUDE * MPOW(10.0, ((real_t)m_ManualGain) / 20.0);

    m_Knee = (real_t)m_Threshold / 20.0;
    m_GainSlope = m_SlopeFactor / 100.0;
    m_FixedGain = AGC_OUTSCALE * MPOW(10.0, m_Knee * (m_GainSlope - 1.0));

    // fast and slow filter values.
    m_AttackRiseAlpha = 1.0 - MEXP(-1.0 / (m_SampleRate * ATTACK_RISE_TIMECONST));
    m_AttackFallAlpha = 1.0 - MEXP(-1.0 / (m_SampleRate * ATTACK_FALL_TIMECONST));
    m_DecayRiseAlpha = 1.0 - MEXP(-1.0 / (m_SampleRate * (real_t)m_Decay *
                        .001*DECAY_RISEFALL_RATIO));

    m_HangTime = (int)(m_SampleRate * (real_t)m_Decay * .001);

    if (m_UseHang)
        m_DecayFallAlpha = 1.0 - MEXP(-1.0 / (m_SampleRate * RELEASE_TIMECONST));
    else
        m_DecayFallAlpha = 1.0 - MEXP(-1.0 / (m_SampleRate * (real_t)m_Decay * .001));

    m_DelaySamples = (int)(m_SampleRate * DELAY_TIMECONST);
    m_WindowSamples = (int)(m_SampleRate * WINDOW_TIMECONST);

    // clamp Delay samples within buffer limit
    // current buffer limit is good to ~220 ksps
    if (m_DelaySamples >= MAX_DELAY_BUF - 1)
    {
        m_DelaySamples = MAX_DELAY_BUF - 1;
        fprintf(stderr, "*** WARNING: AGC delay buff truncated to %d\n",
                m_DelaySamples + 1);
    }

    if (m_WindowSamples >= MAX_DELAY_BUF - 1)
    {
        m_WindowSamples = MAX_DELAY_BUF - 1;
        fprintf(stderr, "*** WARNING: AGC window buff tuncated to %d\n",
                m_WindowSamples + 1);
    }
}

void CAgc::process(int num, complex_t * inbuf, complex_t * outbuf)
{
    real_t      gain;
    real_t      mag = 0.f;
    complex_t   delayedin;

    if (m_AgcOn)
    {
        for (int i = 0; i < num; i++)
        {
            complex_t   in = inbuf[i];
            real_t      mim;
            real_t      tmp;

            // Get delayed sample of input signal
            delayedin = m_SigDelayBuf[m_SigDelayPtr];

            // put new input sample into signal delay buffer
            m_SigDelayBuf[m_SigDelayPtr++] = in;

            if (m_SigDelayPtr >= m_DelaySamples) // delay buffer wrap around
                m_SigDelayPtr = 0;

            mag = MFABS(in.re);
            mim = MFABS(in.im);
            if (mim > mag)
                mag = mim;

            mag = MLOG10(mag + MIN_CONSTANT) - LOG_MAX_AMP;

            // create a sliding window of 'm_WindowSamples' magnitudes and output the peak value within the sliding window
            tmp = m_MagBuf[m_MagBufPos];        // get oldest mag sample from buffer into tmp
            m_MagBuf[m_MagBufPos++] = mag;      // put latest mag sample in buffer;

            if (m_MagBufPos >= m_WindowSamples) // deal with magnitude buffer wrap around
                m_MagBufPos = 0;
            if (mag > m_Peak)
            {
                m_Peak = mag;   // if new sample is larger than current peak then use it, no need to look at buffer values
            }
            else
            {
                if (tmp == m_Peak)       // tmp is oldest sample pulled out of buffer
                {
                    int i;

                    // if oldest sample pulled out was last peak then need to find next highest peak in buffer
                    m_Peak = -8.0;      // set to lowest value to find next max peak

                    //search all buffer for maximum value and set as new peak
                    for (i = 0; i < m_WindowSamples; i++)
                    {
                        tmp = m_MagBuf[i];
                        if (tmp > m_Peak)
                            m_Peak = tmp;
                    }
                }
            }

            if (m_UseHang)
            {
                if (m_Peak > m_AttackAve)
                    // power is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                   m_AttackRiseAlpha * m_Peak;
                else
                    // magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                   m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                {
                    // magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                  m_DecayRiseAlpha*m_Peak;
                    m_HangTimer = 0;
                }
                else
                {
                    // decreasing signal
                    if (m_HangTimer < m_HangTime)
                        // increment and hold current m_DecayAve
                        m_HangTimer++;
                    else
                        // decay with m_DecayFallAlpha which is RELEASE_TIMECONST
                        m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                     m_DecayFallAlpha * m_Peak;
                }
            }
            else
            {
                // using exponential decay mode
                // perform average of magnitude using 2 averagers each with
                // separate rise and fall time constants
                if (m_Peak > m_AttackAve)
                    // magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                   m_AttackRiseAlpha*m_Peak;
                else
                    // magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                    // magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha * m_Peak;
                else
                    // magnitude is falling (use m_DecayFallAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                  m_DecayFallAlpha * m_Peak;
            }

            // use greater magnitude of attack or Decay Averager
            if (m_AttackAve > m_DecayAve)
                mag = m_AttackAve;
            else
                mag = m_DecayAve;

            // gain depends on which side of knee the magnitude is on
            if (mag <= m_Knee)
                gain = m_FixedGain;
            else
                gain = AGC_OUTSCALE * MPOW(10.0, mag * (m_GainSlope - 1.0));

            outbuf[i].re = delayedin.re * gain;
            outbuf[i].im = delayedin.im * gain;
        }
    }
    else
    {
        // using manual gain
        for (int i = 0; i < num; i++)
        {
            outbuf[i].re = m_ManualAgcGain * inbuf[i].re;
            outbuf[i].im = m_ManualAgcGain * inbuf[i].im;
        }
    }
}

void CAgc::process(int num, real_t * inbuf, real_t * outbuf)
{
    real_t      gain;
    real_t      mag = 0.f;
    real_t      delayedin;

    if (m_AgcOn)
    {
        for (int i = 0; i < num; i++)
        {
            real_t in = inbuf[i];

            // Get delayed sample of input signal
            delayedin = m_SigDelayBuf[m_SigDelayPtr].re;

            // put new input sample into signal delay buffer
            m_SigDelayBuf[m_SigDelayPtr++].re = in;
            if (m_SigDelayPtr >= m_DelaySamples)    // deal with delay buffer wrap around
                m_SigDelayPtr = 0;

            mag = MLOG10(mag + MIN_CONSTANT) - LOG_MAX_AMP;

            // create a sliding window of 'm_WindowSamples' magnitudes and output the peak value within the sliding window
            real_t  tmp = m_MagBuf[m_MagBufPos];    // get oldest mag sample from buffer into tmp
            m_MagBuf[m_MagBufPos++] = mag;          // put latest mag sample in buffer;

            if (m_MagBufPos >= m_WindowSamples)     // deal with magnitude buffer wrap around
                m_MagBufPos = 0;
            if (mag > m_Peak)
            {
                m_Peak = mag;   // new sample is larger than current peak then use it, no need to look at buffer values
            }
            else
            {
                if (tmp == m_Peak)  // tmp is oldest sample pulled out of buffer
                {
                    // if oldest sample pulled out was last peak then need to find next highest peak in buffer
                    m_Peak = -8.0;      // set to lowest value to find next max peak

                    // search all buffer for maximum value and set as new peak
                    for (int i = 0; i < m_WindowSamples; i++)
                    {
                        tmp = m_MagBuf[i];
                        if(tmp > m_Peak)
                            m_Peak = tmp;
                    }
                }
            }

            if (m_UseHang)
            {
                if (m_Peak > m_AttackAve)
                    // magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha*m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                {
                    // magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha*m_Peak;
                    m_HangTimer = 0;
                }
                else
                {
                    // decreasing signal
                    if (m_HangTimer < m_HangTime)
                        m_HangTimer++;  //just inc and hold current m_DecayAve
                    else
                        // decay with m_DecayFallAlpha which is RELEASE_TIMECONST
                        m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                     m_DecayFallAlpha*m_Peak;
                }
            }
            else
            {
                // using exponential decay mode
                // perform average of magnitude using 2 averagers each with
                // separate rise and fall time constants
                if (m_Peak > m_AttackAve)
                    // magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                    // magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha * m_Peak;
                else
                    // magnitude is falling (use m_DecayFallAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                 m_DecayFallAlpha * m_Peak;
            }

            // use greater magnitude of attack or Decay Averager
            if (m_AttackAve > m_DecayAve)
                mag = m_AttackAve;
            else
                mag = m_DecayAve;

            // calculate gain depending on which side of knee the magnitude is on
            if (mag <= m_Knee)
                // use fixed gain if below knee
                gain = m_FixedGain;
            else
                // use variable gain if above knee
                gain = AGC_OUTSCALE * MPOW(10.0, mag * (m_GainSlope - 1.0));

            outbuf[i] = delayedin * gain;
        }
    }
    else
    {
        // using manual gain mode
        for (int i = 0; i < num; i++)
            outbuf[i] = m_ManualAgcGain * inbuf[i];
    }
}
