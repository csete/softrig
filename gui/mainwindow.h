/*
 * Main application window interface
 */
#pragma once

#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/sdr_thread.h"
#include "gui/control_panel.h"
#include "gui/freq_ctrl.h"

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

private:
    void    createButtons(void);

private:
    Ui::MainWindow   *ui;

    SdrThread        *sdr;

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
