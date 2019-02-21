/*
 * Common data type declarations
 */
#pragma once

#include <math.h>
#include <stdint.h>

// efine USE_DOUBLE

typedef struct _scplx {
    float re;
    float im;
} scplx_t;

typedef struct _dcplx {
    double re;
    double im;
} dcplx_t;

typedef struct _iscplx {
    int16_t re;
    int16_t im;
} stereo16_t;

typedef stereo16_t stereo_t;
typedef int16_t    mono_t;

// Definitions of real and complex data type
/* clang-format off */
#ifdef USE_DOUBLE
#define real_t    double
#define complex_t dcplx_t
#else
#define real_t float
#define complex_t scplx_t
#endif

#ifdef USE_DOUBLE
#define MSIN(x)      sin(x)
#define MCOS(x)      cos(x)
#define MPOW(x, y)   pow(x, y)
#define MEXP(x)      exp(x)
#define MFABS(x)     fabs(x)
#define MLOG(x)      log(x)
#define MLOG10(x)    log10(x)
#define MSQRT(x)     sqrt(x)
#define MATAN(x)     atan(x)
#define MFMOD(x, y)  fmod(x, y)
#define MATAN2(x, y) atan2(x, y)
#else
#define MSIN(x)      sinf(x)
#define MCOS(x)      cosf(x)
#define MPOW(x, y)   powf(x, y)
#define MEXP(x)      expf(x)
#define MFABS(x)     fabsf(x)
#define MLOG(x)      logf(x)
#define MLOG10(x)    log10f(x)
#define MSQRT(x)     sqrtf(x)
#define MATAN(x)     atanf(x)
#define MFMOD(x, y)  fmodf(x, y)
#define MATAN2(x, y) atan2f(x, y)
#endif

// FIXME for single precision?
#define K_2PI  (2.0 * 3.14159265358979323846)
#define K_PI   (3.14159265358979323846)
#define K_PI4  (K_PI / 4.0)
#define K_PI2  (K_PI / 2.0)
#define K_3PI4 (3.0 * K_PI4)
/* clang-format on */
