/*
 * RTL-SDR user interface
 */
#pragma once

#include <QVector>
#include <QWidget>

namespace Ui {
class SdrDeviceRtlsdrRxCtl;
}

// direct sampling modes
enum _ds_mode {
    RXCTL_DS_MODE_AUTO_Q = 0,
    RXCTL_DS_MODE_AUTO_I = 1,
    RXCTL_DS_MODE_Q = 2,
    RXCTL_DS_MODE_I = 3,
    RXCTL_DS_MODE_OFF = 4,
    RXCTL_DS_MODE_NUM = 5
};

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
    void    dsModeChanged(int mode);

private slots:
    void    on_gainSlider_valueChanged(int index);
    void    on_biasButton_toggled(bool bias_on);
    void    on_agcButton_toggled(bool agc_on);
    void    on_dsCombo_currentIndexChanged(int index);

private:
    Ui::SdrDeviceRtlsdrRxCtl *ui;

    QVector<float>    gains;
};
