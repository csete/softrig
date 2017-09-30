/*
 * Main control panel
 */
#pragma once

#include <QWidget>

namespace Ui {
    class ControlPanel;
}

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = 0);
    ~ControlPanel();

signals:
    void    demodChanged(quint8 new_demod);

private slots:
    void    on_amButton_clicked(bool);
    void    on_ssbButton_clicked(bool);
    void    on_cwButton_clicked(bool);
    void    on_fmButton_clicked(bool);

private:
    Ui::ControlPanel   *ui;
};
