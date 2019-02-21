/*
 * Nanosdr network protocol.
 *
 * Copyright  2015-2017  Alexandru Csete OZ9AEC
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
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common/sdr_data.h"
#include "nanosdr_protocol.h"
#include "sdr_ctl.h"

#ifdef __cplusplus
extern "C" {
#endif

// FIXME: Use builtin_bswap 16, 32 and 64
static inline uint16_t bytes_to_u16(const uint8_t *bytes)
{
#if defined(NANOSDR_OS_LITTLE_ENDIAN)
    return *(uint16_t *)bytes;
#else
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
#endif
}

static inline void u16_to_bytes(uint16_t val, uint8_t *bytes)
{
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
}

static inline void u32_to_bytes(uint32_t val, uint8_t *bytes)
{
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    bytes[2] = (val >> 16) & 0xFF;
    bytes[3] = (val >> 24) & 0xFF;
}

static inline uint32_t bytes_to_u32(const uint8_t *bytes)
{
#if defined(NANOSDR_OS_LITTLE_ENDIAN)
    return *(uint32_t *)bytes;
#else
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
#endif
}

static inline void u64_to_bytes(uint64_t val, uint8_t *bytes)
{
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    bytes[2] = (val >> 16) & 0xFF;
    bytes[3] = (val >> 24) & 0xFF;
    bytes[4] = (val >> 32) & 0xFF;
    bytes[5] = (val >> 40) & 0xFF;
    bytes[6] = (val >> 48) & 0xFF;
    bytes[7] = (val >> 56) & 0xFF;
}

static inline void s64_to_bytes(int64_t val, uint8_t *bytes)
{
    u64_to_bytes((uint64_t)val, bytes);
}

static inline uint64_t bytes_to_u64(const uint8_t *bytes)
{
#if defined(NANOSDR_OS_LITTLE_ENDIAN)
    return *(uint64_t *)bytes;
#else
    return (uint64_t)bytes[0] | ((uint64_t)bytes[1] << 8) |
           ((uint64_t)bytes[2] << 16) | ((uint64_t)bytes[3] << 24) |
           ((uint64_t)bytes[4] << 32) | ((uint64_t)bytes[5] << 40) |
           ((uint64_t)bytes[6] << 48) | ((uint64_t)bytes[7] << 56);
#endif
}

static inline int64_t bytes_to_s64(const uint8_t *bytes)
{
    return (int64_t)bytes_to_u64(bytes);
}

int read_packet_from_fd(int fd, pkt_t *packet)
{
    int bytes_read;

    // get packet length
    bytes_read = read(fd, packet->raw, 2);

    if (bytes_read == 0)
    {
        return 0;
    }
    else if (bytes_read == -1)
    {
        return -1;
    }
    else if (bytes_read != 2)
    {
        fprintf(stderr,
                "%s: Failed to determine packet length (bytes read: %d)\n",
                __func__, bytes_read);
        return -1;
    }

    // read rest of the packet
    packet->length = bytes_to_u16(packet->raw);
    if (packet->length <= 2)
        return 0;

    bytes_read += read(fd, &packet->raw[2], packet->length - 2);

    if (bytes_read != packet->length)
    {
        fprintf(stderr, "%s: Read %d bytes (wanted %d)\n", __func__, bytes_read,
                packet->length);
        return -1;
    }

    packet->type = packet->raw[3];

    // Check that packet type is valid; return -2 if not
    switch (packet->type)
    {
    case PKT_TYPE_PING:
    case PKT_TYPE_SET:
    case PKT_TYPE_GET:
    case PKT_TYPE_GET_RSP:
    case PKT_TYPE_GET_RNG:
    case PKT_TYPE_GET_RNG_RSP:
    case PKT_TYPE_AUDIO:
    case PKT_TYPE_FFT:
        return bytes_read;
    default:
        packet->length = 0;
        packet->type = PKT_TYPE_INVALID;
        return -2;
    }
}

int write_packet_to_fd(int fd, const pkt_t *packet)
{
    return write(fd, packet->raw, packet->length);
}

int server_info_to_raw_packet(const srv_info_t *info, uint8_t *data,
                              uint32_t max_count)
{
    if (max_count < 256)
        return 0;

    u16_to_bytes(256, &data[0]);                // packet length
    data[3] = PKT_TYPE_GET;                     // packet type
    u16_to_bytes(0x0001, &data[4]);             // CID
    data[6] = info->type;                       // server type
    u16_to_bytes(info->if_version, &data[7]);   // interface version
    u16_to_bytes(info->hw_version, &data[9]);   // hardware version
    u16_to_bytes(info->fw_version, &data[11]);  // firmware version
    u64_to_bytes(info->freq_min, &data[13]);    // RF frequency lower limit
    u64_to_bytes(info->freq_max, &data[21]);    // RF frequency upper limit
    u64_to_bytes(info->span_min, &data[29]);    // Spectrum span lower limit
    u64_to_bytes(info->span_max, &data[37]);    // Spectrum span upper limit
    u32_to_bytes(info->gains, &data[45]);       // Gain stages
    data[49] = info->antennas;                  // Number of antennas
    memset(&data[50], 0, 78);                   // Reserved for future use
    memcpy(&data[128], info->srv_name, 64);     // Server name
    memcpy(&data[192], info->dev_name, 64);     // Device name

    return 256;
}

void packet_to_server_info(srv_info_t *info, const pkt_t *pkt)
{
    const uint8_t *data;

    if (!info || !pkt)
        return;

    data = pkt->raw;
    info->type = (srv_type_t)data[6];
    info->if_version = bytes_to_u16(&data[7]);
    info->hw_version = bytes_to_u16(&data[9]);
    info->fw_version = bytes_to_u16(&data[11]);
    info->freq_min = bytes_to_u64(&data[13]);
    info->freq_max = bytes_to_u64(&data[21]);
    info->span_min = bytes_to_u64(&data[29]);
    info->span_max = bytes_to_u64(&data[37]);
    info->gains = bytes_to_u32(&data[45]);
    info->antennas = data[49];
    memcpy(info->srv_name, &data[128], 64);
    memcpy(info->dev_name, &data[192], 64);
}

/* Encode a CTL range into raw packet */
static int encode_ctl_range(const sdr_ctl_t *ctl, uint8_t *data)
{
    switch (ctl->id)
    {
    case SDR_CTL_RX_FREQ:
        u64_to_bytes(ctl->freq_range.min, &data[6]);
        u64_to_bytes(ctl->freq_range.max, &data[14]);
        data[22] = ctl->freq_range.step;
        return 23;
    default:
        fprintf(stderr, "%s: Invalid CTL ID %d\n", __func__, ctl->id);
        return -1;
    }
}

/* Encode CTL parameters into a raw packet */
static int encode_ctl_param(const sdr_ctl_t *ctl, uint8_t *data)
{
    int pkt_len;

    switch (ctl->id)
    {
    case SDR_CTL_SRV_STATE:
        pkt_len = 8;
        data[6] = 0;
        data[7] = ctl->srv_state;
        break;
    case SDR_CTL_RX_FREQ:
        pkt_len = 14;
        u64_to_bytes(ctl->freq, &data[6]);
        break;
    case SDR_CTL_GAIN:
        pkt_len = 8;
        data[6] = ctl->gain.id;
        data[7] = ctl->gain.value;
        break;
    default:
        fprintf(stderr, "%s: Invalid CTL ID %d\n", __func__, ctl->id);
        pkt_len = -1;
        break;
    }

    return pkt_len;
}

int ctl_to_raw_packet(const sdr_ctl_t *ctl, uint8_t *data, int max_count)
{
    uint16_t pkt_len = 0;

    if (max_count < MAX_CTL_LENGTH)
        return 0;

    switch (ctl->type)
    {
    case SDR_CTL_TYPE_GET:
    case SDR_CTL_TYPE_GET_RNG:
        // nothing to do for now; might change if some get_ctl has parameters
        break;
    case SDR_CTL_TYPE_SET:
    case SDR_CTL_TYPE_GET_RSP:
        pkt_len = encode_ctl_param(ctl, data);
        break;
    case SDR_CTL_TYPE_GET_RNG_RSP:
        pkt_len = encode_ctl_range(ctl, data);
        break;
    default:
        fprintf(stderr, "%s: Invalid CTL type %d\n", __func__, ctl->type);
        return -1;
        break;
    }

    u16_to_bytes(pkt_len, data);      // packet length
    data[3] = ctl->type;              // packet type
    u16_to_bytes(ctl->id, &data[4]);  // CTL ID

    return pkt_len;
}

/* Decode CTL parameter from packet */
static void decode_ctl_param(sdr_ctl_t *ctl, const pkt_t *pkt)
{
    switch (ctl->id)
    {
    case SDR_CTL_SRV_INFO:
        // processed by dedicated function
        break;
    case SDR_CTL_SRV_STATE:
        ctl->srv_state = pkt->raw[7];
        break;
    case SDR_CTL_RX_FREQ:
        ctl->freq = bytes_to_u64(&pkt->raw[6]);
        break;
    case SDR_CTL_GAIN:
        ctl->gain.id = pkt->raw[6];
        ctl->gain.value = pkt->raw[7];
        break;
    default:
        ctl->id = SDR_CTL_NONE;
        break;
    }
}

/* Decode CTL range data from packet */
static void decode_ctl_range(sdr_ctl_t *ctl, const pkt_t *pkt)
{
    switch (ctl->id)
    {
    case SDR_CTL_RX_FREQ:
        ctl->freq_range.min = bytes_to_u64(&pkt->raw[6]);
        ctl->freq_range.max = bytes_to_u64(&pkt->raw[14]);
        ctl->freq_range.step = pkt->raw[22];
        break;
    default:
        fprintf(stderr, "%s: Invalid CTL ID %d\n", __func__, ctl->id);
        ctl->id = SDR_CTL_NONE;
        break;
    }
}

void packet_to_ctl(sdr_ctl_t *ctl, const pkt_t *pkt)
{
    assert(ctl != NULL);
    assert(pkt != NULL);
    assert(pkt->type == PKT_TYPE_SET || pkt->type == PKT_TYPE_GET ||
           pkt->type == PKT_TYPE_GET_RSP || pkt->type == PKT_TYPE_GET_RNG ||
           pkt->type == PKT_TYPE_GET_RNG_RSP);

    ctl->type = pkt->type;
    ctl->id = bytes_to_u16(&pkt->raw[4]);

    switch (ctl->type)
    {
    case SDR_CTL_TYPE_GET:
    case SDR_CTL_TYPE_GET_RNG:
        // nothing to do for now; might change if some get_ctl has parameters
        break;
    case SDR_CTL_TYPE_SET:
    case SDR_CTL_TYPE_GET_RSP:
        decode_ctl_param(ctl, pkt);
        break;
    case SDR_CTL_TYPE_GET_RNG_RSP:
        decode_ctl_range(ctl, pkt);
        break;
    default:
        ctl->id = SDR_CTL_NONE;
        break;
    }
}

#ifdef __cplusplus
}
#endif
