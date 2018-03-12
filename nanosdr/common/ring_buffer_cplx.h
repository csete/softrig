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

#include "datatypes.h"
#include "ring_buffer.h"


#define ELEMENT_SIZE    sizeof(complex_t)

/*
 * Ring buffer API for data of type complex_t.
 *
 * This file adds a complex_t wrapper around the basic ring buffer
 * implementation.
 */

static inline ring_buffer_t * ring_buffer_cplx_create(void)
{
    return ring_buffer_create();
}

static inline void ring_buffer_cplx_delete(ring_buffer_t * rb)
{
    ring_buffer_delete(rb);
}

static inline void ring_buffer_cplx_init(ring_buffer_t * rb,
                                         uint_fast32_t size)
{
    ring_buffer_init(rb, size * ELEMENT_SIZE);
}

static inline void ring_buffer_cplx_resize(ring_buffer_t * rb,
                                           uint_fast32_t newsize)
{
    ring_buffer_resize(rb, newsize * ELEMENT_SIZE);
}

static inline int ring_buffer_cplx_is_full(ring_buffer_t * rb)
{
    return ring_buffer_is_full(rb);
}

static inline int ring_buffer_cplx_is_empty(ring_buffer_t * rb)
{
    return ring_buffer_is_empty(rb);
}

static inline uint_fast32_t ring_buffer_cplx_count(ring_buffer_t * rb)
{
    return ring_buffer_count(rb) / ELEMENT_SIZE;
}

static inline uint_fast32_t ring_buffer_cplx_size(ring_buffer_t * rb)
{
    return ring_buffer_size(rb) / ELEMENT_SIZE;
}

static inline void ring_buffer_cplx_write(ring_buffer_t * rb,
                                          const complex_t * src,
                                          uint_fast32_t num)
{
    ring_buffer_write(rb, (const unsigned char *)src, num * ELEMENT_SIZE);
}

static inline void ring_buffer_cplx_read(ring_buffer_t * rb, complex_t * dest,
                                         uint_fast32_t num)
{
    ring_buffer_read(rb, (unsigned char *) dest, num * ELEMENT_SIZE);
}

static inline void ring_buffer_cplx_clear(ring_buffer_t * rb)
{
    ring_buffer_clear(rb);
}

