/*
 * SDR device I/O API for nanosdr
 */
#pragma once

#include <stdint.h>
#include <string.h>
#include <strings.h>    // strcasecmp
#include "common/datatypes.h"
#include "common/sdr_data.h"

#if 1
#define sdr_device_debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define sdr_device_debug(...)
#endif

// Error codes used for return values
#define SDR_DEVICE_OK           0
#define SDR_DEVICE_ERROR       -1
#define SDR_DEVICE_ELIB        -2
#define SDR_DEVICE_EINVAL      -3
#define SDR_DEVICE_ERANGE      -4
#define SDR_DEVICE_EBUSY       -5
#define SDR_DEVICE_EPERM       -6
#define SDR_DEVICE_ENOTFOUND   -7
#define SDR_DEVICE_EOPEN       -8
#define SDR_DEVICE_ESAMPRATE   -9

// IDs for supported devices
#define SDR_DEVICE_NONE         0
#define SDR_DEVICE_RTLSDR       1
#define SDR_DEVICE_AIRSPY       2
#define SDR_DEVICE_AIRSPYMINI   3
#define SDR_DEVICE_SDRIQ        4
#define SDR_DEVICE_STDIN        5       // Free running, ignores sample rate
#define SDR_DEVICE_FILE         6       // Throttled file input


// Gain stage identifiers
// FIXME: Use SDR_GAIN_ID... directly
#define SDR_DEVICE_RX_LNA_GAIN      SDR_GAIN_ID_RX_LNA
#define SDR_DEVICE_RX_MIX_GAIN      SDR_GAIN_ID_RX_MIX
#define SDR_DEVICE_RX_IF_GAIN       SDR_GAIN_ID_RX_IF
#define SDR_DEVICE_RX_VGA_GAIN      SDR_GAIN_ID_RX_VGA
#define SDR_DEVICE_RX_LIN_GAIN      SDR_GAIN_ID_RX_LIN
#define SDR_DEVICE_RX_SENS_GAIN     SDR_GAIN_ID_RX_SENS
#define SDR_DEVICE_RX_RF_AGC        SDR_GAIN_ID_RX_RF_AGC
#define SDR_DEVICE_RX_IF_AGC        SDR_GAIN_ID_RX_IF_AGC
#define SDR_DEVICE_TX_PA_GAIN       SDR_GAIN_ID_TX_PA
#define SDR_DEVICE_TX_MIX_GAIN      SDR_GAIN_ID_TX_MIX
#define SDR_DEVICE_TX_IF_GAIN       SDR_GAIN_ID_TX_IF
#define SDR_DEVICE_TX_VGA_GAIN      SDR_GAIN_ID_TX_VGA

class SdrDevice
{

public:
    virtual ~SdrDevice() { };

    /*
     * Initialize SDR device.
     *
     * Parameters:
     *   sample_rate  The initial sample rate to be used
     *   options      Optional parameters required by some backends
     *
     * Returns:
     *   SDR_DEVICE_OK        The device was initialized with success
     *   SDR_DEVICE_ELIB      Error loading driver library
     *   SDR_DEVICE_EINVAL    Invalid sample rate
     *   SDR_DEVICE_EBUSY     SDR is busy
     *   SDR_DEVICE_ENOTFOUND The requested input device or file could
     *                        not be found
     *   SDR_DEVICE_EOPEN     Failed to open SDR device
     *   SDR_DEVICE_ESAMPRATE Failed to set requested sample rate
     *   SDR_DEVICE_ERROR     Other unknown errors
     *
     * TODO: Any other parameters?
     *
     */
    virtual int         init(float sample_rate, const char * options) = 0;

    /*
     * Get supported sample rates.
     *
     * The parameter rates is a pointer to preallocated storage where
     * the supported sample rates will be stored. Using 0 will return
     * the number of sample rates without copying anything into the
     * array.
     *
     * Returns the number of the sample rates put in the array or
     * SDR_DEVICE_ERROR if an error occured.
     *
     * TODO: Add parameter to indicate max number of rates that can be
     *       copied into the array.
     */
    virtual int         get_sample_rates(float * rates) const = 0;

    /*
     * Set input sample rate.
     * Returns:
     *   SDR_DEVICE_OK     The new sample rate was correctly set
     *   SDR_DEVICE_EINVAL Invalid sample rate
     *   SDR_DEVICE_ERROR  Other erorrs
     */   
    virtual int         set_sample_rate(float new_rate) = 0;

    virtual float       get_sample_rate(void) const = 0;
    virtual float       get_dynamic_range(void) const = 0;

    /*
     * Get list of gain stages.
     *
     * Parameters:
     *   gains: Pointer to preallocated storage where the supported gains
     *          will be stored. If 0, the number of gains is returned
     *
     * Returns the number of gains available or copied into the gains
     * array. If an error occurs SDR_DEVICE_ERROR is returned.
     *
     * TODO: We should use a bit field instead, or add it as an option.
     */
    virtual int         get_gain_stages(uint8_t * gains) const  = 0;

    /* Get gain stages as a bit field. */
    virtual uint16_t    get_gain_stages_bf(void) const = 0;

    /*
     * Set gain.
     *
     * Parameters:
     *   stage: The gain stage to set
     *   value: The new gain value between 0 and 100
     *
     * Returns:
     *   SDR_DEVICE_OK        Operation was successful
     *   SDR_DEVICE_EINVAL    Invalid gain stage
     *   SDR_DEVICE_ERANGE    Gain value out of range
     *   SDR_DEVICE_ERROR     Error setting gain
     */
    virtual int         set_gain(uint8_t stage, uint8_t value) = 0;

    /*
     * Start reading from the device.
     * Returns:
     *   SDR_DEVICE_OK
     *   SDR_DEVICE_ERROR
     */
    virtual int         start(void) = 0;

    /*
     * Stop reading from the device.
     * Returns:
     *   SDR_DEVICE_OK
     *   SDR_DEVICE_ERROR
     */
    virtual int         stop(void) = 0;

    /*
     * Set new RF frequency in Hz
     *
     * Parameters:
     *   freq   The new RF frequency in Hz.
     *
     * Returns:
     *   SDR_DEVICE_OK      Operation successful
     *   SDR_DEVICE_ERANGE  The requested frequency is out of range
     */
    virtual int         set_freq(uint64_t freq) = 0;

    /* Get current RF frequency in Hz */
    virtual uint64_t    get_freq(void) const = 0;

    /*
     * Get frequency range
     * Returns:
     *   SDR_DEVICE_OK
     *   SDR_DEVICE_ERROR
     */
    virtual int         get_freq_range(freq_range_t * range) const = 0;

    /*
     * Set frequency correction in PPM.
     * Returns:
     *   SDR_DEVICE_OK
     *   SDR_DEVICE_ERROR
     */
    virtual int         set_freq_corr(float ppm) = 0;

    /* Get number of bytes available for reading */
    virtual uint32_t    get_num_bytes(void) const = 0;

    /* Get number of samples available for reading */
    virtual uint32_t    get_num_samples(void) const = 0;

    /*
     * Read bytes.
     *
     * Parameters:
     *   buffer:  The buffer where the samples will be stored.
     *   bytes:   The number of bytes to read.
     *
     * Returns the number of bytes copied into the buffer. This will
     * always be equal to the requested number of bytes or 0 if there
     * aren't enough bytes in the internal buffer.
     *
     * TODO: We should return -1 in case of an unrecoverable error, e.g.
     *       when stdin reached EOF.
     */
    virtual uint32_t    read_bytes(void * buffer, uint32_t bytes) = 0;

    /*
     * Read complex samples.
     *
     * Parameters:
     *   buffer:  The buffer where the samples will be stored.
     *   samples: The number of samples to read.
     *
     * Returns the number of samples copied into the buffer. This will
     * always be equal to the requested number of samples or 0 if there
     * aren't enough samples in the internal buffer.
     *
     * TODO: We should return -1 in case of an unrecoverable error,
     *       e.g. when stdin reached EOF
     */
     virtual uint32_t   read_samples(complex_t * buffer, uint32_t samples) = 0;

    /* Get SDR type. Backends implement this to return SDR_DEVICE_XYZ */
    virtual int type(void) const { return SDR_DEVICE_NONE; };
};

SdrDevice * sdr_device_create_rtlsdr(void);
SdrDevice * sdr_device_create_airspy(void);
SdrDevice * sdr_device_create_airspymini(void);
SdrDevice * sdr_device_create_sdriq(void);
SdrDevice * sdr_device_create_stdin(void);
SdrDevice * sdr_device_create_file(void);

inline SdrDevice * sdr_device_create(const char * type)
{
    if (strcasecmp("rtlsdr", type) == 0)
        return sdr_device_create_rtlsdr();
    else if (strcasecmp("airspy", type) == 0)
        return sdr_device_create_airspy();
    else if (strcasecmp("airspymini", type) == 0)
        return sdr_device_create_airspymini();
    else if (strcasecmp("sdriq", type) == 0)
        return sdr_device_create_sdriq();
    else if (strcasecmp("stdin", type) == 0)
        return sdr_device_create_stdin();
    else if (strstr(type, ".wav") != 0 || strstr(type, ".raw") != 0 ||
             strstr(type, ".iq"))
        return sdr_device_create_file();

    return NULL;
}
