/*
 * CTL queue.
 *
 * Copyright  2016  Alexandru Csete OZ9AEC
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
#include <new>          // std::nothrow

#include "sdr_ctl.h"
#include "sdr_ctl_queue.h"

SdrCtlQueue::SdrCtlQueue()
{
    head = 0;
    num = 0;
    size = 0;
    buffer = 0;
    overwrites = 0;
    overflows = 0;
}

SdrCtlQueue::~SdrCtlQueue()
{
    free_memory();
}

void SdrCtlQueue::free_memory(void)
{
    delete[] buffer;
    buffer = 0;
    head = 0;
    num = 0;
    size = 0;
}

int SdrCtlQueue::init(unsigned int n)
{
    if (n == 0)
        return SDR_CTL_QUEUE_ERROR;

    free_memory();
    buffer = new (std::nothrow) sdr_ctl_t[n];
    if (buffer == 0)
        return SDR_CTL_QUEUE_ERROR;

    size = n;
    return SDR_CTL_QUEUE_OK;
}

void SdrCtlQueue::clear(void)
{
    head = 0;
    num = 0;
}

int SdrCtlQueue::add_ctl(const sdr_ctl_t * ctl)
{
    if (num >= size)
    {
        overflows++;
        return SDR_CTL_QUEUE_FULL;
    }

    unsigned int    idx = head + num;
    if (idx >= size)
        idx = 0;

    buffer[idx] = *ctl;
    num++;

    return SDR_CTL_QUEUE_OK;
}

int SdrCtlQueue::get_ctl(sdr_ctl_t * ctl)
{
    if (!num)
        return SDR_CTL_QUEUE_EMPTY;

    *ctl = buffer[head++];
    num--;
    if (num == 0 || head >= size)
        head = 0;

    return SDR_CTL_QUEUE_OK;
}


#ifdef UNITTEST

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    SdrCtlQueue     queue;
    sdr_ctl_t       ctl;
    int             i;

    fputs("Unit test for sdr_ctl_queue.cpp\n", stderr);

    queue.init(4);

    for (i = 0; i < 20; i++)
    {
        ctl.type = i;
        ctl.id = 2 * i;
        queue.add_ctl(&ctl);
    }

    queue.get_ctl(&ctl);
    fprintf(stderr, "CTL TYPE = %u /  ID = %u\n", ctl.type, ctl.id);

    queue.get_ctl(&ctl);
    fprintf(stderr, "CTL TYPE = %u /  ID = %u\n", ctl.type, ctl.id);

    fprintf(stderr, "Overflows: %"PRIu64"\n", queue.get_overflows());

    return 0;
}

#endif
