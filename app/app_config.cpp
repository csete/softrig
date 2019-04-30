/*
 * Configuration backend
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

#include <new>          // std::nothrow
#include <QDebug>
#include <QSettings>

#include "app_config.h"

#define CONFIG_VERSION  1

#define APP                "app"
#define APP_CFG_VER         APP"/config_version"

#define SDR_INPUT           "sdr_input"
#define SDR_INPUT_TYPE      SDR_INPUT"/type"
#define SDR_INPUT_FREQ      SDR_INPUT"/frequency"
#define SDR_INPUT_NCO       SDR_INPUT"/nco"
#define SDR_INPUT_LNB       SDR_INPUT"/transverter"
#define SDR_INPUT_RATE      SDR_INPUT"/sample_rate"
#define SDR_INPUT_DECIM     SDR_INPUT"/decimation"
#define SDR_INPUT_BW        SDR_INPUT"/bandwidth"
#define SDR_INPUT_CORR      SDR_INPUT"/frequency_correction"
#define SDR_INPUT_GAIN_MODE SDR_INPUT"/gain_mode"
#define SDR_INPUT_GAIN      SDR_INPUT"/gain"

#define DEFAULT_FREQ 145500000
#define DEFAULT_GAIN 50

AppConfig::AppConfig()
{
    settings = nullptr;
}

AppConfig::~AppConfig()
{
    close();
}

int AppConfig::load(const QString &filename)
{
    if (filename.isEmpty()) // FIXME: check that it is a correct path
        return APP_CONFIG_EINVAL;

    close();

    settings = new (std::nothrow) QSettings(filename, QSettings::IniFormat);
    if (!settings)
        return APP_CONFIG_EFILE;

    app_config.version = settings->value(APP_CFG_VER, CONFIG_VERSION).toUInt();
    readDeviceConf();

    return APP_CONFIG_OK;
}

void AppConfig::save(void)
{
    if (!settings)
        return;

    settings->setValue(APP_CFG_VER, app_config.version);
    saveDeviceConf();
    settings->sync();
}

void AppConfig::close(void)
{
    if (!settings)
        return;

    settings->sync();
    delete settings;
    settings = nullptr;
}

void AppConfig::readDeviceConf(void)
{
    device_config_t     *input = &app_config.input;

    input->type = settings->value(SDR_INPUT_TYPE, "").toString();
    input->frequency = settings->value(SDR_INPUT_FREQ, DEFAULT_FREQ).toULongLong();
    input->nco = settings->value(SDR_INPUT_NCO, 0).toLongLong();
    input->transverter = settings->value(SDR_INPUT_LNB, 0).toLongLong();
    input->rate = settings->value(SDR_INPUT_RATE, 0).toUInt();
    input->decimation = settings->value(SDR_INPUT_DECIM, 1).toUInt();
    input->bandwidth = settings->value(SDR_INPUT_BW, 0).toUInt();
    input->freq_corr_ppb = settings->value(SDR_INPUT_CORR, 0).toInt();
    input->gain_mode = settings->value(SDR_INPUT_GAIN_MODE, 0).toInt();
    input->gain = settings->value(SDR_INPUT_GAIN, DEFAULT_GAIN).toInt();
}

void AppConfig::saveDeviceConf(void)
{
    device_config_t     *input = &app_config.input;

    if (input->type.isEmpty())
        settings->remove(SDR_INPUT_TYPE);
    else
        settings->setValue(SDR_INPUT_TYPE, input->type);

    if (input->frequency != DEFAULT_FREQ)
        settings->setValue(SDR_INPUT_FREQ, input->frequency);
    else
        settings->remove(SDR_INPUT_FREQ);

    if (input->nco)
        settings->setValue(SDR_INPUT_NCO, input->nco);
    else
        settings->remove(SDR_INPUT_NCO);

    if (input->transverter)
        settings->setValue(SDR_INPUT_LNB, input->transverter);
    else
        settings->remove(SDR_INPUT_LNB);

    if (input->rate)
        settings->setValue(SDR_INPUT_RATE, input->rate);
    else
        settings->remove(SDR_INPUT_RATE);

    if (input->decimation < 2)
        settings->remove(SDR_INPUT_DECIM);
    else
        settings->setValue(SDR_INPUT_DECIM, input->decimation);

    if (input->bandwidth)
        settings->setValue(SDR_INPUT_BW, input->bandwidth);
    else
        settings->remove(SDR_INPUT_BW);

    if (input->freq_corr_ppb)
        settings->setValue(SDR_INPUT_CORR, input->freq_corr_ppb);
    else
        settings->remove(SDR_INPUT_CORR);

    if (input->gain_mode)
        settings->setValue(SDR_INPUT_GAIN_MODE, input->gain_mode);
    else
        settings->remove(SDR_INPUT_GAIN_MODE);

    if (input->gain != DEFAULT_GAIN)
        settings->setValue(SDR_INPUT_GAIN, input->gain);
    else
        settings->remove(SDR_INPUT_GAIN);
}

