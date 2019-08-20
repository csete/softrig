/*
 * Application configuration
 */
#pragma once

#include <QSettings>
#include <QtCore>

typedef struct
{
    QString     type;
    quint64     frequency;
    qint64      nco;
    qint64      transverter;
    quint32     rate;
    quint32     decimation;
    quint32     bandwidth;
    qint32      freq_corr_ppb;
} device_config_t;

typedef struct
{

} audio_config_t;

typedef struct
{
    unsigned int        version;
    device_config_t     input;
    audio_config_t      audio;
} app_config_t;

// error codes
#define APP_CONFIG_OK           0
#define APP_CONFIG_EINVAL       1   // invalid function paramter
#define APP_CONFIG_EFILE        2   // error loading or saving file
#define APP_CONFIG_EDATA        3   // data error (e.g. missing required config record)

class AppConfig
{
public:
    explicit    AppConfig();
    virtual    ~AppConfig();

    int     load(const QSettings &settings);
    void    save(QSettings &settings);

    app_config_t    *getDataPtr(void) {
        return &app_config;
    }

private:
    void    readDeviceConf(const QSettings &settings);
    void    saveDeviceConf(QSettings &settings);

private:
    app_config_t        app_config;
};
