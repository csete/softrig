/*
 * Main application window
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017 Alexandru Csete OZ9AEC.
 * All rights reserved.
 *
 * This software is released under the "Simplified BSD License".
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
#include "device_config.h"
#include "ui_device_config.h"

DeviceConfig::DeviceConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceConfig)
{
    QAudioDeviceInfo    deviceInfo;
    int                 i;

    ui->setupUi(this);
    inputDevices = deviceInfo.availableDevices(QAudio::AudioInput);
    for (i = 0; i < inputDevices.size(); i++)
    {
        ui->sdrDevCombo->addItem(inputDevices.at(i).deviceName());
        ui->ainDevCombo->addItem(inputDevices.at(i).deviceName());
    }
    outputDevices = deviceInfo.availableDevices(QAudio::AudioOutput);
    for (i = 0; i < outputDevices.size(); i++)
        ui->aoutDevCombo->addItem(outputDevices.at(i).deviceName());

    /* form layout has the audio tab selected by default, otherwise
     * device selectors would force the dialog window to be very wide (bug?)
     */
    ui->tabWidget->setCurrentIndex(0);
}

DeviceConfig::~DeviceConfig()
{
    delete ui;
}
