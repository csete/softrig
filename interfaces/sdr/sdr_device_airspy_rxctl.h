/*
 * Airspy user interface
 */
#pragma once

#include <QSlider>
#include <QString>
#include <QWidget>

namespace Ui {
class SdrDeviceAirspyRxctl;
}

typedef struct {
    quint64     frequency;
    quint32     sample_rate;
    quint32     bandwidth;
    QString     gain_mode;
    int         linearity_gain;
    int         sensitivity_gain;
    int         lna_gain;
    int         mixer_gain;
    int         vga_gain;
    bool        bias_on;
} airspy_settings_t;

class SdrDeviceAirspyRxctl : public QWidget
{
    Q_OBJECT

public:
    explicit SdrDeviceAirspyRxctl(QWidget *parent = nullptr);
    ~SdrDeviceAirspyRxctl();

    void readSettings(const airspy_settings_t &settings);

signals:
    void gainModeChanged(const QString &mode);
    void linearityGainChanged(int gain);
    void sensitivityGainChanged(int gain);
    void lnaGainChanged(int gain);
    void mixerGainChanged(int gain);
    void vgaGainChanged(int gain);
    void agcToggled(bool agc_enabled);
    void biasToggled(bool bias_enabled);

private slots:
    void on_gainModeCombo_currentTextChanged(const QString &text);
    void on_linGainSlider_valueChanged(int value);
    void on_sensGainSlider_valueChanged(int value);
    void on_lnaGainSlider_valueChanged(int value);
    void on_mixGainSlider_valueChanged(int value);
    void on_vgaGainSlider_valueChanged(int value);
    void on_biasCheckBox_stateChanged(int state);

private:
    Ui::SdrDeviceAirspyRxctl *ui;

    bool agc_is_on;
};

