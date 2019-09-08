/*
 * SDR device configuration dialog
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
#include <QDebug>

#include "device_config_dialog.h"
#include "ui_device_config_dialog.h"

// Flags used to match SDR type in combo box
#define SDR_TYPE_MATCH_FLAGS    (Qt::MatchFixedString | Qt::MatchCaseSensitive)

DeviceConfigDialog::DeviceConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceConfigDialog)
{
    ui->setupUi(this);

    connect(ui->sdrTypeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(sdrTypeChanged(int)));
    connect(ui->inputRateCombo, SIGNAL(editTextChanged(QString)),
            this, SLOT(inputRateChanged(QString)));
    connect(ui->decimCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(decimationChanged(int)));

    // The item data is the string that can be passed to the backend
    ui->sdrTypeCombo->addItem(tr("Airspy Mini"), "airspymini");
    ui->sdrTypeCombo->addItem(tr("Airspy R2"), "airspy");
    ui->sdrTypeCombo->addItem("Nuand BladeRF 2.0", "bladerf");
    ui->sdrTypeCombo->addItem(tr("LimeSDR Mini"), "limesdr");
//    ui->sdrTypeCombo->addItem(tr("RFSpace SDR-IQ"), "sdriq");
    ui->sdrTypeCombo->addItem(tr("RTL-SDR"), "rtlsdr");
    ui->sdrTypeCombo->addItem(tr("SDRplay RSPduo"), "sdrplay");
}

DeviceConfigDialog::~DeviceConfigDialog()
{
    delete ui;
}

void DeviceConfigDialog::readSettings(const device_config_t * input)
{
    selectSdrType(input->type);
    selectSampleRate(input->rate);
    selectDecimation(input->decimation);
    setBandwidth(input->bandwidth);
}

void DeviceConfigDialog::saveSettings(device_config_t * input)
{
    input->type = ui->sdrTypeCombo->currentData(Qt::UserRole).toString();

    // QString.toInt() returns 0 if conversion fails
    input->rate = ui->inputRateCombo->currentText().toUInt();
    input->decimation = ui->decimCombo->currentText().toUInt();
    input->bandwidth = quint32(ui->bwSpinBox->value() * 1000.0);
}

void DeviceConfigDialog::sdrTypeChanged(int index)
{
    Q_UNUSED(index);

    QString     sdr_type(ui->sdrTypeCombo->currentData(Qt::UserRole).toString());

    ui->inputRateCombo->clear();
    if (sdr_type == "airspy")
    {
        ui->inputRateCombo->addItem("2500000");
        ui->inputRateCombo->addItem("10000000");
        ui->inputRateCombo->setCurrentIndex(1);
    }
    else if (sdr_type == "airspymini")
    {
        ui->inputRateCombo->addItem("3000000");
        ui->inputRateCombo->addItem("6000000");
        ui->inputRateCombo->addItem("10000000");
        ui->inputRateCombo->setCurrentIndex(1);
    }
    else if (sdr_type == "bladerf" || sdr_type == "limesdr")
    {
        ui->inputRateCombo->addItem("240000");
        ui->inputRateCombo->addItem("480000");
        ui->inputRateCombo->addItem("960000");
        ui->inputRateCombo->addItem("1920000");
        ui->inputRateCombo->addItem("3840000");
        ui->inputRateCombo->addItem("7680000");
        ui->inputRateCombo->addItem("15360000");
        ui->inputRateCombo->addItem("30720000");
        ui->inputRateCombo->addItem("61440000");
        ui->inputRateCombo->setCurrentIndex(2);
    }
    else if (sdr_type == "rtlsdr")
    {
        ui->inputRateCombo->addItem("240000");
        ui->inputRateCombo->addItem("300000");
        ui->inputRateCombo->addItem("960000");
        ui->inputRateCombo->addItem("1152000");
        ui->inputRateCombo->addItem("1200000");
        ui->inputRateCombo->addItem("1536000");
        ui->inputRateCombo->addItem("1600000");
        ui->inputRateCombo->addItem("1800000");
        ui->inputRateCombo->addItem("2400000");
        ui->inputRateCombo->addItem("3200000");
        ui->inputRateCombo->setCurrentIndex(8);
    }
    else if (sdr_type == "sdriq")
    {
        ui->inputRateCombo->addItem("55556");
        ui->inputRateCombo->addItem("111111");
        ui->inputRateCombo->addItem("158730");
        ui->inputRateCombo->addItem("196078");
        ui->inputRateCombo->setCurrentIndex(3);
    }
    else if (sdr_type == "sdrplay")
    {
        ui->inputRateCombo->addItem("2000000");
        ui->inputRateCombo->addItem("4000000");
        ui->inputRateCombo->addItem("6000000");
        ui->inputRateCombo->addItem("10000000");
        ui->inputRateCombo->setCurrentIndex(0);
    }
}

void DeviceConfigDialog::inputRateChanged(const QString &rate_str)
{
    bool    conv_ok;
    int     rate;

    rate = rate_str.toInt(&conv_ok);
    if (!conv_ok || rate < 0)
        return;

    ui->decimCombo->clear();
    ui->decimCombo->addItem("None");
    // add decimations that give an integer sample rate >= 48k
    if (rate >= 96000 && rate % 2 == 0)
        ui->decimCombo->addItem("2");
    if (rate >= 192000 && rate % 4 == 0)
        ui->decimCombo->addItem("4");
    if (rate >= 384000 && rate % 8 == 0)
        ui->decimCombo->addItem("8");
    if (rate >= 768000 && rate % 16 == 0)
        ui->decimCombo->addItem("16");
    if (rate >= 1536000 && rate % 32 == 0)
        ui->decimCombo->addItem("32");
    if (rate >= 3072000 && rate % 64 == 0)
        ui->decimCombo->addItem("64");
    if (rate >= 6144000 && rate % 128 == 0)
        ui->decimCombo->addItem("128");
//    if (rate >= 12288000 && rate % 256 == 0)
//        ui->decimCombo->addItem("256");
//    if (rate >= 24576000 && rate % 512 == 0)
//        ui->decimCombo->addItem("512");

    decimationChanged(0);
}

/* New decimation rate selected.
 * Calculate the quadrature rate and update the sample rate
 * label
 */
void DeviceConfigDialog::decimationChanged(int index)
{
    Q_UNUSED(index);

    float       quad_rate;
    int         input_rate;
    int         decim;
    bool        conv_ok;

    decim = ui->decimCombo->currentText().toInt(&conv_ok);
    if (!conv_ok)
        decim = 1;

    input_rate = ui->inputRateCombo->currentText().toInt(&conv_ok);
    if (!conv_ok)
        return;

    quad_rate = float(input_rate) / float(decim);
    if (quad_rate > 1.e6f)
        ui->sampRateString->setText(QString(" %1 Msps").
                                    arg(double(quad_rate * 1.e-6f), 0, 'f', 3));
    else
        ui->sampRateString->setText(QString(" %1 ksps").
                                    arg(double(quad_rate * 1.e-3f), 0, 'f', 3));
}

// Select SDR type from type string
void DeviceConfigDialog::selectSdrType(const QString &type)
{
    int     index;

    if (type.isEmpty())
        return;

    index = ui->sdrTypeCombo->findData(type, Qt::UserRole, SDR_TYPE_MATCH_FLAGS);
    if (index >= 0)
        ui->sdrTypeCombo->setCurrentIndex(index);
}

void DeviceConfigDialog::selectSampleRate(unsigned int rate)
{
    if (rate == 0)
        return;

    ui->inputRateCombo->setCurrentText(QString("%1").arg(rate));
}

static int decim2index(unsigned int decim)
{
    int         idx;

    if (decim == 0)
        return 0;

    idx = 0;
    while (decim >>= 1)
        ++idx;

    return idx;
}

void DeviceConfigDialog::selectDecimation(unsigned int decimation)
{
    ui->decimCombo->setCurrentIndex(decim2index(decimation));
}

void DeviceConfigDialog::setBandwidth(quint32 bw)
{
    ui->bwSpinBox->setValue(double(bw) * 1.e-3);
}
