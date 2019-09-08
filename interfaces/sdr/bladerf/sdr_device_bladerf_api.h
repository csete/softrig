/*
 * BladeRF API
 */
#pragma once

#include <stdlib.h>

#include "sdr_device_bladerf_api_defs.h"

#define LIBBLADERF_API_VERSION (0x02020100)

static int  (*bladerf_open)(struct bladerf **device, const char *device_identifier);
static void (*bladerf_close)(struct bladerf *device);
static void (*bladerf_set_usb_reset_on_open)(bool enabled);

static const char *(*bladerf_get_board_name)(struct bladerf *dev);
static bladerf_dev_speed (*bladerf_device_speed)(struct bladerf *dev);
static int (*bladerf_get_serial)(struct bladerf *dev, char *serial);
static int (*bladerf_fw_version)(struct bladerf *dev, struct bladerf_version *version);
static int (*bladerf_is_fpga_configured)(struct bladerf *dev);
static int (*bladerf_get_fpga_size)(struct bladerf *dev, bladerf_fpga_size *size);
static int (*bladerf_fpga_version)(struct bladerf *dev, struct bladerf_version *version);

//static size_t (*bladerf_get_channel_count)(struct bladerf *dev, bladerf_direction dir);

static int (*bladerf_set_frequency)(struct bladerf *dev, bladerf_channel ch, bladerf_frequency frequency);
static int (*bladerf_set_sample_rate)(struct bladerf *dev, bladerf_channel ch, bladerf_sample_rate rate, bladerf_sample_rate *actual);
static int (*bladerf_set_bandwidth)(struct bladerf *dev, bladerf_channel ch, bladerf_bandwidth bandwidth, bladerf_bandwidth *actual);

static int (*bladerf_set_gain)(struct bladerf *dev, bladerf_channel ch, bladerf_gain gain);
//static int (*bladerf_get_gain)(struct bladerf *dev, bladerf_channel ch, bladerf_gain *gain);
static int (*bladerf_set_gain_mode)(struct bladerf *dev, bladerf_channel ch, bladerf_gain_mode mode);
//static int (*bladerf_get_gain_mode)(struct bladerf *dev, bladerf_channel ch, bladerf_gain_mode *mode);
static int (*bladerf_get_gain_range)(struct bladerf *dev, bladerf_channel ch, const struct bladerf_range **range);

static int (*bladerf_enable_module)(struct bladerf *dev, bladerf_channel ch, bool enable);
static int (*bladerf_sync_config)(struct bladerf *dev, bladerf_channel_layout layout, bladerf_format format, unsigned int num_buffers, unsigned int buffer_size, unsigned int num_transfers, unsigned int stream_timeout);
static int (*bladerf_sync_rx)(struct bladerf *dev, void *samples, unsigned int num_samples, struct bladerf_metadata *metadata, unsigned int timeout_ms);

static void (*bladerf_log_set_verbosity)(bladerf_log_level level);
static void (*bladerf_version)(struct bladerf_version *version);
static const char *(*bladerf_strerror)(int error);

// BladeRF2 specific
static int (*bladerf_set_bias_tee)(struct bladerf *dev, bladerf_channel ch, bool enable);
