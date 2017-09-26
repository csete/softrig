/*
 * Audio output interface
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
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QIODevice>
#include <QObject>

#include "audio_output.h"

#if 1
#include <stdio.h>
#define AOUT_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define AOUT_DEBUG(...)
#endif

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    initialized = false;
    audio_out = 0;
    audio_buffer = 0;
}

AudioOutput::~AudioOutput()
{
    if (initialized)
    {
        delete audio_out;
    }
}

// initialize audio output using defaults
int AudioOutput::init(void)
{
    if (initialized)
        return AUDIO_OUT_OK;

    QAudioFormat    aout_format;
    aout_format.setCodec("audio/pcm");
    aout_format.setSampleRate(48000);
    aout_format.setChannelCount(1);
    aout_format.setSampleSize(16);
    aout_format.setSampleType(QAudioFormat::SignedInt);
    aout_format.setByteOrder(QAudioFormat::LittleEndian);
    if (!aout_format.isValid())
    {
        AOUT_DEBUG("audio_out_format is invalid\n");
        return AUDIO_OUT_EFORMAT;
    }

    QAudioDeviceInfo    aout_info(QAudioDeviceInfo::defaultOutputDevice());
    if (!aout_info.isFormatSupported(aout_format))
    {
        AOUT_DEBUG("audio_out_format not supported\n");
        return AUDIO_OUT_EFORMAT;
    }

    audio_out = new QAudioOutput(aout_format, this);
    audio_out->setCategory("Softrig");
    audio_out->setVolume(1.0);
    connect(audio_out, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(aoutStateChanged(QAudio::State)));

    initialized = true;

    return AUDIO_OUT_OK;
}

int AudioOutput::start(void)
{
    if (!initialized)
        return AUDIO_OUT_EINIT;

    // FIXME: Try pull mode for lower latency
    audio_buffer = audio_out->start();

    return AUDIO_OUT_OK;
}

int AudioOutput::stop(void)
{
    if (!initialized)
        return AUDIO_OUT_EINIT;

    audio_out->stop();

    return AUDIO_OUT_OK;
}

int AudioOutput::write(const char * data, qint64 len)
{
    Q_ASSERT(initialized);
    Q_ASSERT(len > 0);

    if (len != audio_buffer->write(data, len))
        return AUDIO_OUT_EBUFWR;

    return AUDIO_OUT_OK;
}

void AudioOutput::aoutStateChanged(QAudio::State new_state)
{
    switch (new_state)
    {
    case QAudio::ActiveState:
        AOUT_DEBUG("Audio output entered ACTIVE state\n");
        break;

    case QAudio::SuspendedState:
        AOUT_DEBUG("Audio output entered SUSPENDED state\n");
        break;

    case QAudio::StoppedState:
        AOUT_DEBUG("Audio output entered STOPPED state\n");
        // Stopped for other reasons
        if (audio_out->error() != QAudio::NoError)
        {
            AOUT_DEBUG("Audio output error %d\n", audio_out->error());
        }
        break;

    case QAudio::IdleState:
        // no data in buffer
        AOUT_DEBUG("Audio output entered IDLE state\n");
        break;

    default:
        AOUT_DEBUG("Unknown audio output state: %d", new_state);
        break;
    }
}
