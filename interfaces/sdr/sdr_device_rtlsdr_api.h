/*
 * RTL-SDR API
 */
#pragma once

#include <stdint.h>

enum rtlsdr_tuner {
    RTLSDR_TUNER_UNKNOWN = 0,
    RTLSDR_TUNER_E4000,
    RTLSDR_TUNER_FC0012,
    RTLSDR_TUNER_FC0013,
    RTLSDR_TUNER_FC2580,
    RTLSDR_TUNER_R820T,
    RTLSDR_TUNER_R828D
};

enum ds_channel {
    DS_CHANNEL_NONE = 0,
    DS_CHANNEL_I = 1,
    DS_CHANNEL_Q = 2
};

typedef void (*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len,
                                       void *ctx);

static int (*rtlsdr_open)(void **dev, uint32_t index);
static int (*rtlsdr_close)(void *dev);
static int (*rtlsdr_set_sample_rate)(void *dev, uint32_t rate);
static uint32_t (*rtlsdr_get_sample_rate)(void *dev);
static int (*rtlsdr_set_tuner_bandwidth)(void *dev, uint32_t bw);
static int (*rtlsdr_set_center_freq)(void *dev, uint32_t freq);
static uint32_t (*rtlsdr_get_center_freq)(void *dev);
static int (*rtlsdr_set_freq_correction)(void *dev, int ppm);
static enum rtlsdr_tuner (*rtlsdr_get_tuner_type)(void *dev);
static int (*rtlsdr_set_agc_mode)(void *dev, int on);
static int (*rtlsdr_set_tuner_gain)(void *dev, int gain);
static int (*rtlsdr_set_tuner_gain_mode)(void *dev, int manual);
static int (*rtlsdr_get_tuner_gains)(void *dev, int *gains);
static int (*rtlsdr_get_tuner_gain)(void *dev);
static int (*rtlsdr_set_direct_sampling)(void *dev, int on);
static int (*rtlsdr_get_direct_sampling)(void *dev);
static int (*rtlsdr_set_bias_tee)(void *dev, int on);
static int (*rtlsdr_cancel_async)(void *dev);
static int (*rtlsdr_reset_buffer)(void *dev);
static int (*rtlsdr_read_async)(void *dev, rtlsdr_read_async_cb_t cb, void *ctx,
                                uint32_t buf_num, uint32_t buf_len);
