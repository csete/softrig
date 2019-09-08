/*
 * BladeRF API definitions
 */
#pragma once

#include <stdint.h>

#define BLADERF_DESCRIPTION_LENGTH 33
#define BLADERF_SERIAL_LENGTH 33

typedef unsigned int bladerf_sample_rate;
typedef unsigned int bladerf_bandwidth;
typedef uint64_t bladerf_frequency;

typedef enum {
    BLADERF_BACKEND_ANY,         /**< "Don't Care" -- use any available
                                  *   backend */
    BLADERF_BACKEND_LINUX,       /**< Linux kernel driver */
    BLADERF_BACKEND_LIBUSB,      /**< libusb */
    BLADERF_BACKEND_CYPRESS,     /**< CyAPI */
    BLADERF_BACKEND_DUMMY = 100, /**< Dummy used for development purposes */
} bladerf_backend;

struct bladerf_devinfo {
    bladerf_backend backend;            /**< Backend to use when connecting to
                                         *   device */
    char serial[BLADERF_SERIAL_LENGTH]; /**< Device serial number string */
    uint8_t usb_bus;                    /**< Bus # device is attached to */
    uint8_t usb_addr;                   /**< Device address on bus */
    unsigned int instance;              /**< Device instance or ID */

    /** Manufacturer description string */
    char manufacturer[BLADERF_DESCRIPTION_LENGTH];
    char product[BLADERF_DESCRIPTION_LENGTH]; /**< Product description string */
};

struct bladerf_range {
    int64_t min;  /**< Minimum value */
    int64_t max;  /**< Maximum value */
    int64_t step; /**< Step of value */
    float scale;  /**< Unit scale */
};

struct bladerf_serial {
    char serial[BLADERF_SERIAL_LENGTH]; /**< Device serial number string */
};

struct bladerf_version {
    uint16_t major;       /**< Major version */
    uint16_t minor;       /**< Minor version */
    uint16_t patch;       /**< Patch version */
    const char *describe; /**< Version string with any additional suffix
                           *   information.
                           *
                           *   @warning Do not attempt to modify or free()
                           *            this string. */
};

typedef enum {
    BLADERF_FPGA_UNKNOWN = 0,   /**< Unable to determine FPGA variant */
    BLADERF_FPGA_40KLE   = 40,  /**< 40 kLE FPGA */
    BLADERF_FPGA_115KLE  = 115, /**< 115 kLE FPGA */
    BLADERF_FPGA_A4      = 49,  /**< 49 kLE FPGA (A4) */
    BLADERF_FPGA_A9      = 301  /**< 301 kLE FPGA (A9) */
} bladerf_fpga_size;

typedef enum {
    BLADERF_DEVICE_SPEED_UNKNOWN,
    BLADERF_DEVICE_SPEED_HIGH,
    BLADERF_DEVICE_SPEED_SUPER
} bladerf_dev_speed;


/**
 * Channel type
 *
 * Example usage:
 *
 * @code{.c}
 * // RX Channel 0
 * bladerf_channel ch = BLADERF_CHANNEL_RX(0);
 *
 * // RX Channel 1
 * bladerf_channel ch = BLADERF_CHANNEL_RX(1);
 *
 * // TX Channel 0
 * bladerf_channel ch = BLADERF_CHANNEL_TX(0);
 *
 * // TX Channel 1
 * bladerf_channel ch = BLADERF_CHANNEL_TX(1);
 * @endcode
 */
typedef int bladerf_channel;

/**
 * RX Channel Macro
 *
 * Example usage:
 *
 * @code{.c}
 * // RX Channel 0
 * bladerf_channel ch = BLADERF_CHANNEL_RX(0);
 *
 * // RX Channel 1
 * bladerf_channel ch = BLADERF_CHANNEL_RX(1);
 * @endcode
 */
#define BLADERF_CHANNEL_RX(ch) (bladerf_channel)(((ch) << 1) | 0x0)

/**
 * TX Channel Macro
 *
 * Example usage:
 *
 * @code{.c}
 * // TX Channel 0
 * bladerf_channel ch = BLADERF_CHANNEL_TX(0);
 *
 * // TX Channel 1
 * bladerf_channel ch = BLADERF_CHANNEL_TX(1);
 * @endcode
 */
#define BLADERF_CHANNEL_TX(ch) (bladerf_channel)(((ch) << 1) | 0x1)

/**
 * Invalid channel
 */
#define BLADERF_CHANNEL_INVALID (bladerf_channel)(-1)

/** @cond IGNORE */
#define BLADERF_DIRECTION_MASK (0x1)
/** @endcond */

/** @cond IGNORE */
/* Backwards compatible mapping to `bladerf_module`. */
typedef bladerf_channel bladerf_module;
#define BLADERF_MODULE_INVALID BLADERF_CHANNEL_INVALID
#define BLADERF_MODULE_RX BLADERF_CHANNEL_RX(0)
#define BLADERF_MODULE_TX BLADERF_CHANNEL_TX(0)
/** @endcond */

/**
 * Convenience macro: true if argument is a TX channel
 */
#define BLADERF_CHANNEL_IS_TX(ch) (ch & BLADERF_TX)

/**
 * Stream direction
 */
typedef enum {
    BLADERF_RX = 0, /**< Receive direction */
    BLADERF_TX = 1, /**< Transmit direction */
} bladerf_direction;

/**
 * Stream channel layout
 */
typedef enum {
    BLADERF_RX_X1 = 0, /**< x1 RX (SISO) */
    BLADERF_TX_X1 = 1, /**< x1 TX (SISO) */
    BLADERF_RX_X2 = 2, /**< x2 RX (MIMO) */
    BLADERF_TX_X2 = 3, /**< x2 TX (MIMO) */
} bladerf_channel_layout;


/**
 * Gain value, in decibels (dB)
 *
 * May be positive or negative.
 */
typedef int bladerf_gain;

/**
 * Gain control modes
 *
 * In general, the default mode is automatic gain control. This will
 * continuously adjust the gain to maximize dynamic range and minimize clipping.
 *
 * @note Implementers are encouraged to simply present a boolean choice between
 *       "AGC On" (::BLADERF_GAIN_DEFAULT) and "AGC Off" (::BLADERF_GAIN_MGC).
 *       The remaining choices are for advanced use cases.
 */
typedef enum {
    /** Device-specific default (automatic, when available)
     *
     * On the bladeRF x40 and x115 with FPGA versions >= v0.7.0, this is
     * automatic gain control.
     *
     * On the bladeRF 2.0 Micro, this is BLADERF_GAIN_SLOWATTACK_AGC with
     * reasonable default settings.
     */
    BLADERF_GAIN_DEFAULT,

    /** Manual gain control
     *
     * Available on all bladeRF models.
     */
    BLADERF_GAIN_MGC,

    /** Automatic gain control, fast attack (advanced)
     *
     * Only available on the bladeRF 2.0 Micro. This is an advanced option, and
     * typically requires additional configuration for ideal performance.
     */
    BLADERF_GAIN_FASTATTACK_AGC,

    /** Automatic gain control, slow attack (advanced)
     *
     * Only available on the bladeRF 2.0 Micro. This is an advanced option, and
     * typically requires additional configuration for ideal performance.
     */
    BLADERF_GAIN_SLOWATTACK_AGC,

    /** Automatic gain control, hybrid attack (advanced)
     *
     * Only available on the bladeRF 2.0 Micro. This is an advanced option, and
     * typically requires additional configuration for ideal performance.
     */
    BLADERF_GAIN_HYBRID_AGC,
} bladerf_gain_mode;

typedef enum {
    BLADERF_FPGA_SOURCE_UNKNOWN = 0, /**< Uninitialized/invalid */
    BLADERF_FPGA_SOURCE_FLASH   = 1, /**< Last FPGA load was from flash */
    BLADERF_FPGA_SOURCE_HOST    = 2  /**< Last FPGA load was from host */
} bladerf_fpga_source;




/**
 * Sample format
 */
typedef enum {
    /**
     * Signed, Complex 16-bit Q11. This is the native format of the DAC data.
     *
     * Values in the range [-2048, 2048) are used to represent [-1.0, 1.0).
     * Note that the lower bound here is inclusive, and the upper bound is
     * exclusive. Ensure that provided samples stay within [-2048, 2047].
     */
    BLADERF_FORMAT_SC16_Q11,

    /**
     * This format is the same as the ::BLADERF_FORMAT_SC16_Q11 format, except
     * the first 4 samples in every <i>block*</i> of samples are replaced with
     * metadata organized as follows. All fields are little-endian byte order.
     */
    BLADERF_FORMAT_SC16_Q11_META,
} bladerf_format;



typedef enum {
    BLADERF_LOG_LEVEL_VERBOSE,  /**< Verbose level logging */
    BLADERF_LOG_LEVEL_DEBUG,    /**< Debug level logging */
    BLADERF_LOG_LEVEL_INFO,     /**< Information level logging */
    BLADERF_LOG_LEVEL_WARNING,  /**< Warning level logging */
    BLADERF_LOG_LEVEL_ERROR,    /**< Error level logging */
    BLADERF_LOG_LEVEL_CRITICAL, /**< Fatal error level logging */
    BLADERF_LOG_LEVEL_SILENT    /**< No output */
} bladerf_log_level;

// clang-format off
#define BLADERF_ERR_UNEXPECTED  (-1)  /**< An unexpected failure occurred */
#define BLADERF_ERR_RANGE       (-2)  /**< Provided parameter is out of range */
#define BLADERF_ERR_INVAL       (-3)  /**< Invalid operation/parameter */
#define BLADERF_ERR_MEM         (-4)  /**< Memory allocation error */
#define BLADERF_ERR_IO          (-5)  /**< File/Device I/O error */
#define BLADERF_ERR_TIMEOUT     (-6)  /**< Operation timed out */
#define BLADERF_ERR_NODEV       (-7)  /**< No device(s) available */
#define BLADERF_ERR_UNSUPPORTED (-8)  /**< Operation not supported */
#define BLADERF_ERR_MISALIGNED  (-9)  /**< Misaligned flash access */
#define BLADERF_ERR_CHECKSUM    (-10) /**< Invalid checksum */
#define BLADERF_ERR_NO_FILE     (-11) /**< File not found */
#define BLADERF_ERR_UPDATE_FPGA (-12) /**< An FPGA update is required */
#define BLADERF_ERR_UPDATE_FW   (-13) /**< A firmware update is requied */
#define BLADERF_ERR_TIME_PAST   (-14) /**< Requested timestamp is in the past */
#define BLADERF_ERR_QUEUE_FULL  (-15) /**< Could not enqueue data into
                                       *   full queue */
#define BLADERF_ERR_FPGA_OP     (-16) /**< An FPGA operation reported failure */
#define BLADERF_ERR_PERMISSION  (-17) /**< Insufficient permissions for the
                                       *   requested operation */
#define BLADERF_ERR_WOULD_BLOCK (-18) /**< Operation would block, but has been
                                       *   requested to be non-blocking. This
                                       *   indicates to a caller that it may
                                       *   need to retry the operation later.
                                       */
#define BLADERF_ERR_NOT_INIT    (-19) /**< Device insufficiently initialized
                                       *   for operation */
// clang-format on
