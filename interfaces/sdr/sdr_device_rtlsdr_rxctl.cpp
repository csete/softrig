/*
 * RTL-SDR user interface
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2018-2019 Alexandru Csete OZ9AEC
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
#include <QString>

#include "sdr_device_rtlsdr_rxctl.h"
#include "ui_sdr_device_rtlsdr_rxctl.h"


static const char *DS_COMBO_STR[] = {
    "Auto Q", "Auto I", "On Q", "On I", "Off"
};

SdrDeviceRtlsdrRxCtl::SdrDeviceRtlsdrRxCtl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SdrDeviceRtlsdrRxCtl)
{
    ui->setupUi(this);
    setProperty("desc", QString("RTL-SDR"));

    for (int i = 0; i < RXCTL_DS_MODE_NUM; i++)
        ui->dsCombo->addItem(QString(tr(DS_COMBO_STR[i])));
}

SdrDeviceRtlsdrRxCtl::~SdrDeviceRtlsdrRxCtl()
{
    delete ui;
}

void SdrDeviceRtlsdrRxCtl::setTunerGains(int * values, int count)
{
    float   gain;
    int     i;

    if (!values || count <= 0)
        return;

    ui->gainSlider->setRange(0, count - 1);
    gains.clear();
    for (i = 0; i < count; i++)
    {
        gain = 0.1f * float(values[i]);
        gains.append(gain);
    }
}

void SdrDeviceRtlsdrRxCtl::readSettings(rtlsdr_settings_t &settings)
{
    setGain(settings.gain);
    ui->dsCombo->setCurrentIndex(settings.ds_mode);
    ui->biasButton->setChecked(settings.bias_on);
    ui->agcButton->setChecked(settings.agc_on);
}

void SdrDeviceRtlsdrRxCtl::on_gainSlider_valueChanged(int index)
{
    float   gain;

    if (gains.isEmpty() || index > gains.count())
        return;

    gain = gains[index];
    ui->gainValueLabel->setText(QString("%1 dB").arg(double(gain), 5, 'f', 1));
    emit gainChanged(int(10.f * gain));
}

void SdrDeviceRtlsdrRxCtl::on_biasButton_toggled(bool bias_on)
{
    emit biasToggled(bias_on);
}

void SdrDeviceRtlsdrRxCtl::on_agcButton_toggled(bool agc_on)
{
    ui->gainSlider->setEnabled(!agc_on);
    emit agcToggled(agc_on);

    // ensure manual gain is restored when AGC is disabled
    if (!agc_on)
        on_gainSlider_valueChanged(ui->gainSlider->value());
}

void SdrDeviceRtlsdrRxCtl::on_dsCombo_currentIndexChanged(int index)
{
    emit dsModeChanged(index);
}

void SdrDeviceRtlsdrRxCtl::setGain(int gain)
{
    int index = gains.indexOf(0.1f * float(gain));
    if (index >= 0)
        ui->gainSlider->setValue(index);
}
