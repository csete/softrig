/*
 * Main application window interface
 */
#pragma once

#include <QHBoxLayout>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>

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
    void    runDeviceConfig(void);

private:
    void    createButtons(void);

private:
    Ui::MainWindow   *ui;

    // controls
    FreqCtrl         *fctl;

    // Buttons
    QPushButton      *ptt_button;
    QPushButton      *run_button;
    QPushButton      *cfg_button;
    QPushButton      *ctl_button;

    // layout containers
    QVBoxLayout      *win_layout;
    QHBoxLayout      *top_layout;
    QHBoxLayout      *main_layout;
};
