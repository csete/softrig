/*
 * LimeSDR API definitions
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>


typedef double  float_type;
typedef void    lms_device_t;
typedef char    lms_info_str_t[256];

/**Structure used to represent the allowed value range of various parameters*/
typedef struct
{
    float_type min;     ///<Minimum allowed value
    float_type max;     ///<Minimum allowed value
    float_type step;    ///<Minimum value step
}lms_range_t;

/**Metadata structure used in sample transfers*/
typedef struct
{
    /**
     * Timestamp is a value of HW counter with a tick based on sample rate.
     * In RX: time when the first sample in the returned buffer was received
     * In TX: time when the first sample in the submitted buffer should be send
     */
    uint64_t timestamp;

    /**In TX: wait for the specified HW timestamp before broadcasting data over
     * the air
     * In RX: not used/ignored
     */
    bool waitForTimestamp;

    /**In TX: send samples to HW even if packet is not completely filled (end TX burst).
     * in RX: not used/ignored
     */
    bool flushPartialPacket;

}lms_stream_meta_t;

/**Stream structure*/
typedef struct
{
    /**
     * Stream handle. Should not be modified manually.
     * Assigned by LMS_SetupStream().*/
    size_t handle;

    //! Indicates whether stream is TX (true) or RX (false)
    bool isTx;

    //! Channel number. Starts at 0.
    uint32_t channel;

    //! FIFO size (in samples) used by stream.
    uint32_t fifoSize;

    /**
     * Parameter for controlling configuration bias toward low latency or high
     * data throughput range [0,1.0].
     * 0 - lowest latency, usually results in lower throughput
     * 1 - higher throughput, usually results in higher latency
     */
    float throughputVsLatency;

    //! Data output format
    enum
    {
        LMS_FMT_F32=0,    ///<32-bit floating point
        LMS_FMT_I16,      ///<16-bit integers
        LMS_FMT_I12       ///<12-bit integers stored in 16-bit variables
    }dataFmt;
}lms_stream_t;

/**Streaming status structure*/
typedef struct
{
    ///Indicates whether the stream is currently active
    bool active;
    ///Number of samples in FIFO buffer
    uint32_t fifoFilledCount;
    ///Size of FIFO buffer
    uint32_t fifoSize;
    ///FIFO underrun count
    uint32_t underrun;
    ///FIFO overrun count
    uint32_t overrun;
    ///Number of dropped packets by HW
    uint32_t droppedPackets;
    ///Currently not used
    float_type sampleRate;
    ///Combined data rate of all stream of the same direction (TX or RX)
    float_type linkRate;
    ///Current HW timestamp
    uint64_t timestamp;
} lms_stream_status_t;
