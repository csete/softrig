/*
 * Main application window interface
 */
#pragma once

#include <QtWidgets>

#include "app/app_config.h"
#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"
#include "gui/ssi_widget.h"
#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"

#include "gui/tmp_plotter.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void    runButtonClicked(bool);
    void    cfgButtonClicked(bool);
    void    menuActivated(QAction *);
    void    newFrequency(qint64 freq);
    void    newPlotterCenterFreq(qint64);
    void    setDemod(sdr_demod_t);
    void    setFilter(real_t, real_t);
    void    setCwOffset(real_t);
    void    fftTimeout(void);

private:
    void    createButtons(void);
    void    loadConfig(void);
    void    runDeviceConfig(void);
    void    deviceConfigChanged(const device_config_t * conf);

private:
    AppConfig        *cfg;
    SdrThread        *sdr;
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

    // FFT plot
    CPlotter         *fft_plot;

    // layout containers
    QHBoxLayout      *win_layout;
    QHBoxLayout      *top_layout;
    QVBoxLayout      *main_layout;
};
