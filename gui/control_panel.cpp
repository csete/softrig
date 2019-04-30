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

// page indices in stacked widget
#define PAGE_IDX_RX_OPT     0
#define PAGE_IDX_TX_OPT     1
#define PAGE_IDX_DISP_OPT   2


ControlPanel::ControlPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanel)
{
    current_mode = CP_MODE_NONE;
    last_ssb_mode = CP_MODE_USB;

    ui->setupUi(this);
    initModeSettings();

    ui->rxFilterBox->setEnabled(false);

    {
        QFont    font;
        font.setPointSize(14);
        ui->minLabel->setFont(font);
        ui->maxLabel->setFont(font);
        font.setPointSize(16);
        font.setWeight(QFont::Bold);
        ui->rmsLabel->setFont(font);

        stats.reset = true;
    }
}

ControlPanel::~ControlPanel()
{
    delete mode_settings;
    delete ui;
}

void ControlPanel::readSettings(const app_config_t &conf)
{
    ui->rxGainMode->setCurrentIndex(conf.input.gain_mode);
    ui->rxGainSlider->setValue(conf.input.gain);
}

void ControlPanel::saveSettings(app_config_t &conf)
{
    conf.input.gain_mode = ui->rxGainMode->currentIndex();
    conf.input.gain = ui->rxGainSlider->value();
}


void ControlPanel::addSignalData(double rms)
{
    if (stats.reset)
    {
        stats.min = rms;
        stats.max = rms;
        stats.rms = rms;
        stats.num = 1;
        stats.reset = false;
        stats.timer.restart();
    }
    else
    {
        stats.num++;

        if (rms < stats.min)
            stats.min = rms;

        if (rms > stats.max)
            stats.max = rms;

        stats.rms += rms;
    }

    if (stats.timer.elapsed() >= 1000 * ui->avgSpinBox->value())
    {
        ui->minLabel->setText(QString("Min: %1 dB").arg(stats.min, 0, 'f', 1));
        ui->maxLabel->setText(QString("Max: %1 dB").arg(stats.max, 0, 'f', 1));
        ui->rmsLabel->setText(QString("RMS: %1 dB").arg(stats.rms / double(stats.num), 0, 'f', 1));
        stats.reset = true;
    }
}

void ControlPanel::initModeSettings(void)
{
    mode_settings = new(std::nothrow) mode_setting[CP_MODE_NUM];
    if (!mode_settings)
        qFatal("ControlPanel::initModeSettings: Failed to allocate memory for mode array. Aborting.\n");

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
    mode_settings[CP_MODE_CW].filter_lo = -250.0;
    mode_settings[CP_MODE_CW].filter_hi = 250.0;
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
    qDebug("ControlPanel::updateMode: %u -> %u", current_mode, mode);

    current_mode = mode;
    emit    demodChanged(mode_settings[mode].demod);
    emit    filterChanged(mode_settings[mode].filter_lo,
                          mode_settings[mode].filter_hi);
    emit    cwOffsetChanged(mode_settings[mode].cw_offset);
}

void ControlPanel::on_rxButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    ui->stackedWidget->setCurrentIndex(PAGE_IDX_RX_OPT);
}

void ControlPanel::on_txButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    ui->stackedWidget->setCurrentIndex(PAGE_IDX_TX_OPT);
}

void ControlPanel::on_dispButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    ui->stackedWidget->setCurrentIndex(PAGE_IDX_DISP_OPT);
}

void ControlPanel::on_recButton_clicked(bool checked)
{
    qDebug() << __func__ << checked;
}

void ControlPanel::on_rxGainMode_activated(int index)
{
    emit rxGainModeChanged(index);
}

void ControlPanel::on_rxGainSlider_valueChanged(int value)
{
    emit rxGainChanged(value);
}


void ControlPanel::on_amButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    updateMode(CP_MODE_AM);
}

void ControlPanel::on_ssbButton_clicked(bool checked)
{
    Q_UNUSED(checked);

    quint8    new_mode;

    if (current_mode == CP_MODE_LSB)
        new_mode = last_ssb_mode = CP_MODE_USB;
    else if (current_mode == CP_MODE_USB)
        new_mode = last_ssb_mode = CP_MODE_LSB;
    else
        new_mode = last_ssb_mode;

    updateMode(new_mode);
}

void ControlPanel::on_cwButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    updateMode(CP_MODE_CW);
}

void ControlPanel::on_fmButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    updateMode(CP_MODE_FM);
}
