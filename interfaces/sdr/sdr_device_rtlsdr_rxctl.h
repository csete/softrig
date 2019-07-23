/*
 * RTL-SDR user interface
 */
#pragma once

#include <QVector>
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

    void    setTunerGains(int *values, int count);

signals:
    void    gainChanged(int gain);
    void    biasToggled(bool bias_on);
    void    agcToggled(bool agc_on);

private slots:
    void    on_gainSlider_valueChanged(int index);
    void    on_biasButton_toggled(bool bias_on);
    void    on_agcButton_toggled(bool agc_on);

private:
    Ui::SdrDeviceRtlsdrRxCtl *ui;

    QVector<float>    gains;
};
