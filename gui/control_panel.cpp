/*
 * Main control panel
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
#include <new>
#include <QDebug>

#include "control_panel.h"
#include "nanosdr/common/sdr_data.h"
#include "ui_control_panel.h"

#define CP_MODE_NONE    0
#define CP_MODE_AM      1
#define CP_MODE_SAM     2
#define CP_MODE_LSB     3
#define CP_MODE_USB     4
#define CP_MODE_CW      5
#define CP_MODE_FM      6      // Voice FM, 5.0 kHz
#define CP_MODE_FMN     7      // Voice FM, 2.5 kHz
#define CP_MODE_NUM     8

ControlPanel::ControlPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanel)
{
    current_mode = CP_MODE_NONE;
    last_ssb_mode = CP_MODE_USB;

    ui->setupUi(this);
    initModeSettings();
}

ControlPanel::~ControlPanel()
{
    delete mode_settings;
    delete ui;
}

void ControlPanel::initModeSettings(void)
{
    mode_settings = new(std::nothrow) mode_setting[CP_MODE_NUM];
    if (!mode_settings)
        qFatal("%s: Failed to allocate memory for mode array. Aborting.\n",
               __func__);

    mode_settings[CP_MODE_NONE].demod = SDR_DEMOD_NONE;
    mode_settings[CP_MODE_NONE].filter_lo = 0.0;
    mode_settings[CP_MODE_NONE].filter_hi = 0.0;
    mode_settings[CP_MODE_NONE].cw_offset = 0.0;
    mode_settings[CP_MODE_AM].demod = SDR_DEMOD_AM;
    mode_settings[CP_MODE_AM].filter_lo = -5000.0;
    mode_settings[CP_MODE_AM].filter_hi = 5000.0;
    mode_settings[CP_MODE_AM].cw_offset = 0.0;
    mode_settings[CP_MODE_SAM].demod = SDR_DEMOD_AM;
    mode_settings[CP_MODE_SAM].filter_lo = -5000.0;
    mode_settings[CP_MODE_SAM].filter_hi = 5000.0;
    mode_settings[CP_MODE_SAM].cw_offset = 0.0;
    mode_settings[CP_MODE_LSB].demod = SDR_DEMOD_SSB;
    mode_settings[CP_MODE_LSB].filter_lo = -2800.0;
    mode_settings[CP_MODE_LSB].filter_hi = -100.0;
    mode_settings[CP_MODE_LSB].cw_offset = 0.0;
    mode_settings[CP_MODE_USB].demod = SDR_DEMOD_SSB;
    mode_settings[CP_MODE_USB].filter_lo = 100.0;
    mode_settings[CP_MODE_USB].filter_hi = 2800.0;
    mode_settings[CP_MODE_USB].cw_offset = 0.0;
    mode_settings[CP_MODE_CW].demod = SDR_DEMOD_SSB;
    mode_settings[CP_MODE_CW].filter_lo = -400.0;
    mode_settings[CP_MODE_CW].filter_hi = 400.0;
    mode_settings[CP_MODE_CW].cw_offset = 700.0;
    mode_settings[CP_MODE_FM].demod = SDR_DEMOD_FM;
    mode_settings[CP_MODE_FM].filter_lo = -10000.0;
    mode_settings[CP_MODE_FM].filter_hi = 10000.0;
    mode_settings[CP_MODE_FM].cw_offset = 0.0;
    mode_settings[CP_MODE_FMN].demod = SDR_DEMOD_FM;
    mode_settings[CP_MODE_FMN].filter_lo = -7500.0;
    mode_settings[CP_MODE_FMN].filter_hi = 7500.0;
    mode_settings[CP_MODE_FMN].cw_offset = 0.0;
}

void ControlPanel::updateMode(quint8 mode)
{
    Q_ASSERT(mode < CP_MODE_NUM);
    qDebug("%s: %u -> %u", __func__, current_mode, mode);

    current_mode = mode;
    emit    demodChanged(mode_settings[mode].demod);
    emit    filterChanged(mode_settings[mode].filter_lo,
                          mode_settings[mode].filter_hi);
    emit    cwOffsetChanged(mode_settings[mode].cw_offset);
}

void ControlPanel::on_amButton_clicked(bool)
{
    updateMode(CP_MODE_AM);
}

void ControlPanel::on_ssbButton_clicked(bool)
{
    quint8    new_mode;

    if (current_mode == CP_MODE_LSB)
        new_mode = last_ssb_mode = CP_MODE_USB;
    else if (current_mode == CP_MODE_USB)
        new_mode = last_ssb_mode = CP_MODE_LSB;
    else
        new_mode = last_ssb_mode;

    updateMode(new_mode);
}

void ControlPanel::on_cwButton_clicked(bool)
{
    updateMode(CP_MODE_CW);
}

void ControlPanel::on_fmButton_clicked(bool)
{
    updateMode(CP_MODE_FM);
}
