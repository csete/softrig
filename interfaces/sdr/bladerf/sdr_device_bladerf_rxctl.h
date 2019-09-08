/*
 * BladeRF receiver control interface
 */
#pragma once

#include <QWidget>

#include "sdr_device_bladerf_api_defs.h"

typedef struct {
    quint64     rx_frequency;
    quint32     rx_sample_rate;
    quint32     rx_bandwidth;
    int         rx_gain;
    bool        usb_reset_on_open;
} bladerf_settings_t;

typedef struct {
    const char             *board_name;
    char                    serial[BLADERF_SERIAL_LENGTH];
    struct bladerf_version  fw_version;
    struct bladerf_version  fpga_version;
    bladerf_fpga_size       fpga_size;
    bladerf_dev_speed       dev_speed;
} bladerf_info_t;

namespace Ui {
class SdrDeviceBladerfRxctl;
}

class SdrDeviceBladerfRxctl : public QWidget
{
    Q_OBJECT

public:
    explicit SdrDeviceBladerfRxctl(QWidget *parent = nullptr);
    ~SdrDeviceBladerfRxctl();

    void    readSettings(const bladerf_settings_t settings);

signals:
    void    gainChanged(int gain);
    void    biasChanged(bladerf_channel ch, bool enable);

private slots:
    void    on_gainSlider_valueChanged(int value);
    void    on_bias1Box_toggled(bool checked);
    void    on_bias2Box_toggled(bool checked);

private:
    Ui::SdrDeviceBladerfRxctl *ui;
};

