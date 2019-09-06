/*
 * SDRplay user interface
 */
#pragma once

#include <QWidget>

#include "sdr_device_sdrplay_api_defs.h"

namespace Ui {
class SdrDeviceSdrplayRxctl;
}

typedef struct {
    quint64             frequency;
    quint32             sample_rate;
    mir_sdr_Bw_MHzT     bandwidth;
    int                 lna_state;
    int                 grdb;           // gRdB
    mir_sdr_SetGrModeT  gain_mode;
    mir_sdr_LoModeT     lo_mode;
    mir_sdr_If_kHzT     if_mode;
} sdrplay_settings_t;

class SdrDeviceSdrplayRxctl : public QWidget
{
    Q_OBJECT

public:
    explicit SdrDeviceSdrplayRxctl(QWidget *parent = nullptr);
    ~SdrDeviceSdrplayRxctl();

    void    setSystemGainValue(float gain);
    void    readSettings(const sdrplay_settings_t &settings);

signals:
    void    lnaStateChanged(int lna_state);
    void    grdbChanged(int grdb);
    void    debugChanged(bool debug_on);

private slots:
    void    on_rfSlider_valueChanged(int value);
    void    on_ifSlider_valueChanged(int value);
    void    on_debugCheckBox_toggled(bool checked);

private:
    Ui::SdrDeviceSdrplayRxctl *ui;
};
