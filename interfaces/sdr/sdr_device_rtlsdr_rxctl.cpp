#include "sdr_device_rtlsdr_rxctl.h"
#include "ui_sdr_device_rtlsdr_rxctl.h"

SdrDeviceRtlsdrRxCtl::SdrDeviceRtlsdrRxCtl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SdrDeviceRtlsdrRxCtl)
{
    ui->setupUi(this);
    setProperty("desc", QString("RTL-SDR"));
}

SdrDeviceRtlsdrRxCtl::~SdrDeviceRtlsdrRxCtl()
{
    delete ui;
}
