/*
 * Main application window
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017-2019 Alexandru Csete OZ9AEC.
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
#include <QMessageBox>

#include "app/app_config.h"
#include "gui/control_panel.h"
#include "gui/device_config_dialog.h"
#include "gui/freq_ctrl.h"
#include "gui/ssi_widget.h"
#include "interfaces/sdr/sdr_device.h"

#include "mainwindow.h"

#include "gui/tmp_plotter.h"

// IDs used to identify menu items
#define MENU_ID_SDR         0
#define MENU_ID_AUDIO       1
#define MENU_ID_GUI         2

#define FFT_SIZE 2*8192

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QFrame    *central_widget;
    QWidget   *spacer1;
    QWidget   *spacer2;
    QWidget   *spacer3;


    cfg = nullptr;
    device = nullptr;

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
    fctl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fctl->setup(10, 0, 2e9, 1, FCTL_UNIT_NONE);
    connect(fctl, SIGNAL(newFrequency(qint64)), this,
            SLOT(newFrequency(qint64)));

    // SSI
    smeter = new SsiWidget(this);
    smeter->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Control panel
    cpanel = new ControlPanel(this);
    connect(cpanel, SIGNAL(demodChanged(sdr_demod_t)),
            this, SLOT(setDemod(sdr_demod_t)));
    connect(cpanel, SIGNAL(filterChanged(float,float)),
            this, SLOT(setFilter(float,float)));
    connect(cpanel, SIGNAL(cwOffsetChanged(float)),
            this, SLOT(setCwOffset(float)));

    // Temporary FFT plot
    fft_plot = new CPlotter(this);
    fft_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    fft_plot->setFftRange(-100.0, 0.0);
    connect(fft_plot, SIGNAL(newCenterFreq(qint64)),
            this, SLOT(newPlotterCenterFreq(qint64)));
    connect(fft_plot, SIGNAL(newDemodFreq(qint64, qint64)),
            this, SLOT(newPlotterDemodFreq(qint64, qint64)));
    connect(fft_plot, SIGNAL(pandapterRangeChanged(float,float)),
            fft_plot, SLOT(setWaterfallRange(float,float)));

    // top layout with frequency controller, meter and buttons
    top_layout = new QHBoxLayout();
    top_layout->addWidget(ptt_button, 0);
    top_layout->addWidget(spacer1, 1);
    top_layout->addWidget(fctl, 2);
    top_layout->addWidget(spacer2, 1);
    top_layout->addWidget(smeter, 2);
    top_layout->addWidget(spacer3, 1);
    top_layout->addWidget(run_button, 0);
    top_layout->addWidget(cfg_button, 0);
    top_layout->addWidget(hide_button, 0);

    // main layout with FFT and control panel
    main_layout = new QHBoxLayout();
    main_layout->addWidget(fft_plot, 15);
    main_layout->addWidget(cpanel, 2);

    // top level window layout
    win_layout = new QVBoxLayout();
    win_layout->setContentsMargins(0, 4, 0, 0);
    win_layout->setSpacing(5);
    win_layout->addLayout(top_layout, 0);
    win_layout->addLayout(main_layout, 1);

    central_widget = new QFrame(this);
    central_widget->setLayout(win_layout);
    setCentralWidget(central_widget);

    loadConfig();
    setWindowTitle(QString("Softrig %1").arg(VERSION));
    resize(900, 600);
}

MainWindow::~MainWindow()
{
    // stop SDR
    runButtonClicked(false);

    saveConfig();
    cfg->save();
    cfg->close();

    delete cfg;
    delete sdr;
    delete fft_data;
    delete fft_avg;

    delete cfg_menu;
    delete cfg_button;
    delete hide_button;
    delete run_button;
    delete ptt_button;

    delete fctl;
    delete smeter;
    delete fft_plot;
    delete cpanel;
    delete top_layout;
    delete main_layout;
    delete win_layout;
}

void MainWindow::loadConfig(void)
{
    app_config_t *conf;

    if (cfg)
        cfg->close();
    else
        cfg = new AppConfig();

    if (cfg->load("./softrig.conf") == APP_CONFIG_OK)
    {
        conf = cfg->getDataPtr();
        if (conf->input.type.isEmpty())
            runDeviceConfig();
        else
            deviceConfigChanged(&conf->input);

        cpanel->readSettings(*conf);
    }
    else
    {
        QMessageBox::critical(this, tr("Configuration error"),
                              tr("Error loading configuration file"));
    }
}

void MainWindow::saveConfig(void)
{
    app_config_t *conf = cfg->getDataPtr();

    conf->input.frequency = fft_plot->getCenterFreq();
    conf->input.nco = fft_plot->getFilterOffset();
    cpanel->saveSettings(*conf);
}

// SDR device configuration changed; update GUI
void MainWindow::deviceConfigChanged(const device_config_t * conf)
{
    float   quad_rate;
    bool    running = sdr->isRunning();

    if (running)
        runButtonClicked(false);

    if (!conf->type.isEmpty())
    {
        if (device)
            delete device;
        device = sdr_device_create(conf->type);
        if (!device)
            QMessageBox::critical(this, tr("Configuration error"),
                                  tr("Error creating SDR device"));
        else
            cpanel->addRxControls(device->getRxControls());
    }

    quad_rate = conf->rate;
    if (conf->decimation > 1)
        quad_rate /= conf->decimation;

    fft_plot->setSampleRate(quad_rate);
    fft_plot->setSpanFreq(quad_rate);
    fft_plot->setCenterFreq(conf->frequency);
    fft_plot->setFilterOffset(conf->nco);
    fctl->setFrequency(conf->frequency + conf->nco);

    if (running && device)
        runButtonClicked(true);
}

void MainWindow::createButtons(void)
{
    ptt_button = new QToolButton(this);
    ptt_button->setText(tr("PTT"));
    ptt_button->setCheckable(true);
    ptt_button->setMinimumSize(36, 36);
    ptt_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);

    {
        QAction     *action;

        cfg_menu = new QMenu();
        cfg_menu->setTitle(tr("Configure..."));
        action = cfg_menu->addAction(tr("SDR device"));
        action->setData(QVariant((int)MENU_ID_SDR));
        action = cfg_menu->addAction(tr("Soundcard"));
        action->setData(QVariant((int)MENU_ID_AUDIO));
        action = cfg_menu->addAction(tr("User interface"));
        action->setData(QVariant((int)MENU_ID_GUI));
    }

    run_button = new QToolButton(this);
    run_button->setText("Run");
    run_button->setCheckable(true);
    run_button->setMinimumSize(36, 36);
    run_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);
    connect(run_button, SIGNAL(clicked(bool)),
            this, SLOT(runButtonClicked(bool)));

    cfg_button = new QToolButton(this);
    cfg_button->setText("Conf");
    cfg_button->setMinimumSize(36, 36);
    cfg_button->setMenu(cfg_menu);
    cfg_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);
    connect(cfg_button, SIGNAL(triggered(QAction *)),
            this, SLOT(menuActivated(QAction *)));
    connect(cfg_button, SIGNAL(clicked(bool)),
            this, SLOT(cfgButtonClicked(bool)));

    hide_button = new QToolButton(this);
    hide_button->setText("Panel");
    hide_button->setMinimumSize(36, 36);
    hide_button->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::MinimumExpanding);
    connect(hide_button, SIGNAL(clicked(bool)),
            this, SLOT(hideButtonClicked(bool)));
}

void MainWindow::runButtonClicked(bool checked)
{
    if (checked)
    {
        app_config_t *conf = cfg->getDataPtr();

        if (!device)
            runDeviceConfig();

        if (!device)
        {
            // still no SDR configuration
            (void)QMessageBox::critical(this, tr("SDR device error"),
                                        tr("SDR device not configured"));
            run_button->setChecked(false);
            return;
        }

        if (device->open() != SDR_DEVICE_OK)
        {
            // FIXME: Error code
            (void)QMessageBox::critical(this, tr("SDR device error"),
                                        tr("Failed to open SDR device"));
            run_button->setChecked(false);
            return;
        }

        // FIXME: should be in deviceConfigChanged()
        device->setRxSampleRate(conf->input.rate);
        device->setRxBandwidth(conf->input.bandwidth);

        if (sdr->start(conf, device) == SDR_THREAD_OK)
        {
            device->startRx();
            newFrequency(fctl->getFrequency());
            fft_timer->start(40);
            {
                int i;
                for (i = 0; i < FFT_SIZE; i++)
                    fft_avg[i] = -100.0;
            }
        }
        // FIXME: Error message
    }
    else
    {
        device->stopRx();
        device->close();
        fft_timer->stop();
        sdr->stop();
    }
}

void MainWindow::hideButtonClicked(bool checked)
{
    Q_UNUSED(checked);
    cpanel->setVisible(!cpanel->isVisible());
}

void MainWindow::cfgButtonClicked(bool checked)
{
    Q_UNUSED(checked);
    runDeviceConfig();
}

void MainWindow::runDeviceConfig(void)
{
    DeviceConfigDialog    conf_dialog(this);
    device_config_t      *input_settings = &cfg->getDataPtr()->input;

    // FIXME: setInteractive()
    conf_dialog.readSettings(input_settings);
    if (conf_dialog.exec() == QDialog::Accepted)
    {
        // save changes
        conf_dialog.saveSettings(input_settings);
        deviceConfigChanged(input_settings);
    }
}

void MainWindow::menuActivated(QAction *action)
{
    bool    conv_ok;
    int     menu_id;

    menu_id = action->data().toInt(&conv_ok);
    if (!conv_ok)
    {
        qCritical("%s: Menu item '%s' has no valid ID", __func__,
                  action->text().toLatin1().data());
        return;
    }

    switch (menu_id)
    {
    case MENU_ID_SDR:
        runDeviceConfig();
        break;
    case MENU_ID_AUDIO:
        break;
    case MENU_ID_GUI:
        break;
    default:
        qCritical("%s: Unknown menu item '%s' ID=%d", __func__,
                  action->text().toLatin1().data(), menu_id);
        break;
    }
}

void MainWindow::newFrequency(qint64 freq)
{
    qint64      center_freq;

    center_freq = freq - fft_plot->getFilterOffset();
    fft_plot->setCenterFreq(quint64(center_freq));
    if (device)
        device->setRxFrequency(quint64(center_freq));
}

void MainWindow::newPlotterCenterFreq(qint64 freq)
{
    fctl->setFrequency(freq);
}

void MainWindow::newPlotterDemodFreq(qint64 freq, qint64 delta)
{
    fctl->setFrequency(freq);
    sdr->setRxTuningOffset(delta);
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
            fft_avg[i] += 0.25f * (fft_data[i] - fft_avg[i]);

        fft_plot->setNewFttData(fft_avg, fft_data, fft_samples);
    }

    // FIXME
    float signal = sdr->getSignalStrength();
    smeter->setLevel(signal);
    cpanel->addSignalData(signal);
}
