#pragma once

#include <QWidget>

namespace Ui {
class SdrDeviceRtlsdrRxCtl;
}

class SdrDeviceRtlsdrRxCtl : public QWidget
{
    Q_OBJECT

public:
    explicit SdrDeviceRtlsdrRxCtl(QWidget *parent = nullptr);
    ~SdrDeviceRtlsdrRxCtl();

private:
    Ui::SdrDeviceRtlsdrRxCtl *ui;
};
