/*
 * RTL-SDR backend helper functions to wrap the rtlsdr_read_async() interface
 * into something that we can use in our C++ input reader object.
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
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/library_loader.h"
#include "common/ring_buffer.h"
#include "sdr_device_rtlsdr_reader.h"

// RTLSDR API functions loaded in sdr_device_rtlsdr.cpp
typedef void (*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len,
                                       void *ctx);
static uint32_t (*rtlsdr_get_sample_rate)(void *dev);
static int (*rtlsdr_cancel_async)(void *dev);
static int (*rtlsdr_reset_buffer)(void *dev);
static int (*rtlsdr_read_async)(void *dev, rtlsdr_read_async_cb_t cb, void *ctx,
                                uint32_t buf_num, uint32_t buf_len);

struct _rtlsdr_reader {
    pthread_t        thread;  // Reader thread ID
    pthread_rwlock_t rwlock;  // Read/write lock
    ring_buffer_t *  rb;  // Ring buffer used to store 100 msec of I/Q samples

    void *dev;  // Local pointer to the rtlsdr handle (owned by parent object)
    int   running;  // Flag indicating whether the reader thread is running
    int   exiting;  // Flag indicating whether we are exiting the reader. If
                  // read_async() returns and this flag is false, a device error
                  // must have occured
};

/*
 * Rtlsdr reader callback.
 *
 * Parameters:
 *   buf    Pointer to the buffer holding the input samples.
 *   len    The number of bytes in the input buffer. Equals buf_len from the
 *          rtlsdr_read_async() call.
 *   data   Pointer to the rtlsdr reader handle.
 */
static void rtlsdr_reader_cb(unsigned char *buf, uint32_t len, void *data)
{
    if (data)
    {
        rtlsdr_reader_t *reader;
        uint32_t         bytes_to_write;

        reader = (rtlsdr_reader_t *)data;
        if (reader->exiting)
            return;

        /* ensure we don't overflow ring buffer */
        bytes_to_write = ring_buffer_size(reader->rb);
        if (len < bytes_to_write)
            bytes_to_write = len;
        else
            fprintf(stderr, "Trying to write %u bytes into buffer of size %u\n",
                    len, bytes_to_write);

        pthread_rwlock_wrlock(&reader->rwlock);
        ring_buffer_write(reader->rb, buf, bytes_to_write);
        pthread_rwlock_unlock(&reader->rwlock);
    }
}

/*
 * Rtlsdr reader thread function.
 * data is a pointer to the rtlsdr reader handle.
 */
static void *rtlsdr_reader_thread(void *data)
{
    rtlsdr_reader_t *reader = (rtlsdr_reader_t *)data;
    uint32_t         sample_rate;
    uint32_t         buf_len;

    fputs("Rtlsdr reader thread started.\n", stderr);

    /* aim for 20-40 ms buffers but in multiples of 16k */
    sample_rate = rtlsdr_get_sample_rate(reader->dev);
    if (sample_rate < 1e6)
        buf_len = 16384;
    else if (sample_rate < 2e6)
        buf_len = 4 * 16384;
    else
        buf_len = 6 * 16384;

    rtlsdr_reset_buffer(reader->dev);
    rtlsdr_read_async(reader->dev, rtlsdr_reader_cb, reader, 0, buf_len);

    if (reader->exiting)
        fputs("Exiting rtlsdr reader...\n", stderr);
    else
        fputs("Exiting rtlsdr reader because of a device error...\n", stderr);

    pthread_exit(NULL);
}

rtlsdr_reader_t *rtlsdr_reader_create(void *rtldev, lib_handle_t lib)
{
    rtlsdr_reader_t *reader;
    uint32_t         sample_rate;

    /* clang-format off */
    /* NB:
     * The assignment used below is the POSIX.1-2003 (Technical
     * Corrigendum 1) workaround for the "ISO C forbids conversion of object
     * pointer to function pointer type" earning when using -Wpedantic with gcc;
     * See the Rationale for the POSIX specification of dlsym().
     * See https://linux.die.net/man/3/dlsym
     * This is not a problem in C++
     */
    *(void **)(&rtlsdr_get_sample_rate) = get_symbol(lib, "rtlsdr_get_sample_rate");
    *(void **)(&rtlsdr_cancel_async) = get_symbol(lib, "rtlsdr_cancel_async");
    *(void **)(&rtlsdr_reset_buffer) = get_symbol(lib, "rtlsdr_reset_buffer");
    *(void **)(&rtlsdr_read_async) = get_symbol(lib, "rtlsdr_read_async");
    /* clang-format on */

    if (rtlsdr_get_sample_rate == NULL || rtlsdr_cancel_async == NULL ||
        rtlsdr_reset_buffer == NULL || rtlsdr_read_async == NULL)
        return NULL;

    reader = (rtlsdr_reader_t *)malloc(sizeof(rtlsdr_reader_t));
    reader->dev = rtldev;
    reader->running = 0;
    reader->exiting = 0;
    pthread_rwlock_init(&reader->rwlock, NULL);

    /* Initialise input buffer to 100 ms or 64k if sample rate < 1M */
    sample_rate = rtlsdr_get_sample_rate(reader->dev);
    reader->rb = ring_buffer_create();
    if (sample_rate > 1000000)
        ring_buffer_init(reader->rb, sample_rate / 5); /* 2 bytes per sample */
    else
        ring_buffer_init(reader->rb, 65536);

    return reader;
}

void rtlsdr_reader_destroy(rtlsdr_reader_t *reader)
{
    if (!reader)
        return;

    if (reader->running)
        rtlsdr_reader_stop(reader);

    ring_buffer_delete(reader->rb);
    pthread_rwlock_destroy(&reader->rwlock);
    free(reader);
}

int rtlsdr_reader_start(rtlsdr_reader_t *reader)
{
    int ret;

    ret = pthread_create(&reader->thread, NULL, rtlsdr_reader_thread, reader);
    if (ret)
    {
        fprintf(stderr, "Error creating rtlsdr reader thread: %d\n", ret);
        return ret;
    }

    reader->running = 1;
    reader->exiting = 0;

    return 0;
}

int rtlsdr_reader_stop(rtlsdr_reader_t *reader)
{
    reader->exiting = 1;
    rtlsdr_cancel_async(reader->dev);
    reader->running = 0;
    return 0;
}

uint32_t rtlsdr_reader_get_num_bytes(rtlsdr_reader_t *reader)
{
    return ring_buffer_count(reader->rb);
}

uint32_t rtlsdr_reader_read_bytes(rtlsdr_reader_t *reader, void *buffer,
                                  uint32_t bytes)
{
    if (bytes > ring_buffer_count(reader->rb))
        return 0;

    pthread_rwlock_rdlock(&reader->rwlock);
    ring_buffer_read(reader->rb, (unsigned char *)buffer, bytes);
    pthread_rwlock_unlock(&reader->rwlock);

    return bytes;
}
