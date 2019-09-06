/*
 * SDRPlay API
 */
#pragma once

#include "sdr_device_sdrplay_api_defs.h"

// Application code should check that it is compiled against the same API version
// mir_sdr_ApiVersion() returns the API version
#define MIR_SDR_API_VERSION   (float)(2.13)

// mir_sdr_StreamInit() callback function prototypes
typedef void (*mir_sdr_StreamCallback_t)(short *xi, short *xq, unsigned int firstSampleNum, int grChanged, int rfChanged, int fsChanged, unsigned int numSamples, unsigned int reset, unsigned int hwRemoved, void *cbContext);
typedef void (*mir_sdr_GainChangeCallback_t)(unsigned int gRdB, unsigned int lnaGRdB, void *cbContext);

static mir_sdr_ErrT (*mir_sdr_SetRf)(double drfHz, int abs, int syncUpdate);
static mir_sdr_ErrT (*mir_sdr_SetFs)(double dfsHz, int abs, int syncUpdate, int reCal);
//static mir_sdr_ErrT (*mir_sdr_SetGr)(int gRdB, int abs, int syncUpdate);
//static mir_sdr_ErrT (*mir_sdr_SetGrParams)(int minimumGr, int lnaGrThreshold);
//static mir_sdr_ErrT (*mir_sdr_SetDcMode)(int dcCal, int speedUp);
//static mir_sdr_ErrT (*mir_sdr_SetDcTrackTime)(int trackTime);
//static mir_sdr_ErrT (*mir_sdr_SetSyncUpdateSampleNum)(unsigned int sampleNum);
//static mir_sdr_ErrT (*mir_sdr_SetSyncUpdatePeriod)(unsigned int period);
static mir_sdr_ErrT (*mir_sdr_ApiVersion)(float *version);    // Called by application to retrieve version of API used to create Dll
//static mir_sdr_ErrT (*mir_sdr_ResetUpdateFlags)(int resetGainUpdate, int resetRfUpdate, int resetFsUpdate);
#if defined(ANDROID) || defined(__ANDROID__)
// This function provides a machanism for the Java application to set
// the callback function used to send request to it
static mir_sdr_ErrT mir_sdr_SetJavaReqCallback(mir_sdr_SendJavaReq_t sendJavaReq);
#endif
//static mir_sdr_ErrT (*mir_sdr_SetTransferMode)(mir_sdr_TransferModeT mode);
//static mir_sdr_ErrT (*mir_sdr_DownConvert)(short *in, short *xi, short *xq, unsigned int samplesPerPacket, mir_sdr_If_kHzT ifType, unsigned int M, unsigned int preReset);
//static mir_sdr_ErrT (*mir_sdr_SetPpm)(double ppm);                              // This MAY be called before mir_sdr_Init()
//static mir_sdr_ErrT (*mir_sdr_SetLoMode)(mir_sdr_LoModeT loMode);               // This MUST be called before mir_sdr_Init()/mir_sdr_StreamInit() - otherwise use mir_sdr_Reinit()
//static mir_sdr_ErrT (*mir_sdr_SetGrAltMode)(int *gRidx, int LNAstate, int *gRdBsystem, int abs, int syncUpdate);
//static mir_sdr_ErrT (*mir_sdr_DCoffsetIQimbalanceControl)(unsigned int DCenable, unsigned int IQenable);
//static mir_sdr_ErrT (*mir_sdr_DecimateControl)(unsigned int enable, unsigned int decimationFactor, unsigned int wideBandSignal);
//static mir_sdr_ErrT (*mir_sdr_AgcControl)(mir_sdr_AgcControlT enable, int setPoint_dBfs, int knee_dBfs, unsigned int decay_ms, unsigned int hang_ms, int syncUpdate, int LNAstate);
static mir_sdr_ErrT (*mir_sdr_StreamInit)(int *gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType, mir_sdr_If_kHzT ifType, int LNAstate, int *gRdBsystem, mir_sdr_SetGrModeT setGrMode, int *samplesPerPacket, mir_sdr_StreamCallback_t StreamCbFn, mir_sdr_GainChangeCallback_t GainChangeCbFn, void *cbContext);
static mir_sdr_ErrT (*mir_sdr_StreamUninit)(void);
static mir_sdr_ErrT (*mir_sdr_Reinit)(int *gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType, mir_sdr_If_kHzT ifType, mir_sdr_LoModeT loMode, int LNAstate, int *gRdBsystem, mir_sdr_SetGrModeT setGrMode, int *samplesPerPacket, mir_sdr_ReasonForReinitT reasonForReinit);
static mir_sdr_ErrT (*mir_sdr_DebugEnable)(unsigned int enable);
static mir_sdr_ErrT (*mir_sdr_GetCurrentGain)(mir_sdr_GainValuesT *gainVals);
//static mir_sdr_ErrT (*mir_sdr_GainChangeCallbackMessageReceived)(void);

static mir_sdr_ErrT (*mir_sdr_GetDevices)(mir_sdr_DeviceT *devices, unsigned int *numDevs, unsigned int maxDevs);
static mir_sdr_ErrT (*mir_sdr_SetDeviceIdx)(unsigned int idx);
static mir_sdr_ErrT (*mir_sdr_ReleaseDeviceIdx)(void);

static mir_sdr_ErrT (*mir_sdr_GetHwVersion)(unsigned char *ver);
//static mir_sdr_ErrT (*mir_sdr_RSPII_AntennaControl)(mir_sdr_RSPII_AntennaSelectT select);
//static mir_sdr_ErrT (*mir_sdr_RSPII_ExternalReferenceControl)(unsigned int output_enable);
//static mir_sdr_ErrT (*mir_sdr_RSPII_BiasTControl)(unsigned int enable);
//static mir_sdr_ErrT (*mir_sdr_RSPII_RfNotchEnable)(unsigned int enable);

static mir_sdr_ErrT (*mir_sdr_RSP_SetGr)(int gRdB, int LNAstate, int abs, int syncUpdate);
static mir_sdr_ErrT (*mir_sdr_RSP_SetGrLimits)(mir_sdr_MinGainReductionT minGr);

//static mir_sdr_ErrT (*mir_sdr_AmPortSelect)(int port);
//static mir_sdr_ErrT (*mir_sdr_rsp1a_BiasT)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rsp1a_DabNotch)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rsp1a_BroadcastNotch)(int enable);

//static mir_sdr_ErrT (*mir_sdr_rspDuo_TunerSel)(mir_sdr_rspDuo_TunerSelT sel);
//static mir_sdr_ErrT (*mir_sdr_rspDuo_ExtRef)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rspDuo_BiasT)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rspDuo_Tuner1AmNotch)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rspDuo_BroadcastNotch)(int enable);
//static mir_sdr_ErrT (*mir_sdr_rspDuo_DabNotch)(int enable);
