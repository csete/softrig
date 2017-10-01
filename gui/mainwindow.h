/*
 * Main application window interface
 */
#pragma once

#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"
#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"

namespace Ui {
    class MainWindow;
}

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
    void    setDemod(sdr_demod_t);
    void    setFilter(real_t, real_t);
    void    setCwOffset(real_t);
    void    fftTimeout(void);

private:
    void    createButtons(void);

private:
    Ui::MainWindow   *ui;

    SdrThread        *sdr;
    QTimer           *fft_timer;
    real_t           *fft_data;

    // controls
    FreqCtrl         *fctl;
    ControlPanel     *cpanel;

    // Buttons
    QToolButton      *ptt_button;
    QToolButton      *run_button;
    QToolButton      *cfg_button;
    QMenu            *cfg_menu;

    // layout containers
    QVBoxLayout      *win_layout;
    QHBoxLayout      *top_layout;
    QHBoxLayout      *main_layout;
};
