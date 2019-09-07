/*
 * Main application window interface
 */
#pragma once

#include <QSettings>
#include <QtWidgets>

#include "app/app_config.h"
#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"
#include "gui/ssi_widget.h"
#include "interfaces/sdr/sdr_device.h"

#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"

#include "gui/tmp_plotter.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void    runButtonClicked(bool);
    void    cfgButtonClicked(bool);
    void    hideButtonClicked(bool);
    void    menuActivated(QAction *);
    void    newFrequency(qint64 freq);
    void    newPlotterCenterFreq(qint64);
    void    newPlotterDemodFreq(qint64, qint64);
    void    setDemod(sdr_demod_t);
    void    setFilter(real_t, real_t);
    void    setFilterInt(int, int);
    void    setCwOffset(real_t);
    void    fftTimeout(void);

private:
    void    saveWindowState(void);
    void    restoreWindowState(void);
    void    createButtons(void);
    void    loadConfig(void);
    void    saveConfig(void);
    void    runDeviceConfig(void);
    void    deviceConfigChanged(const device_config_t * conf);

private:
    SdrDevice        *device;
    SdrThread        *sdr;

private:
    QSettings        *settings;
    AppConfig        *cfg;
    QTimer           *fft_timer;
    real_t           *fft_data;
    real_t           *fft_avg;

    // controls
    FreqCtrl         *fctl;
    SsiWidget        *smeter;
    ControlPanel     *cpanel;

    // Buttons
    QToolButton      *ptt_button;
    QToolButton      *run_button;
    QToolButton      *cfg_button;
    QMenu            *cfg_menu;
    QToolButton      *hide_button;

    // FFT plot
    CPlotter         *fft_plot;

    // layout containers
    QVBoxLayout      *win_layout;
    QHBoxLayout      *top_layout;
    QHBoxLayout      *main_layout;
};
