/*
 * Airspy R2/Mini API definitions
 */
#pragma once

#include <stdint.h>

enum airspy_sample_type {
    AIRSPY_SAMPLE_FLOAT32_IQ = 0,   /* 2 * 32bit float per sample */
    AIRSPY_SAMPLE_FLOAT32_REAL = 1, /* 1 * 32bit float per sample */
    AIRSPY_SAMPLE_INT16_IQ = 2,     /* 2 * 16bit int per sample */
    AIRSPY_SAMPLE_INT16_REAL = 3,   /* 1 * 16bit int per sample */
    AIRSPY_SAMPLE_UINT16_REAL = 4,  /* 1 * 16bit unsigned int per sample */
    AIRSPY_SAMPLE_RAW = 5,          /* Raw packed samples from the device */
    AIRSPY_SAMPLE_END = 6           /* Number of supported sample types */
};

typedef struct {
    struct airspy_device *  device;
    void *                  ctx;
    void *                  samples;
    int                     sample_count;
    uint64_t                dropped_samples;
    enum airspy_sample_type sample_type;
} airspy_transfer_t, airspy_transfer;

typedef struct {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t revision;
} airspy_lib_version_t;
