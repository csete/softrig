/*
 * SDR-IQ driver for nanosdr.
 *
 * Copyright  2014-2017  Alexandru Csete OZ9AEC
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
 *
 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "common/library_loader.h"
#include "common/ring_buffer.h"
#include "sdriq.h"
#include "sdriq_parser.h"

/* libftdi API */
static void * (*ftdi_new) (void);
static void   (*ftdi_free) (void * ftdi);
static int    (*ftdi_usb_open_desc) (void * ftdi, int vendor, int product,
                                     const char* description, const char* serial);
static int    (*ftdi_usb_purge_rx_buffer) (void * ftdi);
static int    (*ftdi_usb_close) (void * ftdi);
static int    (*ftdi_read_data_set_chunksize) (void * ftdi, unsigned int chunksize);
static int    (*ftdi_read_data) (void * ftdi, unsigned char * buf, int size);
static int    (*ftdi_write_data) (void * ftdi, const unsigned char * buf, int size);
static char * (*ftdi_get_error_string) (void * ftdi);

                              
/**
 * SDR-IQ data structure.
 *
 * @fw_version      Firmware version multiplied by 100
 * @boot_version    Boot code version multiplied by 100
 * @ascp_version    Interface version multipled by 100
 * @is_loaded       Flag indicating whether libftdi is loaded
 * @is_open         Flag indicating whether the device is open
 * @is_running      Flag indicating whether the device is streaming
 * @rf_gain         Last RF gain reported by the device
 * @if_gain         Last IF gain reported by the device
 * @frequency       Last RX frequency reported by the device
 * @sample_rate     Last sample rate reported by the device
 *
 * @reader_thread_id    ID of the thread reading I/Q samples from the device.
 * @ftdi_mutex          Mutex used for USB transactions.
 *
 * @rb      Ring buffer holding the I/Q samples.
 * @ftdi    FTDI handle.
 */
struct _sdriq
{
    uint16_t  fw_version;
    uint16_t  boot_version;
    uint16_t  ascp_version;

    int       is_loaded;
    int       is_open;
    int       is_running;

    int8_t    rf_gain;
    int8_t    if_gain;
    uint32_t  frequency;
    uint32_t  sample_rate;

    pthread_t reader_thread_id;
    pthread_mutex_t ftdi_mutex;

    ring_buffer_t *rb;

//    struct ftdi_context *ftdi;
    void     *ftdi;
    lib_handle_t    ftdi_lib_handle;
};

/**
 * SDR-IQ reader thread.
 * @param data_ptr Pointer to the SDR-IQ handle (assumed non NULL).
 *
 * This thread is used to read data from the SDR-IQ. The thread is created and
 * started as soon as the SDR-IQ device is opened and it will keep running
 * until the SDR-IQ is closed. This allows using this single function to
 * perform all reads from the device.
 *
 * We assume that we can read whole messages in one iteration. Therefore, our
 * strategy is to first read the 16 bit header of the message (c.f. section 3
 * of the ICD), then the contents of the message using the length extreacted
 * from the header.
 *
 * Writing to the device is done asynchronously in the set_xyz() functions.
 * Thus the ftdi and libusb contexts become shared resources and we use a mutex
 * to manage access to them. Technically, a mutex is only required when writing
 * to shared resources; however, we do not know what libftdi and libusb do with
 * the contexts and therefore we have chosen the safest path.
 *
 * The I/Q data read from the device is accumulated in a ring buffer that has
 * room for approximately 100 ms of data.
 */
static void *sdriq_reader_thread(void *data_ptr)
{
    sdriq_t *sdr = (sdriq_t *) data_ptr;

    unsigned char read_buffer[8192];
    int rd, length;
    uint_fast64_t short_reads = 0;
    uint_fast64_t iq_bytes = 0;
    uint_fast64_t other_bytes = 0;

    fputs("Starting SDR-IQ I/O thread.\n", stderr);

    while (sdr->is_open)
    {
        usleep(1000);

        pthread_mutex_lock(&sdr->ftdi_mutex);
        rd = ftdi_read_data(sdr->ftdi, read_buffer, 2);
        pthread_mutex_unlock(&sdr->ftdi_mutex);

        if (rd == 0)
        {
            continue;
        }
        else if (rd == 1)
        {
            short_reads++;
            continue;
        }

        if (read_buffer[1] == 0x80)
        {
            /* IQ data */
            length = 8192;
            iq_bytes += length; // not sure
        }
        else
        {
            /* See sec. 3 in ICD */
            length = (read_buffer[0] | read_buffer[1] << 8) & 0x1FFF;
            if (length >= 2)
                length -= 2; /* length includes the 2 byte header */
            other_bytes += length; //not sure
        }

        if (!length)
            continue;

        /* length can not be larger than 8192 */
        pthread_mutex_lock(&sdr->ftdi_mutex);
        rd = ftdi_read_data(sdr->ftdi, read_buffer, length);
        pthread_mutex_unlock(&sdr->ftdi_mutex);

        if (rd != length)
        {
            short_reads++;
        }
        else if (length == 8192)
        {
            /* copy I/Q data to buffer */
            ring_buffer_write(sdr->rb, read_buffer, length);
        }
        else
        {
            /* we got a response to a command */
            parse_response(sdr, read_buffer, length);
        }

    }

    fprintf(stderr,
        "Exiting SDR-IQ I/O thread.\n"
        "  IQ data bytes      : %" PRIu64 "\n"
        "  Other message bytes: %" PRIu64 "\n"
        "  Short reads (num)  : %" PRIu64 "\n",
        iq_bytes, other_bytes, short_reads);

    pthread_exit(NULL);
}

/** Print a data buffer as hex octets */
static void print_buffer(uint8_t *buffer, int size)
{
    int i;

    for (i = 0; i < size; i++)
        fprintf(stderr, " %02X", buffer[i]);
    fputs("\n", stderr);
}

/* Performs some sanity check on the opened device:
 *  - Print target name.
 *  - Print serial number.
 *  - Read and store FW, bootcode and interface version numbers.
 *  - Print product ID.
 * Currently, this function is only called from the sdriq_open() function
 * before the io_thread is started.
 */
static void print_info(sdriq_t *sdr)
{
    unsigned char write_buf[8], read_buf[16];
    int size;
    int ret;

    if (sdr == NULL)
        return;

    /* 5.1.1  Target name */
    write_buf[0] = 0x04;
    write_buf[1] = 0x20;
    write_buf[2] = 0x01;
    write_buf[3] = 0x00;
    size = 4;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 11;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            fprintf(stderr, "  Device name   : %s\n", &read_buf[4]);
    }

    /* 5.1.2  Serial number */
    write_buf[0] = 0x04;
    write_buf[1] = 0x20;
    write_buf[2] = 0x02;
    write_buf[3] = 0x00;
    size = 4;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 16;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            fprintf(stderr, "  Device serial : %s\n", &read_buf[4]);
    }

    /* 5.1.3  Interface version */
    write_buf[0] = 0x04;
    write_buf[1] = 0x20;
    write_buf[2] = 0x03;
    write_buf[3] = 0x00;
    size = 4;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 6;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            sdr->ascp_version = (read_buf[5] << 8) + read_buf[4];
    }

    /* 5.1.4  Firmware and boot code version */
    write_buf[0] = 0x05;
    write_buf[1] = 0x20;
    write_buf[2] = 0x04;
    write_buf[3] = 0x00;
    write_buf[4] = 0x00; /* parameter for boot code */
    size = 5;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 7;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            sdr->boot_version = (read_buf[6] << 8) + read_buf[5];
    }

    write_buf[4] = 0x01; /* parameter for firmware */
    size = 5;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 7;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            sdr->fw_version = (read_buf[6] << 8) + read_buf[5];
    }

    fprintf(stderr,
        "  Boot version  : %d\n"
        "  FW version    : %d\n"
        "  ASCP version  : %d\n",
        sdr->boot_version, sdr->fw_version, sdr->ascp_version);

    /* 5.1.5  Status/Error Code */
    write_buf[0] = 0x04;
    write_buf[1] = 0x20;
    write_buf[2] = 0x05;
    write_buf[3] = 0x00;
    size = 4;
    ret = ftdi_write_data(sdr->ftdi, write_buf, size);
    if (ret != size)
    {
        fprintf(stderr, "SDR-IQ error_ Wrote %d bytes. Expected %d\n", ret, size);
    }
    else
    {
        size = 5;
        ret = ftdi_read_data(sdr->ftdi, read_buf, size);
        if (ret != size)
            fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
        else
            fprintf(stderr, "  Status code   : 0x%02X\n", read_buf[4]);
    }

    /* 5.1.6  Product ID (FW >= 1.04) */
    if (sdr->fw_version >= 104)
    {
        write_buf[0] = 0x04;
        write_buf[1] = 0x20;
        write_buf[2] = 0x09;
        write_buf[3] = 0x00;
        size = 4;
        ret = ftdi_write_data(sdr->ftdi, write_buf, size);
        if (ret != size)
        {
            fprintf(stderr, "SDR-IQ error: Wrote %d bytes. Expected %d\n", ret, size);
        }
        else
        {
            size = 8;
            ret = ftdi_read_data(sdr->ftdi, read_buf, size);
            if (ret != size)
            {
                fprintf(stderr, "SDR-IQ error: Read %d bytes. Expected %d\n", ret, size);
            }
            else
            {
                fprintf(stderr, "  Product ID    :");
                print_buffer(read_buf, ret);
            }
        }
    }

}

/**
 * Create a new SDR-IQ object.
 * 
 * @returns A pointer to a new sdriq_t handle or NULL in case of failure.
 */
sdriq_t *sdriq_new(void)
{
    void *libhandle = load_library("ftdi1");
    if (libhandle == NULL)
        goto dl_error;

    ftdi_new = (void * (*) (void)) get_symbol(libhandle, "ftdi_new");
    if (ftdi_new == NULL)
        goto dl_error;

    ftdi_free = (void (*) (void *)) get_symbol(libhandle, "ftdi_free");
    if (ftdi_free == NULL)
        goto dl_sym_error;

    ftdi_usb_open_desc = (int (*) (void *, int, int, const char *, const char *))
                            get_symbol(libhandle, "ftdi_usb_open_desc");
    if (ftdi_usb_open_desc == NULL)
        goto dl_sym_error;

    ftdi_usb_purge_rx_buffer = (int (*) (void *))
                            get_symbol(libhandle, "ftdi_usb_purge_rx_buffer");
    if (ftdi_usb_purge_rx_buffer == NULL)
        goto dl_sym_error;

    ftdi_usb_close = (int (*) (void *)) get_symbol(libhandle, "ftdi_usb_close");
    if (ftdi_usb_close == NULL)
        goto dl_sym_error;

    ftdi_read_data_set_chunksize = (int (*) (void *, unsigned int))
                            get_symbol(libhandle, "ftdi_read_data_set_chunksize");
    if (ftdi_read_data_set_chunksize == NULL)
        goto dl_sym_error;

    ftdi_read_data = (int (*) (void *, unsigned char *, int))
                            get_symbol(libhandle, "ftdi_read_data");
    if (ftdi_read_data == NULL)
        goto dl_sym_error;

    ftdi_write_data = (int (*) (void *, const unsigned char *, int))
                            get_symbol(libhandle, "ftdi_write_data");
    if (ftdi_write_data == NULL)
        goto dl_sym_error;

    ftdi_get_error_string = (char * (*) (void *))
                            get_symbol(libhandle, "ftdi_get_error_string");
    if (ftdi_get_error_string == NULL)
        goto dl_sym_error;


    sdriq_t *sdr = (sdriq_t *) malloc(sizeof(sdriq_t));
    sdr->ftdi_lib_handle = libhandle;
    sdr->is_loaded = 1;

    if ((sdr->ftdi = ftdi_new()) == 0)
    {
        fputs("ftdi_new() failed\n", stderr);
        free(sdr);
        sdr = NULL;
    }
    else
    {
        ftdi_read_data_set_chunksize(sdr->ftdi, 8192);
        sdr->rf_gain = 0;
        sdr->if_gain = 0;
        sdr->frequency = 0;
        sdr->sample_rate = 0;
        sdr->fw_version = 0;
        sdr->boot_version = 0;
        sdr->ascp_version = 0;
        sdr->is_open = 0;
        sdr->is_running = 0;
        sdr->rb = (ring_buffer_t *) malloc(sizeof(ring_buffer_t));
        ring_buffer_init(sdr->rb, (196078*4)/10); /* 100 msec at default sample rate */
    }

    return sdr;

dl_sym_error:
    close_library(libhandle);
dl_error:
    fputs("Failed to load FTDI library\n", stderr);
    return NULL;
}

/**
 * Free sdriq_t handle.
 * 
 * @param sdr Pointer to the sdriq_t handle.
 */
void sdriq_free(sdriq_t *sdr)
{
    if (sdr != NULL)
    {
        if (sdr->is_loaded)
            close_library(sdr->ftdi_lib_handle);

        /* ensure SDR is stopped and device closed */
        if (sdr->is_running)
            sdriq_stop(sdr);

        if (sdr->is_open)
            sdriq_close(sdr);

        /* free resources */
        ftdi_free(sdr->ftdi);
        ring_buffer_delete(sdr->rb);
        free(sdr);
    }
}

/**
 * Open the SDR-IQ device.
 * 
 * @param  sdr The sdriq_t handle.
 * @retval  0  Device opened successfully.
 * @retval -3  USB device not found.
 * @retval -4  Unable to open device.
 * @retval -5  Unable to claim device.
 * @retval -6  Reset failed.
 * @retval -7  Set baudrate failed.
 * @retval -8  Get product description failed.
 * @retval -9  Get serial number failed.
 * @retval -12 libusb_get_device_list() failed.
 * @retval -13 libusb_get_device_descriptor() failed.
 * @retval -100 Invalid sdriq_t handle.
 * @retval  >0  Failed to start input reader thread.
 *
 * @todo Add device index to allow using multiple devices.
 */
int sdriq_open(sdriq_t *sdr)
{
    if (sdr == NULL)
        return -100;

    /* All FTDI devices have VID=0x0403 and PID=0x6001; however, we can find the
     * string "SDR-IQ" in the USB device description (ftdi_usb_get_strings()):
     * 
     *   Number of devices found: 2
     *   Device: 0  Mf: FTDI,  Desc: FT232R USB UART
     *   Device: 1  Mf: FTDI,  Desc: SDR-IQ
     *
     * For a single device we can use:
     *   ftdi_usb_open_desc(ftdi, VID, PID, "SDR-IQ", NULL);
     * 
     * or if we want to support multiple devices:
     *   ftdi_usb_open_desc_index(ftdi, VID, PID, "SDR-IQ", NULL, idx);
     */
    int ret = ftdi_usb_open_desc(sdr->ftdi, 0x0403, 0x6001, "SDR-IQ", NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Unable to open SDR-IQ device: %d (%s)\n", ret,
                ftdi_get_error_string(sdr->ftdi));
    }
    else
    {
        sdr->is_open = 1;
        if (ftdi_usb_purge_rx_buffer(sdr->ftdi))
            fputs("Failed to purge RX buffer\n", stderr);

        fputs("SDR-IQ device opened:\n", stderr);
        print_info(sdr);

        /* start input reader thread */
        ret = pthread_create(&sdr->reader_thread_id, NULL, sdriq_reader_thread, sdr);
        if (ret)
            fprintf(stderr, "Error creating SDR-IQ input_reader thread: %d\n", ret);

        pthread_mutex_init(&sdr->ftdi_mutex, NULL);
    }

    return ret;
}

/**
 * Close the SDR-IQ device.
 *
 * @param sdr The sdriq_t handle.
 * @retval  0  The device closed successfully.
 * @retval -1	usb_release() failed.
 * @retval -3	ftdi context invalid.
 * @retval -100 Invalid SDR-IQ handle.
 * @retval  >0  Failed to terminate input reader thread (huh, then what?)
 */
int sdriq_close(sdriq_t *sdr)
{
    if (sdr == NULL)
        return -100;

    int ret;

    sdr->is_open = 0;
    if ((ret = pthread_join(sdr->reader_thread_id, NULL)))
        fprintf(stderr, "Error stopping input_reader thread: %d\n", ret);

    /* close device */
    if ((ret = ftdi_usb_close(sdr->ftdi)) < 0)
        fprintf(stderr, "Unable to close SDR-IQ device: %d (%s)\n", ret,
                ftdi_get_error_string(sdr->ftdi));

    return ret;
}

int sdriq_start(sdriq_t *sdr)
{
    return sdriq_set_state(sdr, SDRIQ_STATE_RUN);
}

int sdriq_stop(sdriq_t *sdr)
{
    return sdriq_set_state(sdr, SDRIQ_STATE_IDLE);
}

/**
 * Set SDR-IQ state.
 * @param sdr The SDR-IQ handle.
 * @param state One of SDRIQ_STATE_RUN or SDRIQ_STATE_IDLE.
 * @retval    0  Everything OK
 * @retval   -1  Invalid state.
 * @retval -100  Invalid SDR-IQ handle
 *
 * This function will start or stop data acquisition on the SDR-IQ. Currently
 * only continuous mode is supported, see section 5.2.1 in the ICD.
 *
 * @sa sdriq_start
 * @sa sdriq_stop
 */
int sdriq_set_state(sdriq_t *sdr, unsigned char state)
{
    unsigned char write_buffer[] = {0x08,0x00,0x18,0x00,0x81,0x00,0x00,0x01};

    if (sdr == NULL)
        return -100;

    if (state != SDRIQ_STATE_IDLE && state != SDRIQ_STATE_RUN)
        return -1;

    write_buffer[5] = state;

    if (ftdi_write_data(sdr->ftdi, write_buffer, 8) != 8) 
        return -2;
    else
        /* FIXME: should use readback value? */
        sdr->is_running = (state == SDRIQ_STATE_RUN);

    return 0;
}


int sdriq_set_freq(sdriq_t *sdr, uint32_t freq)
{
    unsigned char write_buffer[] = {0x0A,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    int rv = 0;

    if (sdr == NULL)
        return -100;

    if (freq > 33333333)
        return -1;

    write_buffer[5] = freq & 0xFF;
    write_buffer[6] = (freq >> 8) & 0xFF;
    write_buffer[7] = (freq >> 16) & 0xFF;
    write_buffer[8] = (freq >> 24) & 0xFF;

    pthread_mutex_lock(&sdr->ftdi_mutex);
    if (ftdi_write_data(sdr->ftdi, write_buffer, 10) != 10)
        rv = -2;

    pthread_mutex_unlock(&sdr->ftdi_mutex);

    return rv;
}

/**
 * Set sample rate (sec. 5.2.4 of the ICD).
 *
 * @param sdr The SDR-IQ handle.
 * @param rate The sample rate.
 * @retval    0  The sample rate was correctly set.
 * @retval   -1  Invalid sample rate.
 * @retval   -2  Write error.
 * @retval -100  Invalid SDR-IQ handle.
 * @retval -101  SDR-IQ is running.
 *
 * Supported sample rates are: 8138, 16276, 37793, 55556, 111111, 158730 and
 * 196078 ksps c.f. section 5.2.4 of the ICD.
 *
 * This command should not be sent while the device is running.
 */
int sdriq_set_sample_rate(sdriq_t *sdr, uint32_t rate)
{
    unsigned char write_buffer[] = {0x09,0x00,0xB8,0x00,0x00,0x00,0x00,0x00,0x00};
    int ret=0, wr;

    if (sdr == NULL)
        return -100;

    if (sdr->is_running)
        return -101;

    if ((rate != 8138) && (rate != 16276) && (rate != 37793) && (rate != 55556)
        && (rate != 111111) && (rate != 158730) && (rate != 196078))
        return -1;

    write_buffer[5] = rate & 0xFF;
    write_buffer[6] = (rate >> 8) & 0xFF;
    write_buffer[7] = (rate >> 16) & 0xFF;
    write_buffer[8] = (rate >> 24) & 0xFF;

    wr = ftdi_write_data(sdr->ftdi, write_buffer, 9); 
    if (wr != 9)
    {
        ret = -2;
    }
    else
    {
        /* resize the ring buffer to ~100 msec and integer multiple of 8192 */
        uint32_t new_size = (rate*4)/10;
        new_size += 8192 - (new_size % 8192);
        fprintf(stderr, "SDR-IQ ringbuf size: %d\n", new_size);
        ring_buffer_resize(sdr->rb, new_size);
    }

    sdr->sample_rate = rate;

    return ret;
}

uint32_t sdriq_get_sample_rate(const sdriq_t * sdr)
{
    return sdr->sample_rate;
}

/**
 * Set actual A/D input rate (sec 5.2.3 of the ICD)
 *
 * @param  rate  The actual input rate.
 * @retval    0  The sample rate was correctly set.
 * @retval   -2  Write error.
 * @retval -100  Invalid SDR-IQ handle.
 */
int sdriq_set_input_rate(sdriq_t * sdr, uint32_t rate)
{
    uint8_t     write_buffer[] = {0x09,0x00,0xB0,0x00,0x00,0x00,0x00,0x00,0x00};
    int         wr;

    if (sdr == NULL)
        return -100;

    write_buffer[5] = rate & 0xFF;
    write_buffer[6] = (rate >> 8) & 0xFF;
    write_buffer[7] = (rate >> 16) & 0xFF;
    write_buffer[8] = (rate >> 24) & 0xFF;

    wr = ftdi_write_data(sdr->ftdi, write_buffer, 9); 
    if (wr != 9)
        return -2;
    else
        return 0;
}

/* Generic set_gain function implementing the common parts of sections
 * 5.2.5 and 5.2.6 of the ICD.
 *
 * Currently only fixed gain mode is supported.
 *
 * stage: RF=0x38 IF=0x40
 * Return values are as follows:
 *  -2  Write error.
 */
static int set_gain(sdriq_t *sdr, uint8_t stage, int8_t gain)
{
    unsigned char write_buffer[] = { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 };
    int ret=0, wr;

    write_buffer[2] = stage;
    write_buffer[5] = gain;

    /* write new gain */
    wr = ftdi_write_data(sdr->ftdi, write_buffer, 6); 
    if (wr != 6)
        ret = -2;

    return ret;
}

/**
 * Set fixed RF gain.
 * 
 * @param sdr The SDR-IQ handle.
 * @param gain The new RF gain: 0, -10, -20 or -30.
 * @retval  0  The new gain was successfully set.
 * @retval -1  Incorrect gain parameter.
 * @retval -2  Write error.
 * @retval -100  Invalid SDR-IQ handle.
 *
 * Set fixed RF gain according to 5.2.5 of the ICD.
 */
int sdriq_set_fixed_rf_gain(sdriq_t *sdr, int8_t gain)
{
    if (sdr == NULL)
        return -100;

    if (gain != 0 && gain != -10 && gain != -20 && gain != -30)
        return -1;

    return set_gain(sdr, 0x38, gain);
}

/**
 * Set fixed IF gain.
 *
 * @param sdr The SDR-IQ handle.
 * @param gain The new IF gain: 0, 6, 12, 18 or 24 dB.
 * @retval  0  The new gain was successfully set.
 * @retval -1  Incorrect gain parameter.
 * @retval -2  Write error.
 * @retval -100  Invalid SDR-IQ handle.
 *
 * Set fixed IF gain according to 5.2.6 of the ICD.
 */
int sdriq_set_fixed_if_gain(sdriq_t *sdr, int8_t gain)
{
    if (sdr == NULL)
        return -100;

    if (gain != 0 && gain != 6 && gain != 12 && gain != 18 && gain != 24)
        return -1;

    return set_gain(sdr, 0x40, gain);
}

/**
 * Return the number of samples available in the buffer.
 * 
 * @param sdr The SDR-IQ handle.
 * @returns The number og samples available.
 */
uint_fast32_t sdriq_get_num_samples(const sdriq_t *sdr)
{
    return (ring_buffer_count(sdr->rb) / 4);
}

/**
 * Get IQ samples.
 *
 * @param sdr The SDR-IQ handle.
 * @param buffer Pointer to a pre-allocated buffer with \ref num * 4 bytes.
 * @param num The number of samples to fetch.
 * @returns The actual number of samples copied into \ref buffer.
 */
uint_fast32_t sdriq_get_samples(sdriq_t *sdr, unsigned char *buffer, uint_fast32_t num)
{
    uint_fast32_t count = ring_buffer_count(sdr->rb) / 4;
    uint_fast32_t num_samples = (num <= count) ? num : count;

    ring_buffer_read(sdr->rb, buffer, num_samples*4);

    return num_samples;
}
