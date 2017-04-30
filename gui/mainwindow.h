/*
 * Main application window interface
 */
#pragma once

#include <QMainWindow>

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
    Ui::MainWindow   *ui;

    /*
     * UI members:
     *   ControlPanel   ui->cpanel
     */
};
