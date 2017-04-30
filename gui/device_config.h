/*
 * Input/output device configuration dialog
 */
#pragma once

#include <QAudioDeviceInfo>
#include <QDialog>
#include <QList>

namespace Ui {
    class DeviceConfig;
}

class DeviceConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceConfig(QWidget *parent = 0);
    ~DeviceConfig();

private:
    Ui::DeviceConfig          *ui;

    QList<QAudioDeviceInfo>    inputDevices;
    QList<QAudioDeviceInfo>    outputDevices;
};
