/*
 * Common data type definitions shared across various parts of the program
 */
#pragma once

#include "common/datatypes.h"


// FIXME: Move to audio_codec or datatypes?
typedef enum _audio_codec {
    AUDIO_CODEC_NONE    = 0,    /* No codec, audio muted. */
    AUDIO_CODEC_RAW     = 1,    /* 8 bit 64 kbps raw samples (1 channel) */
    AUDIO_CODEC_G711    = 2,    /* 8 bit 64 kbps uLaw G711 */
    AUDIO_CODEC_G726_40 = 3,    /* 5 bit 40 kbps G.726 */
    AUDIO_CODEC_G726_32 = 4,    /* 4 bit 32 kbps G.726 */
    AUDIO_CODEC_G726_24 = 5,    /* 3 bit 24 kbps G.726 */
    AUDIO_CODEC_G726_16 = 6     /* 2 bit 16 kbps G.726 */
} audio_codec_t;

/* Gain stage identifiers */
#define SDR_GAIN_ID_RX_LNA         0
#define SDR_GAIN_ID_RX_MIX         1
#define SDR_GAIN_ID_RX_IF          2
#define SDR_GAIN_ID_RX_VGA         3
#define SDR_GAIN_ID_RX_LIN         4
#define SDR_GAIN_ID_RX_SENS        5
#define SDR_GAIN_ID_RX_RF_AGC      6
#define SDR_GAIN_ID_RX_IF_AGC      7
#define SDR_GAIN_ID_TX_PA          8
#define SDR_GAIN_ID_TX_MIX         9
#define SDR_GAIN_ID_TX_IF          10
#define SDR_GAIN_ID_TX_VGA         11
#define SDR_GAIN_ID_NUM           (SDR_GAIN_ID_TX_VGA + 1)

typedef struct _range {
    uint64_t        min;
    uint64_t        max;
    uint8_t         step;
} freq_range_t;

/* Server type (NB: Same values as in server/client ICD) */
typedef enum _srv_type {
    SRV_TYPE_RX_SU      = 0x01,     /* Single user RX */
    SRV_TYPE_RX_MU      = 0x02,     /* Multi user RX (fixed RF freq) */
    SRV_TYPE_TX         = 0x10,     /* Transmitter */
    SRV_TYPE_TRX_HDX    = 0x20,     /* Half-duplex transceiver */
    SRV_TYPE_TRX_FDX    = 0x21,     /* Full-duplex transceiver */
    SRV_TYPE_SA         = 0x30      /* Spectrum analyser */
} srv_type_t;

/* Server info */
typedef struct _srv_info {
    srv_type_t      type;           /* Server type. */
    uint64_t        freq_min;       /* Lower limit of RF frequency [Hz]. */
    uint64_t        freq_max;       /* Upper limit of RF frequency [Hz]. */
    uint64_t        span_min;       /* Minimum spectrum span, i.e. max zoom in [Hz]. */
    uint64_t        span_max;       /* Maximum spectrum span, i.e. max zoom out [Hz]. */
    uint32_t        gains;          /* Bit field of available gain stages. */
    uint16_t        if_version;     /* Server interface version (MSB.LSB). */
    uint16_t        hw_version;     /* Hardware version (MSB.LSB). */
    uint16_t        fw_version;     /* Firmware version (MSB.LSB). */
    uint8_t         antennas;       /* Number of antennas. */
    uint8_t         srv_name[64];   /* Server name and description (0 terminated). */
    uint8_t         dev_name[64];   /* SDR device name and description (0 terminated). */
} srv_info_t;

/**
 * Structure used to exchange FFT data.
 * 
 * @data    The FFT data
 * @size    The number of samples in data
 * @rate    The sample rate of the FFT data
 */
typedef struct _fft_data {
    complex_t      *data;
    unsigned int    size;
    float           rate;
} fft_data_t;

