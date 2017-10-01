/*
 * Main application window
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
#include <QtWidgets>

#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"
#include "gui/ssi_widget.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gui/tmp_plotter.h"

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
    ui->menuBar->hide();
    ui->toolBar->hide();

    sdr = new SdrThread();

    fft_timer = new QTimer(this);
    connect(fft_timer, SIGNAL(timeout()), this, SLOT(fftTimeout()));
    fft_data = new real_t[FFT_SIZE]; // FIXME
    fft_avg = new real_t[FFT_SIZE];

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
    fctl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    fctl->setup(9, 0, 900e6, 1, FCTL_UNIT_NONE);
    fctl->setFrequency(127100000);
    connect(fctl, SIGNAL(newFrequency(qint64)), this,
            SLOT(newFrequency(qint64)));

    // SSI
    smeter = new SsiWidget(this);
    smeter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Control panel
    cpanel = new ControlPanel(this);
    connect(cpanel, SIGNAL(runButtonClicked(bool)),
            this, SLOT(runButtonClicked(bool)));
    connect(cpanel, SIGNAL(demodChanged(sdr_demod_t)),
            this, SLOT(setDemod(sdr_demod_t)));
    connect(cpanel, SIGNAL(filterChanged(float,float)),
            this, SLOT(setFilter(float,float)));
    connect(cpanel, SIGNAL(cwOffsetChanged(float)),
            this, SLOT(setCwOffset(float)));

    // Temporary FFT plot
    fft_plot = new CPlotter(this);
    fft_plot->setSampleRate(3000000.0);
    fft_plot->setSpanFreq(3000000);
    fft_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    fft_plot->setFftRange(-100.0, 0.0);
    connect(fft_plot, SIGNAL(newCenterFreq(qint64)),
            this, SLOT(newPlotterCenterFreq(qint64)));

    // top layout with frequency controller, meter and buttons
    top_layout = new QHBoxLayout();
    top_layout->addWidget(ptt_button, 0);
    top_layout->addWidget(spacer1, 1);
    top_layout->addWidget(fctl, 2);
    top_layout->addWidget(spacer2, 1);
    top_layout->addWidget(smeter, 2);
    top_layout->addWidget(spacer3, 1);
    top_layout->addWidget(cfg_button, 0);

    // main layout with FFT and control panel
    main_layout = new QVBoxLayout();
    main_layout->addLayout(top_layout, 0);
    main_layout->addWidget(fft_plot, 1);

    // top level window layout
    win_layout = new QHBoxLayout();
    win_layout->setMargin(4);
    win_layout->addLayout(main_layout, 12);
    win_layout->addWidget(cpanel, 2);
    ui->centralWidget->setLayout(win_layout);

    // FIXME: Adjust default size as a function of desktop size
}

MainWindow::~MainWindow()
{
    // stop SDR
    runButtonClicked(false);

    delete sdr;
    delete fft_data;
    delete fft_avg;

    delete cfg_menu;
    delete cfg_button;
    delete ptt_button;

    delete fctl;
    delete smeter;
    delete fft_plot;
    delete cpanel;
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
    ptt_button->setMinimumSize(36, 36);
    ptt_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);

    cfg_menu = new QMenu();
    cfg_menu->setTitle(tr("Configure..."));
    cfg_menu->addAction(tr("SDR device"));
    cfg_menu->addAction(tr("Soundcard"));
    cfg_menu->addAction(tr("User interface"));

    cfg_button = new QToolButton(this);
    cfg_button->setText("C");
    cfg_button->setMinimumSize(36, 36);
    cfg_button->setMenu(cfg_menu);
    cfg_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);
    connect(cfg_button, SIGNAL(triggered(QAction *)),
            this, SLOT(menuActivated(QAction *)));
    connect(cfg_button, SIGNAL(clicked(bool)),
            this, SLOT(cfgButtonClicked(bool)));
}

void MainWindow::runButtonClicked(bool checked)
{
    if (checked)
    {
        if (sdr->start() == SDR_THREAD_OK)
        {
            newFrequency(fctl->getFrequency());
            fft_timer->start(50);

            {
                int i;
                for (i = 0; i < FFT_SIZE; i++)
                    fft_avg[i] = -100.0;
            }
        }
        // else FIXME
    }
    else
    {
        fft_timer->stop();
        sdr->stop();
    }
}

void MainWindow::cfgButtonClicked(bool checked)
{
    Q_UNUSED(checked);
    cpanel->setVisible(!cpanel->isVisible());
}

void MainWindow::menuActivated(QAction *action)
{
    MW_DEBUG("%s: %s\n", __func__, action->text().toLatin1().data());
}

void MainWindow::newFrequency(qint64 freq)
{
    sdr->setRxFrequency(freq);
    fft_plot->setCenterFreq(freq);
}

void MainWindow::newPlotterCenterFreq(qint64 freq)
{
    fctl->setFrequency(freq);
}

void MainWindow::setDemod(sdr_demod_t demod)
{
    sdr->setDemod(demod);
}

void MainWindow::setFilter(real_t low_cut, real_t high_cut)
{
    sdr->setRxFilter(low_cut, high_cut);
    fft_plot->setHiLowCutFrequencies(low_cut, high_cut);
}

void MainWindow::setCwOffset(real_t offset)
{
    sdr->setRxCwOffset(offset);
}

void MainWindow::fftTimeout(void)
{
    quint32    fft_samples;

    fft_samples = sdr->getFftData(fft_data);
    if (fft_samples == FFT_SIZE)
    {
        int i;
        for (i = 0; i < FFT_SIZE; i++)
            fft_avg[i] += 0.5f * (fft_data[i] - fft_avg[i]);

        fft_plot->setNewFttData(fft_avg, fft_data, fft_samples);
    }

    // FIXME
    smeter->setLevel(sdr->getSignalStrength());
}
