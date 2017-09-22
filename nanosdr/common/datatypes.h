/*
 * Common data type declarations
 *
 * Copyright  2010-2013  Moe Wheatley AE4JY
 * Copyright  2014       Alexandru Csete OZ9AEC
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
#pragma once

#include <stdint.h>
#include <math.h>


// comment out to use double precision math
//#define USE_DOUBLE_PRECISION

typedef struct _scplx {
    float       re;
    float       im;
} scplx_t;

typedef struct _dcplx {
    double      re;
    double      im;
} dcplx_t;

typedef struct _iscplx {
    int16_t     re;
    int16_t     im;
} stereo16_t;

typedef stereo16_t  stereo_t;
typedef int16_t     mono_t;

// Definitions of real and complex data type
#ifdef USE_DOUBLE_PRECISION
    #define real_t      double
    #define complex_t   dcplx_t
#else
    #define real_t      float
    #define complex_t   scplx_t
#endif

#ifdef USE_DOUBLE_PRECISION
    #define MSIN(x) sin(x)
    #define MCOS(x) cos(x)
    #define MPOW(x,y) pow(x,y)
    #define MEXP(x) exp(x)
    #define MFABS(x) fabs(x)
    #define MLOG(x) log(x)
    #define MLOG10(x) log10(x)
    #define MSQRT(x) sqrt(x)
    #define MATAN(x) atan(x)
    #define MFMOD(x,y) fmod(x,y)
    #define MATAN2(x,y) atan2(x,y)
#else
    #define MSIN(x) sinf(x)
    #define MCOS(x) cosf(x)
    #define MPOW(x,y) powf(x,y)
    #define MEXP(x) expf(x)
    #define MFABS(x) fabsf(x)
    #define MLOG(x) logf(x)
    #define MLOG10(x) log10f(x)
    #define MSQRT(x) sqrtf(x)
    #define MATAN(x) atanf(x)
    #define MFMOD(x,y) fmodf(x,y)
    #define MATAN2(x,y) atan2f(x,y)
#endif

// FIXME for single precision?
#define K_2PI (2.0 * 3.14159265358979323846)
#define K_PI (3.14159265358979323846)
#define K_PI4 (K_PI/4.0)
#define K_PI2 (K_PI/2.0)
#define K_3PI4 (3.0*K_PI4)
