/*
 * SDRPlay API definitions
 */
#pragma once


// Supported device IDs (copied from V3 API)
#define SDRPLAY_RSP1_ID     1
#define SDRPLAY_RSP1A_ID    255
#define SDRPLAY_RSP2_ID     2
#define SDRPLAY_RSPduo_ID   3

#if defined(ANDROID) || defined(__ANDROID__)
// Android requires a mechanism to request info from Java application
typedef enum
{
  mir_sdr_GetFd              = 0,
  mir_sdr_FreeFd             = 1,
  mir_sdr_DevNotFound        = 2,
  mir_sdr_DevRemoved         = 3,
  mir_sdr_GetVendorId        = 4,
  mir_sdr_GetProductId       = 5,
  mir_sdr_GetRevId           = 6,
  mir_sdr_GetDeviceId        = 7
} mir_sdr_JavaReqT;

typedef int (*mir_sdr_SendJavaReq_t)(mir_sdr_JavaReqT cmd, char *path, char *serNum);
#endif

typedef enum
{
  mir_sdr_Success            = 0,
  mir_sdr_Fail               = 1,
  mir_sdr_InvalidParam       = 2,
  mir_sdr_OutOfRange         = 3,
  mir_sdr_GainUpdateError    = 4,
  mir_sdr_RfUpdateError      = 5,
  mir_sdr_FsUpdateError      = 6,
  mir_sdr_HwError            = 7,
  mir_sdr_AliasingError      = 8,
  mir_sdr_AlreadyInitialised = 9,
  mir_sdr_NotInitialised     = 10,
  mir_sdr_NotEnabled         = 11,
  mir_sdr_HwVerError         = 12,
  mir_sdr_OutOfMemError      = 13,
  mir_sdr_HwRemoved          = 14
} mir_sdr_ErrT;

typedef enum
{
  mir_sdr_BW_Undefined = 0,
  mir_sdr_BW_0_200     = 200,
  mir_sdr_BW_0_300     = 300,
  mir_sdr_BW_0_600     = 600,
  mir_sdr_BW_1_536     = 1536,
  mir_sdr_BW_5_000     = 5000,
  mir_sdr_BW_6_000     = 6000,
  mir_sdr_BW_7_000     = 7000,
  mir_sdr_BW_8_000     = 8000
} mir_sdr_Bw_MHzT;

typedef enum
{
  mir_sdr_IF_Undefined = -1,
  mir_sdr_IF_Zero      = 0,
  mir_sdr_IF_0_450     = 450,
  mir_sdr_IF_1_620     = 1620,
  mir_sdr_IF_2_048     = 2048
} mir_sdr_If_kHzT;

typedef enum
{
  mir_sdr_ISOCH = 0,
  mir_sdr_BULK  = 1
} mir_sdr_TransferModeT;

typedef enum
{
  mir_sdr_CHANGE_NONE    = 0x00,
  mir_sdr_CHANGE_GR      = 0x01,
  mir_sdr_CHANGE_FS_FREQ = 0x02,
  mir_sdr_CHANGE_RF_FREQ = 0x04,
  mir_sdr_CHANGE_BW_TYPE = 0x08,
  mir_sdr_CHANGE_IF_TYPE = 0x10,
  mir_sdr_CHANGE_LO_MODE = 0x20,
  mir_sdr_CHANGE_AM_PORT = 0x40
} mir_sdr_ReasonForReinitT;

typedef enum
{
  mir_sdr_LO_Undefined = 0,
  mir_sdr_LO_Auto      = 1,
  mir_sdr_LO_120MHz    = 2,
  mir_sdr_LO_144MHz    = 3,
  mir_sdr_LO_168MHz    = 4
} mir_sdr_LoModeT;

typedef enum
{
  mir_sdr_BAND_AM_LO   = 0,
  mir_sdr_BAND_AM_MID  = 1,
  mir_sdr_BAND_AM_HI   = 2,
  mir_sdr_BAND_VHF     = 3,
  mir_sdr_BAND_3       = 4,
  mir_sdr_BAND_X       = 5,
  mir_sdr_BAND_4_5     = 6,
  mir_sdr_BAND_L       = 7
} mir_sdr_BandT;

typedef enum
{
  mir_sdr_USE_SET_GR                = 0,
  mir_sdr_USE_SET_GR_ALT_MODE       = 1,
  mir_sdr_USE_RSP_SET_GR            = 2
} mir_sdr_SetGrModeT;

typedef enum
{
  mir_sdr_RSPII_BAND_UNKNOWN = 0,
  mir_sdr_RSPII_BAND_AM_LO   = 1,
  mir_sdr_RSPII_BAND_AM_MID  = 2,
  mir_sdr_RSPII_BAND_AM_HI   = 3,
  mir_sdr_RSPII_BAND_VHF     = 4,
  mir_sdr_RSPII_BAND_3       = 5,
  mir_sdr_RSPII_BAND_X_LO    = 6,
  mir_sdr_RSPII_BAND_X_MID   = 7,
  mir_sdr_RSPII_BAND_X_HI    = 8,
  mir_sdr_RSPII_BAND_4_5     = 9,
  mir_sdr_RSPII_BAND_L       = 10
} mir_sdr_RSPII_BandT;

typedef enum
{
  mir_sdr_RSPII_ANTENNA_A = 5,
  mir_sdr_RSPII_ANTENNA_B = 6
} mir_sdr_RSPII_AntennaSelectT;

typedef enum
{
  mir_sdr_AGC_DISABLE  = 0,
  mir_sdr_AGC_100HZ    = 1,
  mir_sdr_AGC_50HZ     = 2,
  mir_sdr_AGC_5HZ      = 3
} mir_sdr_AgcControlT;

typedef enum
{
  mir_sdr_GAIN_MESSAGE_START_ID  = 0x80000000,
  mir_sdr_ADC_OVERLOAD_DETECTED  = mir_sdr_GAIN_MESSAGE_START_ID + 1,
  mir_sdr_ADC_OVERLOAD_CORRECTED = mir_sdr_GAIN_MESSAGE_START_ID + 2
} mir_sdr_GainMessageIdT;

typedef enum
{
  mir_sdr_EXTENDED_MIN_GR = 0,
  mir_sdr_NORMAL_MIN_GR   = 20
} mir_sdr_MinGainReductionT;

typedef struct
{
   char *SerNo;
   char *DevNm;
   unsigned char hwVer;
   unsigned char devAvail;
} mir_sdr_DeviceT;

typedef struct
{
   float curr;
   float max;
   float min;
} mir_sdr_GainValuesT;

typedef enum
{
   mir_sdr_rspDuo_Tuner_1 = 1,
   mir_sdr_rspDuo_Tuner_2 = 2,
} mir_sdr_rspDuo_TunerSelT;
