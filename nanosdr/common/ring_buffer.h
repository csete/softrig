/*
 * Simple ring buffer for nanosdr.
 * 
 * Copyright 2015 Alexandru Csete OZ9AEC
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Simple ring buffer implementation.
 *
 * This file implements a simple byte FIFO buffer of a fixed size.
 *
 * The present implementation does not contain any boundary checks and the
 * callers must ensure that
 *
 * 1. The number of elemets written is less than or equal to the buffer size
 * 2. The umber of elements read from the buffer is less than or equal to the
 *    number of elements currently in the buffer.
 *
 * Furthermore, the buffer does not check how much empty space there is during
 * write operations, i.e. old data will be overwritten if writing happens
 * faster than reading.
 */

typedef struct {
    uint_fast32_t   size;
    uint_fast32_t   start;  /* index of the oldest element */
    uint_fast32_t   count;  /* number of elements in the buffer */
    unsigned char  *buffer;
} ring_buffer_t;

static inline ring_buffer_t * ring_buffer_create(void)
{
    return (ring_buffer_t *) malloc(sizeof(ring_buffer_t));
}

static inline void ring_buffer_delete(ring_buffer_t * rb)
{
    if (rb)
    {
        if (rb->buffer)
            free(rb->buffer);
        free(rb);
    }
}

static inline void ring_buffer_init(ring_buffer_t * rb, uint_fast32_t size)
{
    rb->size = size;
    rb->start = 0;
    rb->count = 0;
    rb->buffer = (unsigned char *) malloc(rb->size);
}

static inline void ring_buffer_resize(ring_buffer_t * rb,
                                      uint_fast32_t newsize)
{
    free(rb->buffer);
    ring_buffer_init(rb, newsize);
}

static inline int ring_buffer_is_full(ring_buffer_t * rb)
{
    return (rb->count == rb->size);
}

static inline int ring_buffer_is_empty(ring_buffer_t * rb)
{
    return (rb->count == 0);
}

static inline uint_fast32_t ring_buffer_count(ring_buffer_t * rb)
{
    return rb->count;
}

static inline uint_fast32_t ring_buffer_size(ring_buffer_t * rb)
{
    return rb->size;
}

static inline void ring_buffer_write(ring_buffer_t * rb, const unsigned char * src,
                                     uint_fast32_t num)
{
    if (!num)
        return;

    /* write pointer to first available slot */
    uint_fast32_t   wp = (rb->start + rb->count) % rb->size;

    /* new write pointer after successful write */
    uint_fast32_t   new_wp = (wp + num) % rb->size;

    if (new_wp > wp)
    {
        /* we can write in a single pass */
        memcpy(&rb->buffer[wp], src, num);
    }
    else
    {
        /* we need to wrap around the end */
        memcpy(&rb->buffer[wp], src, num - new_wp);
        memcpy(rb->buffer, &src[num - new_wp], new_wp);
    }

    /* Update count and check if overwrite has occurred, if yes, adjust
     * count and read pointer (rb->start)
     */
    rb->count += num;
    if (rb->count > rb->size)
    {
        rb->count = rb->size;
        rb->start = new_wp;
    }
}

static inline void ring_buffer_read(ring_buffer_t * rb, unsigned char * dest,
                                    uint_fast32_t num)
{
    if (!num)
        return;

    /* index of the last value to read */
    uint_fast32_t   end = (rb->start + num - 1) % rb->size;

    if (end < rb->start)
    {
        /* we need to read in two passes */
        uint_fast32_t   split = rb->size - rb->start;

        memcpy(dest, &rb->buffer[rb->start], split);
        memcpy(&dest[split], rb->buffer, end + 1);
    }
    else
    {
        /* can read in a single pass */
        memcpy(dest, &rb->buffer[rb->start], num);
    }

    rb->count -= num;
    rb->start = (end + 1) % rb->size;
}

static inline void ring_buffer_clear(ring_buffer_t * rb)
{
    rb->start = 0;
    rb->count = 0;
}
