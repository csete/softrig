/*
 * Main application window
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017 Alexandru Csete OZ9AEC.
 * All rights reserved.
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
#include <QMenu>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#if 1
#include <stdio.h>
#define MW_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define MW_DEBUG(...)
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QWidget   *spacer1;
    QWidget   *spacer2;
    QWidget   *spacer3;


    ui->setupUi(this);

    sdr = new SdrThread();

    // 3 horizontal spacers
    spacer1 = new QWidget();
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    createButtons();

    // Frequency controller
    fctl = new FreqCtrl(this);
    fctl->setup(8, 0, 60e6, 1, FCTL_UNIT_NONE);
    fctl->setFrequency(14236000);
    connect(fctl, SIGNAL(newFrequency(qint64)), this, SLOT(newFrequency(qint64)));

    // top layout with frequency controller, meter and buttons
    top_layout = new QHBoxLayout();
    top_layout->addWidget(ptt_button);
    top_layout->addWidget(spacer1);
    top_layout->addWidget(fctl);
    top_layout->addWidget(spacer2);
    top_layout->addWidget(run_button);
    top_layout->addWidget(cfg_button);

    // main layout with FFT and control panel
    main_layout = new QHBoxLayout();

    // top level window layout
    win_layout = new QVBoxLayout();
    win_layout->addLayout(top_layout, 0);
    win_layout->addLayout(main_layout, 1);
    ui->centralWidget->setLayout(win_layout);

//    connect(ui->cpanel, SIGNAL(confButtonClicked()),
//            this, SLOT(runDeviceConfig()));
}

MainWindow::~MainWindow()
{
    delete sdr;

    delete cfg_menu;
    delete cfg_button;

    delete ptt_button;
    delete run_button;

    delete fctl;
    delete top_layout;
    delete main_layout;
    delete win_layout;
    delete ui;
}

void MainWindow::createButtons(void)
{
    ptt_button = new QToolButton(this);
    ptt_button->setText(tr("PTT"));
    ptt_button->setCheckable(true);
    ptt_button->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);

    run_button = new QToolButton(this);
    run_button->setText(tr("Run"));
    run_button->setCheckable(true);
    run_button->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
    connect(run_button, SIGNAL(clicked(bool)),
            this, SLOT(runButtonClicked(bool)));

    cfg_menu = new QMenu();
    cfg_menu->setTitle(tr("Configure..."));
    cfg_menu->addAction(tr("SDR device"));
    cfg_menu->addAction(tr("Soundcard"));
    cfg_menu->addAction(tr("User interface"));

    cfg_button = new QToolButton(this);
    cfg_button->setMenu(cfg_menu);
    cfg_button->setArrowType(Qt::NoArrow);
    cfg_button->setText(tr("CTL"));
    cfg_button->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
    connect(cfg_button, SIGNAL(triggered(QAction *)),
            this, SLOT(menuActivated(QAction *)));
}

void MainWindow::runButtonClicked(bool checked)
{
    if (checked)
    {
        if (sdr->start() == SDR_THREAD_OK)
            sdr->setRxFrequency(fctl->getFrequency());
    }
    else
    {
        sdr->stop();
    }
}

void MainWindow::menuActivated(QAction *action)
{
    MW_DEBUG("%s: %s\n", __func__, action->text().toLatin1().data());
}

void MainWindow::newFrequency(qint64 freq)
{
    sdr->setRxFrequency(freq);
}
