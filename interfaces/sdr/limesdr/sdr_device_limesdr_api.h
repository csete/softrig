/*
 * LimeSDR API
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "sdr_device_limesdr_api_defs.h"

#define API_VERISON     "19.04"
#define LMS_SUCCESS     0

#define LMS_CH_TX       true
#define LMS_CH_RX       false

static const char* (*LMS_GetLibraryVersion)(void);

static int (*LMS_GetDeviceList)(lms_info_str_t *dev_list);
static int (*LMS_Open)(lms_device_t **device, const lms_info_str_t info, void *args);
static int (*LMS_Close)(lms_device_t *device);
static int (*LMS_Init)(lms_device_t *device);
static int (*LMS_EnableChannel)(lms_device_t *device, bool dir_tx, size_t chan, bool enabled);
static int (*LMS_SetLOFrequency)(lms_device_t *device, bool dir_tx, size_t chan, float_type f);
static int (*LMS_SetSampleRate)(lms_device_t *device, float_type rate, size_t oversample);
static int (*LMS_SetLPF)(lms_device_t *device, bool dir_tx, size_t chan, bool enable);
static int (*LMS_SetLPFBW)(lms_device_t *device, bool dir_tx, size_t chan, float_type bandwidth);
static int (*LMS_SetGFIRLPF)(lms_device_t *device, bool dir_tx, size_t chan, bool enabled, float_type bandwidth);
static int (*LMS_SetNCOFrequency)(lms_device_t *device, bool dir_tx, size_t chan, const float_type *freq, float_type pho);
static int (*LMS_SetGaindB)(lms_device_t *device, bool dir_tx, size_t chan, unsigned gain);
static int (*LMS_GetGaindB)(lms_device_t *device, bool dir_tx, size_t chan, unsigned *gain);

static int (*LMS_SetupStream)(lms_device_t *device, lms_stream_t *stream);
static int (*LMS_DestroyStream)(lms_device_t *device, lms_stream_t *stream);
static int (*LMS_StartStream)(lms_stream_t *stream);
static int (*LMS_StopStream)(lms_stream_t *stream);
static int (*LMS_RecvStream)(lms_stream_t *stream, void *samples, size_t sample_count, lms_stream_meta_t *meta, unsigned timeout_ms);

static int (*LMS_Calibrate)(lms_device_t *device, bool dir_tx, size_t chan, double bw, unsigned flags);

static int (*LMS_GetNumChannels)(lms_device_t *device, bool dir_tx);
static int (*LMS_GetLOFrequencyRange)(lms_device_t *device, bool dir_tx, lms_range_t *range);
static int (*LMS_GetLPFBWRange)(lms_device_t *device, bool dir_tx, lms_range_t *range);
static int (*LMS_GetSampleRateRange)(lms_device_t *device, bool dir_tx, lms_range_t *range);
