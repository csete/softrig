/*
 * Audio output interface
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017 Alexandru Csete OZ9AEC.
 * All rights reserved.
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

    QAudioFormat    audio_out_format;
    audio_out_format.setCodec("audio/pcm");
    audio_out_format.setSampleRate(48000);
    audio_out_format.setChannelCount(1);
    audio_out_format.setSampleSize(16);
    audio_out_format.setSampleType(QAudioFormat::SignedInt);
    audio_out_format.setByteOrder(QAudioFormat::LittleEndian);
    if (!audio_out_format.isValid())
        AOUT_DEBUG("audio_out_format is invalid");

    audio_out = new QAudioOutput(audio_out_format, this);
    audio_out->setCategory("Softrig");

    initialized = true;
    return AUDIO_OUT_OK;
}
