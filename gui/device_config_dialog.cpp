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
    ui->sdrTypeCombo->addItem(tr("RTL-SDR"), "rtlsdr");
    ui->sdrTypeCombo->addItem(tr("Airspy R2"), "airspy");
    ui->sdrTypeCombo->addItem(tr("Airspy Mini"), "airspy");
    ui->sdrTypeCombo->addItem(tr("RFSpace SDR-IQ"), "sdriq");
}

DeviceConfigDialog::~DeviceConfigDialog()
{
    delete ui;
}

void DeviceConfigDialog::sdrTypeChanged(int index)
{
    Q_UNUSED(index);

    QString     sdr_type(ui->sdrTypeCombo->currentData(Qt::UserRole).toString());

    ui->inputRateCombo->clear();
    if (sdr_type == "rtlsdr")
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
    }
    else if (sdr_type == "airspy")
    {
        if (ui->sdrTypeCombo->currentText().contains("mini", Qt::CaseInsensitive))
        {
            ui->inputRateCombo->addItem("3000000");
            ui->inputRateCombo->addItem("6000000");
        }
        else
        {
            ui->inputRateCombo->addItem("2500000");
            ui->inputRateCombo->addItem("10000000");
        }
    }
    else if (sdr_type == "sdriq")
    {
        ui->inputRateCombo->addItem("55556");
        ui->inputRateCombo->addItem("111111");
        ui->inputRateCombo->addItem("158730");
        ui->inputRateCombo->addItem("196078");
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

    quad_rate = (float)input_rate / (float)decim;
    if (quad_rate > 1.e6f)
        ui->sampRateString->setText(QString(" %1 Msps").
                                    arg(quad_rate * 1.e-6f, 0, 'f', 3));
    else
        ui->sampRateString->setText(QString(" %1 ksps").
                                    arg(quad_rate * 1.e-3f, 0, 'f', 3));
}
