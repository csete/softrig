/*
 * Nanosdr network protocol.
 *
 * Copyright (c) 2015-2017  Alexandru Csete OZ9AEC
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
 */
#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "common/sdr_data.h"
#include "sdr_ctl.h"

#define MAX_PKT_LENGTH          65536
#define MAX_CTL_LENGTH          256     // TBC

// Packet types
#define PKT_TYPE_PING           0x00
#define PKT_TYPE_SET            0x01
#define PKT_TYPE_GET            0x02
#define PKT_TYPE_GET_RSP        0x03
#define PKT_TYPE_GET_RNG        0x04
#define PKT_TYPE_GET_RNG_RSP    0x05
#define PKT_TYPE_AUDIO          0x10
#define PKT_TYPE_FFT            0x20
#define PKT_TYPE_INVALID        0xFF


#ifdef __cplusplus
extern "C"
{
#endif

/** Packet representation */
typedef struct {
    uint16_t    length;         /* Packet length in bytes */
    uint8_t     type;           /* Packet type (PKT_TYPE_... */
    uint8_t     raw[MAX_PKT_LENGTH]; /* raw bytes */
} pkt_t;

/**
 * Read packet from file descriptor
 *
 * @param fd The file desriptor to read from
 * @param packet Preallocated packet structure
 * @returns Number of bytes read, 0 in case of EOF or -1 if an error occurred
 *          during the read
 */
int read_packet_from_fd(int fd, pkt_t * packet);

/**
 * Write packet to file descriptor
 *
 * @param fd The file descriptor to write to
 * @param packet The packet to write
 * @return The number of bytes written or -1 if an error occurred
 */
int write_packet_to_fd(int fd, const pkt_t * packet);

/**
 * Convert server info to raw packet.
 *
 * @param info Pointer to the server info structure.
 * @param data Pointer to the packet buffer.
 * @param max_count  The maximum number of bytes that can be written to data.
 * @return The number of bytes written or 0 if max_count is lower than the
 *         required number of bytes.
 *
 * @todo Change to use pkt_t
 */
int server_info_to_raw_packet(const srv_info_t * info, uint8_t * data,
                              uint32_t max_count);

/**
 * Extract server info from raw packet data.
 *
 * @param info Pointer to the server info structure.
 * @param data Pointer to the packet struct.
 */
void packet_to_server_info(srv_info_t * info, const pkt_t * pkt);

/**
 * Convert CTL to a raw packet.
 *
 * @param info Pointer to the server info structure.
 * @param data Pointer to the packet buffer.
 * @param max_count  The maximum number of bytes that can be written to data.
 * @return The number of bytes written, 0 if max_count is lower than the
 *         required number of bytes, -1 if CTL is not supported.
 *
 * @note CTL type must be SET, GET or GET_RANGE.
 */
int ctl_to_raw_packet(const sdr_ctl_t * ctl, uint8_t * data, int max_count);

/**
 * Extract CTL from packet data.
 *
 * @param ctl  Pointer to a CTL structure.
 * @param pkt  The packet struc.
 *
 * @note The packet must be either SET_CI, GET_CI or GET_CI_RANGE
 */
void packet_to_ctl(sdr_ctl_t * ctl, const pkt_t * pkt);


#ifdef __cplusplus
}
#endif
