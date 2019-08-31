/*
 * Airspy R2/Mini API
 */
#pragma once

#include <stdint.h>

#define AIRSPY_VER_MAJOR 1
#define AIRSPY_VER_MINOR 0

enum airspy_error {
    AIRSPY_SUCCESS = 0,
    AIRSPY_TRUE = 1,
    AIRSPY_ERROR_INVALID_PARAM = -2,
    AIRSPY_ERROR_NOT_FOUND = -5,
    AIRSPY_ERROR_BUSY = -6,
    AIRSPY_ERROR_NO_MEM = -11,
    AIRSPY_ERROR_LIBUSB = -1000,
    AIRSPY_ERROR_THREAD = -1001,
    AIRSPY_ERROR_STREAMING_THREAD_ERR = -1002,
    AIRSPY_ERROR_STREAMING_STOPPED = -1003,
    AIRSPY_ERROR_OTHER = -9999,
};

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

typedef int (*airspy_sample_block_cb_fn)(airspy_transfer *transfer);

static int (*airspy_open)(struct airspy_device **dev);
static int (*airspy_close)(void *dev);
static int (*airspy_set_samplerate)(void *dev, uint32_t samplerate);
static int (*airspy_set_conversion_filter_float32)(void *         dev,
                                                   const float *  kernel,
                                                   const uint32_t len);
static int (*airspy_start_rx)(void *dev, airspy_sample_block_cb_fn cb,
                              void *ctx);
static int (*airspy_stop_rx)(void *dev);
static int (*airspy_is_streaming)(void *dev);
static int (*airspy_set_sample_type)(void *dev,
                                     enum airspy_sample_type sample_type);
static int (*airspy_set_freq)(void *dev, const uint32_t freq);
static int (*airspy_set_linearity_gain)(void *dev, uint8_t gain);
static int (*airspy_set_sensitivity_gain)(void *dev, uint8_t gain);
static int (*airspy_set_lna_gain)(void *dev, uint8_t gain);
static int (*airspy_set_mixer_gain)(void *dev, uint8_t gain);
static int (*airspy_set_vga_gain)(void *dev, uint8_t gain);
static int (*airspy_set_lna_agc)(void *dev, uint8_t gain);
static int (*airspy_set_mixer_agc)(void *dev, uint8_t gain);
static int (*airspy_set_rf_bias)(void *dev, uint8_t bias);
static void (*airspy_lib_version)(airspy_lib_version_t *lib_version);
static char *(*airspy_error_name)(enum airspy_error errcode);
