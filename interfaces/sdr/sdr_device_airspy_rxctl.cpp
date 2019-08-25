/*
 * Airspy user interface
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

#include <QDebug>

#include "sdr_device_airspy_rxctl.h"
#include "ui_sdr_device_airspy_rxctl.h"

SdrDeviceAirspyRxctl::SdrDeviceAirspyRxctl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SdrDeviceAirspyRxctl),
    agc_is_on(false)
{
    ui->setupUi(this);
    setProperty("desc", QString("Airspy"));

    on_gainModeCombo_currentTextChanged("Linearity");
}

SdrDeviceAirspyRxctl::~SdrDeviceAirspyRxctl()
{
    delete ui;
}

void SdrDeviceAirspyRxctl::readSettings(const airspy_settings_t &settings)
{
    ui->linGainSlider->setValue(settings.linearity_gain);
    ui->sensGainSlider->setValue(settings.sensitivity_gain);
    ui->lnaGainSlider->setValue(settings.lna_gain);
    ui->mixGainSlider->setValue(settings.mixer_gain);
    ui->vgaGainSlider->setValue(settings.vga_gain);
    ui->biasCheckBox->setChecked(settings.bias_on);
    ui->gainModeCombo->setCurrentText(settings.gain_mode);
    on_gainModeCombo_currentTextChanged(settings.gain_mode);
}

void SdrDeviceAirspyRxctl::on_gainModeCombo_currentTextChanged(const QString &text)
{
    emit gainModeChanged(text);
    if (text.contains("linearity", Qt::CaseInsensitive))
    {
        ui->linGainSlider->setVisible(true);
        ui->linGainLabel->setVisible(true);
        ui->linLabel->setVisible(true);
        ui->sensGainSlider->setVisible(false);
        ui->sensGainLabel->setVisible(false);
        ui->sensLabel->setVisible(false);
        ui->lnaGainSlider->setVisible(false);
        ui->lnaGainLabel->setVisible(false);
        ui->lnaLabel->setVisible(false);
        ui->mixGainSlider->setVisible(false);
        ui->mixGainLabel->setVisible(false);
        ui->mixLabel->setVisible(false);
        ui->vgaGainSlider->setVisible(false);
        ui->vgaGainLabel->setVisible(false);
        ui->vgaLabel->setVisible(false);
        if (agc_is_on)
        {
            agc_is_on = false;
            emit agcToggled(false);
        }
        emit linearityGainChanged(ui->linGainSlider->value());
    }
    else if (text.contains("sensitivity", Qt::CaseInsensitive))
    {
        ui->linGainSlider->setVisible(false);
        ui->linGainLabel->setVisible(false);
        ui->linLabel->setVisible(false);
        ui->sensGainSlider->setVisible(true);
        ui->sensGainLabel->setVisible(true);
        ui->sensLabel->setVisible(true);
        ui->lnaGainSlider->setVisible(false);
        ui->lnaGainLabel->setVisible(false);
        ui->lnaLabel->setVisible(false);
        ui->mixGainSlider->setVisible(false);
        ui->mixGainLabel->setVisible(false);
        ui->mixLabel->setVisible(false);
        ui->vgaGainSlider->setVisible(false);
        ui->vgaGainLabel->setVisible(false);
        ui->vgaLabel->setVisible(false);
        if (agc_is_on)
        {
            agc_is_on = false;
            emit agcToggled(false);
        }
        emit sensitivityGainChanged(ui->sensGainSlider->value());
    }
    else if (text.contains("manual", Qt::CaseInsensitive))
    {
        ui->linGainSlider->setVisible(false);
        ui->linGainLabel->setVisible(false);
        ui->linLabel->setVisible(false);
        ui->sensGainSlider->setVisible(false);
        ui->sensGainLabel->setVisible(false);
        ui->sensLabel->setVisible(false);
        ui->lnaGainSlider->setVisible(true);
        ui->lnaGainLabel->setVisible(true);
        ui->lnaLabel->setVisible(true);
        ui->mixGainSlider->setVisible(true);
        ui->mixGainLabel->setVisible(true);
        ui->mixLabel->setVisible(true);
        ui->vgaGainSlider->setVisible(true);
        ui->vgaGainLabel->setVisible(true);
        ui->vgaLabel->setVisible(true);
        if (agc_is_on)
        {
            agc_is_on = false;
            emit agcToggled(false);
        }
        emit lnaGainChanged(ui->lnaGainSlider->value());
        emit mixerGainChanged(ui->mixGainSlider->value());
        emit vgaGainChanged(ui->vgaGainSlider->value());
    }
    else if (text.contains("auto", Qt::CaseInsensitive))
    {
        ui->linGainSlider->setVisible(false);
        ui->linGainLabel->setVisible(false);
        ui->linLabel->setVisible(false);
        ui->sensGainSlider->setVisible(false);
        ui->sensGainLabel->setVisible(false);
        ui->sensLabel->setVisible(false);
        ui->lnaGainSlider->setVisible(false);
        ui->lnaGainLabel->setVisible(false);
        ui->lnaLabel->setVisible(false);
        ui->mixGainSlider->setVisible(false);
        ui->mixGainLabel->setVisible(false);
        ui->mixLabel->setVisible(false);
        ui->vgaGainSlider->setVisible(false);
        ui->vgaGainLabel->setVisible(false);
        ui->vgaLabel->setVisible(false);
        agc_is_on = true;
        emit agcToggled(true);
    }
}

void SdrDeviceAirspyRxctl::on_linGainSlider_valueChanged(int value)
{
    ui->linGainLabel->setText(QString("%1").arg(value));
    emit linearityGainChanged(quint8(value));
}

void SdrDeviceAirspyRxctl::on_sensGainSlider_valueChanged(int value)
{
    ui->sensGainLabel->setText(QString("%1").arg(value));
    emit sensitivityGainChanged(quint8(value));
}

void SdrDeviceAirspyRxctl::on_lnaGainSlider_valueChanged(int value)
{
    ui->lnaGainLabel->setText(QString("%1").arg(value));
    emit lnaGainChanged(quint8(value));
}

void SdrDeviceAirspyRxctl::on_mixGainSlider_valueChanged(int value)
{
    ui->mixGainLabel->setText(QString("%1").arg(value));
    emit mixerGainChanged(quint8(value));
}

void SdrDeviceAirspyRxctl::on_vgaGainSlider_valueChanged(int value)
{
    ui->vgaGainLabel->setText(QString("%1").arg(value));
    emit vgaGainChanged(quint8(value));
}

void SdrDeviceAirspyRxctl::on_biasCheckBox_stateChanged(int state)
{
    emit biasToggled(state == Qt::Checked ? true : false);
}
