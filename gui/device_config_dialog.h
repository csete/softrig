/*
 * SDR device configuration dialog
 */
#pragma once

#include <QDialog>

namespace Ui {
class DeviceConfigDialog;
}

class DeviceConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceConfigDialog(QWidget *parent = 0);
    ~DeviceConfigDialog();

private slots:
    void    sdrTypeChanged(int);
    void    inputRateChanged(const QString &);
    void    decimationChanged(int);

private:
    Ui::DeviceConfigDialog *ui;
};
