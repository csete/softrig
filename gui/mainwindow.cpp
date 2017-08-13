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
#include <QDebug>
#include <QSpacerItem>
#include <QWidget>

#include "gui/control_panel.h"
#include "gui/device_config.h"
#include "gui/freq_ctrl.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QWidget     *spacer1;
    QWidget     *spacer2;

    ui->setupUi(this);

    spacer1 = new QWidget();
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    fctl = new FreqCtrl(this);
    fctl->setup(8, 0, 60e6, 1, FCTL_UNIT_NONE);
    fctl->setFrequency(14236000);

    ui->toolBar->addWidget(spacer1);
    ui->toolBar->addWidget(fctl);
    ui->toolBar->addWidget(spacer2);

//    connect(ui->cpanel, SIGNAL(confButtonClicked()),
//            this, SLOT(runDeviceConfig()));
}

MainWindow::~MainWindow()
{
    delete fctl;
    delete ui;
}

void MainWindow::runDeviceConfig()
{
    DeviceConfig   *dc;
    int             code;

    dc = new DeviceConfig(this);
    // dc->readSettings();
    code = dc->exec();
    if (code == QDialog::Accepted)
    {
        // dc->saveSettings
    }
    delete dc;
}
