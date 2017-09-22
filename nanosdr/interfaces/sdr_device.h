/*
 * SDR device I/O API for nanosdr
 */
#pragma once

#include <stdint.h>
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

#define SDR_DEVICE_NONE         0
#define SDR_DEVICE_STDIN        1       // Free running, ignores sample rate
#define SDR_DEVICE_FILE         2       // Throttled file input
#define SDR_DEVICE_AIRSPY       3
#define SDR_DEVICE_RTLSDR       4
#define SDR_DEVICE_SDRIQ        5

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

/**
 * Interface definition for SDR devices.
 * @todo    Query sample formats.
 */
class SdrDevice
{

public:
    virtual ~SdrDevice() { };

    /**
     * Initialize SDR device.
     * @param  sample_rate  The sample rate the device should be initialized to
     * @param  options      Optional parameters required by some implementations
     * @retval SDR_DEVICE_OK        The device was initialized with success
     * @retval SDR_DEVICE_ELIB      Error loading driver library
     * @retval SDR_DEVICE_EINVAL    Invalid sample rate
     * @retval SDR_DEVICE_EBUSY     SDR is busy
     * @retval SDR_DEVICE_ENOTFOUND The requested input device or file could
     *                              not be found
     * @retval SDR_DEVICE_EOPEN     Failed to open SDR device
     * @retval SDR_DEVICE_ESAMPRATE Failed to set requested sample rate
     * @retval SDR_DEVICE_ERROR     Other unknown errors
     * @todo   Any other parameters?
     *
     */
    virtual int         init(float sample_rate, const char * options) = 0;

    /**
     * Get supported sample rates
     * @param rates Pointer to preallocated storage where the supported sample
     *              rates will be stored. Using 0 will simply return the number
     *              of sample rates
     * @return The number of the sample rates put in the array or SDR_DEVICE_ERROR
     *         if an error occured
     */
    virtual int         get_sample_rates(float * rates) const = 0;

    /**
     * Set input sample rate
     * @param  new_rate         The desired sample rate
     * @retval SDR_DEVICE_OK     The new sample rate was correctly set
     * @retval SDR_DEVICE_EINVAL Invalid sample rate
     * @retval SDR_DEVICE_ERROR  Other erorrs
     */   
    virtual int         set_sample_rate(float new_rate) = 0;

    /** Get current sample rate. */
    virtual float       get_sample_rate(void) const = 0;

    /** Get dynamic range. */
    virtual float       get_dynamic_range(void) const = 0;

    /**
     * Get list of gain stages.
     * @param gains Pointer to preallocated storage where the supported gains
     *              will be stored. If 0, the number of gains is returned
     * @return The number of gains available or copied into the gains array. If
     *         an error occurs SDR_DEVICE_ERROR is returned.
     *
     * @todo We should use a bit field instead, or add it as an option.
     */
    virtual int         get_gain_stages(uint8_t * gains) const  = 0;

    /** Get gain stages as a bit field. */
    virtual uint16_t    get_gain_stages_bf(void) const = 0;

    /**
     * Set gain.
     * @param stage The gain stage to set
     * @param value The new gain value between 0 and 100
     * @retval SDR_DEVICE_OK        Operation was successful
     * @retval SDR_DEVICE_EINVAL    Invalid gain stage
     * @retval SDR_DEVICE_ERANGE    Gain value out of range
     * @retval SDR_DEVICE_ERROR     Error setting gain
     */
    virtual int         set_gain(uint8_t stage, uint8_t value) = 0;

    /**
     * Start reading the input
     * @retval  SDR_DEVICE_OK    The input reader was started OK
     * @retval  SDR_DEVICE_ERROR An error occured
     */
    virtual int         start(void) = 0;

    /**
     * Stop reading the input
     * @retval  SDR_DEVICE_OK    The input reader was stopped OK
     * @retval  SDR_DEVICE_ERROR An error occured
     */
    virtual int         stop(void) = 0;

    /**
     * Set new RF frequency in Hz
     * @param freq  The new RF frequency in Hz
     * @retval  SDR_DEVICE_OK    Operation successful
     * @retval  SDR_DEVICE_ERANGE    The requested frequency is out of range
     */
    virtual int         set_freq(uint64_t freq) = 0;

    /** Get current RF frequency in Hz. */
    virtual uint64_t    get_freq(void) const = 0;

    /**
     * Get frequency range
     * @retval  SDR_DEVICE_OK       Operation successful
     * @retval  SDR_DEVICE_ERROR    An error occurred
     */
    virtual int         get_freq_range(freq_range_t * range) const = 0;

    /**
     * Set frequency correction in PPM
     * @param ppm The frequency correction in PPM
     * @retval SDR_DEVICE_OK
     * @retval SDR_DEVICE_ERROR
     */
    virtual int         set_freq_corr(float ppm) = 0;

    /** Get number of bytes available for reading */
    virtual uint32_t    get_num_bytes(void) const = 0;

    /** Get number of samples available for reading */
    virtual uint32_t    get_num_samples(void) const = 0;

    /**
     * Read bytes
     * @param   buffer  The buffer where the samples will be stored
     * @param   bytes   The number of bytes to read
     * @return  The number of bytes actually copied into the buffer
     *
     * This function will always return the requested number of bytes or 0 if
     * there aren't enough bytes in the internal buffer
     *
     * @todo We should return -1 in case of an unrecoverable error, e.g. stdin
     *       reached EOF
     */
    virtual uint32_t    read_bytes(void * buffer, uint32_t bytes) = 0;

    /**
     * Read samples
     * @param   buffer  The buffer where the samples will be stored
     * @param   samples The number of samples to read
     * @return  The number of samples actually copied into the buffer
     * 
     * This function will always return the requested number of samples or 0 if
     * there aren't enough samples in the internal buffer
     *
     * @todo We should return -1 in case of an unrecoverable error, e.g. stdin
     *       reached EOF
     */
     virtual uint32_t   read_samples(complex_t * buffer, uint32_t samples) = 0;
};

SdrDevice * sdr_device_create_airspy(void);
SdrDevice * sdr_device_create_file(void);
SdrDevice * sdr_device_create_rtlsdr(void);
SdrDevice * sdr_device_create_sdriq(void);
SdrDevice * sdr_device_create_stdin(void);

