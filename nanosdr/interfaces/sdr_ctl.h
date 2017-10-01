/*
 * SDR control commands and messages
 * These data structures define the commands used internaly in the program.
 * Every remote control protocol is mapped over into these commands.
 */
#pragma once

#include <stdint.h>
//#include "common/datatypes.h"
#include "common/sdr_data.h"

// Command IDs
//   0x0000 ... 0x00FF  High level commands that may affect both device and DSP
#define SDR_CTL_NONE                0x0000
#define SDR_CTL_SRV_INFO            0x0001
#define SDR_CTL_SRV_STATE           0x0005

//   0x0100 ... 0x03FF  Commands for SDR device (or driver)
#define SDR_CTL_RX_FREQ             0x0110
#define SDR_CTL_GAIN                0x0120

//   0x0400 ... 0x07FF  DSP comands
#define SDR_CTL_RX_DEMOD            0x0428
#define SDR_CTL_RX_AGC              0x0450
#define SDR_CTL_RX_FILTER           0x0458
#define SDR_CTL_RX_SQL              0x0480
#define SDR_CTL_RX_CODEC            0x0484


// Command types
#define SDR_CTL_TYPE_PING           0x00
#define SDR_CTL_TYPE_SET            0x01        // set 
#define SDR_CTL_TYPE_GET            0x02        // get
#define SDR_CTL_TYPE_GET_RSP        0x03        // response to get
#define SDR_CTL_TYPE_GET_RNG        0x04        // get range
#define SDR_CTL_TYPE_GET_RNG_RSP    0x05        // response to get range
#define SDR_CTL_TYPE_INVALID        0xFF

// Server state flags
#define SRV_STATE_IDLE              0x00
#define SRV_STATE_FLAG_RUNNING      0x01        // server is running
#define SRV_STATE_FLAG_RX           0x02        // server is receiving
#define SRV_STATE_FLAG_TX           0x04        // server is transmitting
#define SRV_STATE_FLAG_ERROR        0x80

// Gain stage identifiers
// FIXME: Use SDR_GAIN_ID directly!
#define SDR_CTL_GAIN_RX_LNA         SDR_GAIN_ID_RX_LNA
#define SDR_CTL_GAIN_RX_MIX         SDR_GAIN_ID_RX_MIX
#define SDR_CTL_GAIN_RX_IF          SDR_GAIN_ID_RX_IF
#define SDR_CTL_GAIN_RX_VGA         SDR_GAIN_ID_RX_VGA
#define SDR_CTL_GAIN_RX_LIN         SDR_GAIN_ID_RX_LIN
#define SDR_CTL_GAIN_RX_SENS        SDR_GAIN_ID_RX_SENS
#define SDR_CTL_GAIN_RX_RF_AGC      SDR_GAIN_ID_RX_RF_AGC
#define SDR_CTL_GAIN_RX_IF_AGC      SDR_GAIN_ID_RX_IF_AGC
#define SDR_CTL_GAIN_TX_PA          SDR_GAIN_ID_TX_PA
#define SDR_CTL_GAIN_TX_MIX         SDR_GAIN_ID_TX_MIX
#define SDR_CTL_GAIN_TX_IF          SDR_GAIN_ID_TX_IF
#define SDR_CTL_GAIN_TX_VGA         SDR_GAIN_ID_TX_VGA
#define SDR_CTL_GAIN_NUM            SDR_GAIN_ID_NUM


/**
 * AGC settings
 *
 * @todo    use_hang, manual gain and other parameters
 */
typedef struct _ctl_agc {
    int8_t          threshold;  /**< AGC threshold between -127 and 0 dB. */
    uint8_t         slope;      /**< AGC slope between 0 and 10 dB. */
    uint16_t        decay;      /**< AGC decay time between 20 and 2000 ms. */
} ctl_agc_t;

/**
 * Channel filter settings
 *
 * @fixme   We probably want to use 32 bit here
 * @fixme   CW offset should be separate!
 * @todo    Add transition width
 */
typedef struct _ctl_filter {
    int16_t         lo_cut;     /**< Low cut-off frequency in Hz. */
    int16_t         hi_cut;     /**< High cut-off frequency in Hz. */
    int16_t         offset;     /**< Frequency ofset. */
} ctl_filter_t;

/** Gain setting. */
typedef struct _gain {
    uint8_t         id;         // See defines in sdr_device.h
    uint8_t         value;      // 0...100
} gain_t;

/** SDR CTL data structure. */
typedef struct _sdr_ctl {
    uint8_t         type;       /**< The CTL type, see SDR_CTL_TYPE_* */
    uint16_t        id;         /**< The CTL ID, see SDR_CTL_* */

    union {
        uint8_t         srv_state;
        gain_t          gain;
        uint64_t        freq;
        sdr_demod_t     mode;       // AM, FM, SSB, ...
        ctl_agc_t       agc;        // AGC settings
        ctl_filter_t    filter;     // Channel filter settings
        int16_t         sql;        // Squelch threshold in dB
        audio_codec_t   codec;      // Audio codec

        /* ranges */
        freq_range_t    freq_range;
    };
} sdr_ctl_t;

