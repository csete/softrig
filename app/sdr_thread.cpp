/*
 * Main SDR sequencer thread
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017 Alexandru Csete OZ9AEC.
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
#include <QThread>

#include "interfaces/audio_output.h"
#include "nanosdr/common/time.h"
#include "nanosdr/interfaces/sdr_device.h"
#include "sdr_thread.h"

#if 1
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#define SDR_THREAD_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define SDR_THREAD_DEBUG(...)
#endif

SdrThread::SdrThread(QObject *parent) : QObject(parent)
{
    is_running = false;
    buflen = 30720;     // 20 msec @ 1.536 Msps

    sdr_dev = 0;
    rx = 0;
    input_samples = 0;
    output_samples = 0;
    aout_buffer = 0;
    resetStats();

    audio_out.init();

    thread = new QThread();
    moveToThread(thread);
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    connect(thread, SIGNAL(finished()), this, SLOT(thread_finished()));
    thread->setObjectName("SoftrigSdrThread\n");
    thread->start();
}

SdrThread::~SdrThread()
{
    if (is_running)
        stop();

    thread->requestInterruption();
    thread->quit();
    thread->wait(10000);
    delete thread;
}

int SdrThread::start(void)
{
    if (is_running)
        return SDR_THREAD_OK;

    SDR_THREAD_DEBUG("Starting SDR thread...\n");

    sdr_dev = sdr_device_create_rtlsdr();
    if (sdr_dev->init(1536000, "") != SDR_DEVICE_OK)
    {
        // FIXME: Emit error string
        return SDR_THREAD_EDEV;
    }
    sdr_dev->set_gain(SDR_DEVICE_RX_LNA_GAIN, 70);

    rx = new Receiver();
    rx->init(1536000, 48000, 100, buflen);

    is_running = true;

    resetStats();
    sdr_dev->start();
    audio_out.start();

    return SDR_THREAD_OK;
}

void SdrThread::stop(void)
{
    if (!is_running)
        return;

    SDR_THREAD_DEBUG("Stopping SDR thread...\n");

    stats.tstop = time_ms();
    SDR_THREAD_DEBUG("Receiver statistics:\n"
                     "  Time: %" PRIu64 " ms\n"
                     "  Samples in:  %" PRIu64 " samples = %" PRIu64 " sps\n"
                     "  Samples out: %" PRIu64 " samples = %" PRIu64 " sps\n",
                     stats.tstop - stats.tstart,
                     stats.samples_in,
                     (1000 * stats.samples_in) / (stats.tstop - stats.tstart),
                     stats.samples_out,
                     (1000 * stats.samples_out) / (stats.tstop - stats.tstart));

    audio_out.stop();

    sdr_dev->stop();
    delete sdr_dev;

    delete rx;

    is_running = false;

}

void SdrThread::process(void)
{
    quint32    samples_in = buflen;
    quint32    samples_read;
    int        samples_out;

    SDR_THREAD_DEBUG("SDR process entered\n");

    input_samples = new complex_t[buflen];
    output_samples = new real_t[buflen];
    aout_buffer = new qint16[buflen];

    while (!thread->isInterruptionRequested())
    {
        if (!is_running)
        {
            QThread::msleep(100);
            continue;
        }

        if (sdr_dev->get_num_samples() < samples_in)
        {
            QThread::usleep(2000);
            continue;
        }

        samples_read = sdr_dev->read_samples(input_samples, samples_in);
        if (samples_read == 0)
        {
            QThread::usleep(2000);
            continue;
        }
        stats.samples_in += samples_read;

        // TODO: Decimate
        // TODO: FFT
        samples_out = rx->process(samples_read, input_samples, output_samples);
        // TODO: SSI
        // NOTE: samples_out = -1 means SSI below squelch level

        if (samples_out > 0)
        {
            int     i;

            for (i = 0; i < samples_out; i++)
                aout_buffer[i] = (qint16)(32767.0f * output_samples[i]);

            audio_out.write((const char *) aout_buffer, samples_out * 2);
            stats.samples_out += samples_out;
        }
    }

    delete[] aout_buffer;
    delete[] output_samples;
    delete[] input_samples;
}

void SdrThread::thread_finished(void)
{
    SDR_THREAD_DEBUG("SDR thread finished\n");
}

void SdrThread::setRxFrequency(qint64 freq)
{
    if (!is_running)
        return;

    if (sdr_dev->set_freq(freq))
        SDR_THREAD_DEBUG("Error setting frequency\n");
}

void SdrThread::resetStats(void)
{
    stats.tstart = time_ms();
    stats.tstop = 0;
    stats.samples_in = 0;
    stats.samples_out = 0;
}
