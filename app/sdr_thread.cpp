/*
 * Main SDR sequencer thread
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017-2019 Alexandru Csete OZ9AEC.
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

#include "app_config.h"
#include "interfaces/audio_output.h"
#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"
#include "nanosdr/common/time.h"
#include "nanosdr/interfaces/sdr_device.h"
#include "nanosdr/fft_thread.h"
#include "nanosdr/nanodsp/filter/decimator.h"
#include "nanosdr/receiver.h"
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
    buflen_ms = 20;
    buflen = 0;
    decimation = 0;

    sdr_dev = nullptr;
    rx = nullptr;
    fft_data_buf = nullptr;
    fft_swap_buf = nullptr;
    input_samples = nullptr;
    output_samples = nullptr;
    aout_buffer = nullptr;
    resetStats();

    audio_out.init();

    fft = new FftThread();
    fft->init(FFT_SIZE, 25);

    thread = new QThread();
    moveToThread(thread);
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    connect(thread, SIGNAL(finished()), this, SLOT(thread_finished()));
    thread->setObjectName("SdrThread\n");
}

SdrThread::~SdrThread()
{
    if (is_running)
        stop();

    delete thread;
    delete fft;
}

int SdrThread::start(const app_config_t *conf)
{
    device_config_t     input_cfg;
    float               rx_rate;

    if (is_running)
        return SDR_THREAD_OK;

    SDR_THREAD_DEBUG("Starting SDR thread...\n");

    input_cfg = conf->input;
    if (input_cfg.type.isEmpty())
        return SDR_THREAD_EDEV;

    sdr_dev = sdr_device_create(input_cfg.type.toLatin1().data());
    if (!sdr_dev)
        return SDR_THREAD_EDEV;

    if (sdr_dev->init(input_cfg.rate, "") != SDR_DEVICE_OK)
    {
        // FIXME: Emit error string
        return SDR_THREAD_EDEV;
    }

    decimation = input_cfg.decimation;
    rx_rate = input_cfg.rate;
    if (decimation > 1)
    {
        decimation = input_decim.init(decimation, sdr_dev->get_dynamic_range());
        rx_rate /= (float)decimation;
    }

    buflen = buflen_ms * 1.e-3f * rx_rate;
    rx = new Receiver();
    rx->init(rx_rate, 48000, 100, buflen);

    is_running = true;

    // set frequency and correction
    setRxFrequency(input_cfg.frequency);
    // set bandwidth
    // set gain
    setRxGainMode(input_cfg.gain_mode);
    setRxGain(input_cfg.gain);

    // setup receiver
    setRxTuningOffset(input_cfg.nco);

    resetStats();
    audio_out.start();
    fft->start();
    sdr_dev->start();
    thread->start();

    return SDR_THREAD_OK;
}

void SdrThread::stop(void)
{
    if (!is_running)
        return;

    SDR_THREAD_DEBUG("Stopping SDR thread...\n");

    thread->requestInterruption();
    thread->quit();
    thread->wait(10000);

    stats.tstop = time_ms();
    /* *INDENT-OFF* */
    SDR_THREAD_DEBUG("Receiver statistics:\n"
                     "  Time: %" PRIu64 " ms\n"
                     "  Samples in:  %" PRIu64 " samples = %" PRIu64 " sps\n"
                     "  Samples out: %" PRIu64 " samples = %" PRIu64 " sps\n",
                     stats.tstop - stats.tstart,
                     stats.samples_in,
                     (1000 * stats.samples_in) / (stats.tstop - stats.tstart),
                     stats.samples_out,
                     (1000 * stats.samples_out) / (stats.tstop - stats.tstart));
    /* *INDENT-ON* */
    is_running = false;
    sdr_dev->stop();
    fft->stop();
    audio_out.stop();

    delete sdr_dev;
    delete rx;
}

void SdrThread::process(void)
{
    quint32    samples_in = buflen;
    quint32    samples_read;
    int        samples_out;

    SDR_THREAD_DEBUG("SDR thread started\n");

    fft_data_buf = new complex_t[FFT_SIZE];
    fft_swap_buf = new complex_t[FFT_SIZE];
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
        if (decimation > 1)
            samples_read = input_decim.process(samples_read, input_samples);

        fft->add_fft_input(samples_read, input_samples);
        samples_out = rx->process(samples_read, input_samples, output_samples);
        // TODO: SSI
        // NOTE: samples_out = -1 means SSI below squelch level

        if (samples_out > 0)
        {
            int    i;

            for (i = 0; i < samples_out; i++)
                aout_buffer[i] = (qint16)(32767.0f * output_samples[i]);

            audio_out.write((const char *) aout_buffer, samples_out * 2);
            stats.samples_out += samples_out;
        }
    }

    /* *INDENT-OFF* */
    delete[] fft_data_buf;
    delete[] fft_swap_buf;
    delete[] aout_buffer;
    delete[] output_samples;
    delete[] input_samples;
    /* *INDENT-ON* */
}

void SdrThread::thread_finished(void)
{
    SDR_THREAD_DEBUG("SDR thread finished\n");
}

void SdrThread::setRxFrequency(quint64 freq)
{
    if (!is_running)
        return;

    if (sdr_dev->set_freq(freq))
        SDR_THREAD_DEBUG("Error setting frequency\n");
}

void SdrThread::setRxGainMode(int mode)
{
    if (!is_running)
        return;

    if (sdr_dev->set_gain_mode(mode))
        SDR_THREAD_DEBUG("Error setting gain mode\n");
}

void SdrThread::setRxGain(int gain)
{
    if (!is_running)
        return;

    if (sdr_dev->set_gain(gain))
        SDR_THREAD_DEBUG("Error setting gain\n");
}

void SdrThread::setDemod(sdr_demod_t demod)
{
    if (!is_running)           // FIXME
        return;

    rx->set_demod(demod);
}

void SdrThread::setRxFilter(real_t low_cut, real_t high_cut)
{
    if (!is_running)           // FIXME
        return;

    rx->set_filter(low_cut, high_cut);
}

void SdrThread::setRxTuningOffset(real_t offset)
{
    if (!is_running)           // FIXME
        return;

    rx->set_tuning_offset(offset);
}

void SdrThread::setRxCwOffset(real_t offset)
{
    if (!is_running)           // FIXME
        return;

    rx->set_cw_offset(offset);
}

void SdrThread::resetStats(void)
{
    stats.tstart = time_ms();
    stats.tstop = 0;
    stats.samples_in = 0;
    stats.samples_out = 0;
}

quint32 SdrThread::getFftData(real_t *fft_data_out)
{
    quint32    fft_samples;
    quint32    i;
    real_t     pwr;

    if ((fft_samples = fft->get_fft_output(fft_data_buf)) == 0)
        return 0;

    // shift buffer
    int    cidx = fft_samples / 2;
    memcpy(fft_swap_buf, &fft_data_buf[cidx], sizeof(complex_t) * cidx);
    memcpy(&fft_swap_buf[cidx], fft_data_buf, sizeof(complex_t) * cidx);

#define FFT_PWR_SCALE   (1.0f / ((float)FFT_SIZE * (float)FFT_SIZE))
    for (i = 0; i < fft_samples; i++)
    {
        pwr = FFT_PWR_SCALE * (fft_swap_buf[i].im * fft_swap_buf[i].im +
                               fft_swap_buf[i].re * fft_swap_buf[i].re);
        fft_data_out[i] = 10.f * log10f(pwr + 1.0e-20f);
    }

    return fft_samples;
}

float SdrThread::getSignalStrength(void)
{
    return rx->get_signal_strength();
}
