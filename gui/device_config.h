/*
 * SDR device configuration dialog
 */
#pragma once

#include <QDialog>

namespace Ui {
class DeviceConfig;
}

class DeviceConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceConfig(QWidget *parent = 0);
    ~DeviceConfig();

private slots:
    void    sdrTypeChanged(int);
    void    inputRateChanged(const QString &);
    void    decimationChanged(int);

private:
    Ui::DeviceConfig *ui;
};
