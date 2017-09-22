/*
 * Half-band filter coefficients with 70 dB stop band attenuation.
 *
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
#pragma once

#include "common/datatypes.h"

// Normalized 70 dB alias free bandwidths
#define HBF_70_11_BW     0.2f
#define HBF_70_39_BW     0.4f

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 11
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Alias free BW     : 0.2
 * Passband ripple   : +/- 3.2e-3 dB
 * Stop band         : -70 dB
 */
#define HBF_70_11_LENGTH     11
const real_t HBF_70_11[HBF_70_11_LENGTH] =
{
	0.009707733567516,
	0.0,
	-0.05811715559409,
	0.0,
	0.2985919803575,
	0.5,
	0.2985919803575,
	0.0,
	-0.05811715559409,
	0.0,
	0.009707733567516
};

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 39
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Alias free BW     : 0.4
 * Passband ripple   : +/- 3e-3 dB
 * Stop band         : -70 dB
 */
#define HBF_70_39_LENGTH     39
const real_t HBF_70_39[HBF_70_39_LENGTH] =
{
    -0.0006388614035059,
    0.0,
    0.001631195589637,
    0.0,
    -0.003550156604839,
    0.0,
    0.006773869396241,
    0.0,
    -0.01188293607946,
    0.0,
    0.01978182909123,
    0.0,
    -0.03220528568021,
    0.0,
    0.05351179043142,
    0.0,
    -0.09972534459278,
    0.0,
    0.3161340967929,
    0.5,
    0.3161340967929,
    0.0,
    -0.09972534459278,
    0.0,
    0.05351179043142,
    0.0,
    -0.03220528568021,
    0.0,
    0.01978182909123,
    0.0,
    -0.01188293607946,
    0.0,
    0.006773869396241,
    0.0,
    -0.003550156604839,
    0.0,
    0.001631195589637,
    0.0,
    -0.0006388614035059
};
