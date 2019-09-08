/*
 * BladeRF receiver user interface
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

#include "sdr_device_bladerf_rxctl.h"
#include "ui_sdr_device_bladerf_rxctl.h"

SdrDeviceBladerfRxctl::SdrDeviceBladerfRxctl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SdrDeviceBladerfRxctl)
{
    ui->setupUi(this);
    setProperty("desc", QString("BladeRF 2.0 micro"));

    ui->gainModeLabel->setVisible(false);
    ui->gainModeCombo->setVisible(false);
    ui->bias1Box->setVisible(false);
    ui->bias2Box->setVisible(false);
}

SdrDeviceBladerfRxctl::~SdrDeviceBladerfRxctl()
{
    delete ui;
}

void SdrDeviceBladerfRxctl::on_gainSlider_valueChanged(int value)
{
    ui->gainValueLabel->setText(QString("%1 dB").arg(value));
    emit gainChanged(value);
}

void SdrDeviceBladerfRxctl::on_bias1Box_toggled(bool checked)
{
    emit biasChanged(BLADERF_CHANNEL_RX(0), checked);
}

void SdrDeviceBladerfRxctl::on_bias2Box_toggled(bool checked)
{
    emit biasChanged(BLADERF_CHANNEL_RX(1), checked);
}

void SdrDeviceBladerfRxctl::readSettings(const bladerf_settings_t settings)
{
    ui->gainSlider->setValue(settings.rx_gain);
}
