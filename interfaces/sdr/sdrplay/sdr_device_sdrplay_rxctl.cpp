/*
 * SDRplay user interface
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2019 Alexandru Csete OZ9AEC
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
#include <QDebug>
#include <QString>

#include "sdr_device_sdrplay_rxctl.h"
#include "ui_sdr_device_sdrplay_rxctl.h"

SdrDeviceSdrplayRxctl::SdrDeviceSdrplayRxctl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SdrDeviceSdrplayRxctl)
{
    ui->setupUi(this);
    setProperty("desc", QString("SDRplay RSPduo"));
}

SdrDeviceSdrplayRxctl::~SdrDeviceSdrplayRxctl()
{
    delete ui;
}

void SdrDeviceSdrplayRxctl::setSystemGainValue(float gain)
{
    ui->gainValueLabel->setText(QString("%1 dB").arg(double(gain), 0, 'f', 1));
}

void SdrDeviceSdrplayRxctl::on_rfSlider_valueChanged(int value)
{
    int lna_state = ui->rfSlider->maximum() - value;

    ui->rfValueLabel->setText(QString("%1").arg(value));
    emit lnaStateChanged(lna_state);
}

void SdrDeviceSdrplayRxctl::on_ifSlider_valueChanged(int value)
{
    int grdb = -value;

    ui->ifValueLabel->setText(QString("%1 dB").arg(value));
    emit grdbChanged(grdb);
}

void SdrDeviceSdrplayRxctl::on_debugCheckBox_toggled(bool checked)
{
    emit debugChanged(checked);
}

void SdrDeviceSdrplayRxctl::readSettings(const sdrplay_settings_t &settings)
{
    ui->ifSlider->setValue(-settings.grdb);
    ui->rfSlider->setValue(ui->rfSlider->maximum() - settings.lna_state);
}
