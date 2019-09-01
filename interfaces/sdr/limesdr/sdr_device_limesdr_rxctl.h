/*
 * LimeSDR control interface
 */
#pragma once

#include <QtCore>
#include <QWidget>

#include "sdr_device_limesdr_api_defs.h"

typedef struct {
    quint64         rx_frequency;
    quint32         rx_sample_rate;
    quint32         rx_bandwidth;
    unsigned int    rx_gain;
    size_t          rx_channel;
    bool            rx_lpf;
    bool            rx_gfir;
} limesdr_settings_t;

typedef struct {
    int             rx_channels;
    int             tx_channels;
    lms_range_t     rx_lo;
    lms_range_t     tx_lo;
    lms_range_t     rx_lpf;
    lms_range_t     tx_lpf;
    lms_range_t     rx_rate;
    lms_range_t     tx_rate;
} limesdr_info_t;


namespace Ui {
class SdrDeviceLimesdrRxctl;
}

class SdrDeviceLimesdrRxctl : public QWidget
{
    Q_OBJECT

public:
    explicit SdrDeviceLimesdrRxctl(QWidget *parent = nullptr);
    ~SdrDeviceLimesdrRxctl();

    void    readSettings(const limesdr_settings_t &settings);

signals:
    void    gainChanged(unsigned int gain);
    void    lpfToggled(bool lpf_on);
    void    gfirToggled(bool gfir_on);
    void    calibrate(void);

private slots:
    void    on_gainSlider_valueChanged(int value);
    void    on_lpfCheckBox_toggled(bool checked);
    void    on_gfirCheckBox_toggled(bool checked);
    void    on_calButton_clicked(bool checked);

private:
    Ui::SdrDeviceLimesdrRxctl *ui;
};
