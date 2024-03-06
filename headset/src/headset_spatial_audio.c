/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_spatial_audio.c
\brief      Provides Spatial (3D) audio support
*/

#ifdef INCLUDE_SPATIAL_AUDIO

#ifdef INCLUDE_SPATIAL_DATA

#include "headset_spatial_audio.h"
#include "app_sensor_bus.h"

#include "spatial_data.h"

#include <cm_lib.h>
#define DEBUG_LOG_MODULE_NAME spatial_audio
#include <logging.h>
DEBUG_LOG_DEFINE_LEVEL_VAR /* Enable logging for this module. */
#include <message.h>
#include <panic.h>
#include <system_clock.h>
#include <sensor.h>
#include <lsm6ds.h>

#include "multidevice.h"
#include "headset_sm.h"
#include "headset_config.h"
#include "context_framework.h"

#ifdef INCLUDE_ATTITUDE_FILTER
#include "attitude_filter.h"
#endif /* INCLUDE_ATTITUDE_FILTER */

#ifdef INCLUDE_HIDD_PROFILE
#include "hidd_profile.h"

/* #include "headset_production_test.h" - no headset production test yet. */

static const uint8 spatial_audio_hidd_service_record[] =
{
   0x09,0x00,0x01,       /* ServiceClassIDList(0x0001) */
   0x35,0x03,            /* DataElSeq 3 bytes */
   0x19,0x11,0x24,       /* uuid16_t HID(0x1124) */

   0x09,0x00,0x04,       /* ProtocolDescriptorList(0x0004) */
   0x35,0x0d,            /* DataElSeq 13 bytes */
   0x35,0x06,            /* DataElSeq 6 bytes */
   0x19,0x01,0x00,       /* uuid16_t L2CAP(0x0100) */
   0x09,0x00,0x11,       /* CsrUint16 0x0011 (HID_Control PSM) */
   0x35,0x03,            /* DataElSeq 3 bytes */
   0x19,0x00,0x11,       /* uuid16_t HIDP(0x0011) */


   0x09,0x00,0x05,       /* AttrId = BrowseGroupList */
   0x35,0x03,            /* Data element seq. 3 bytes */
   0x19,0x10,0x02,       /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

   0x09,0x00,0x06,       /* LanguageBaseAttributeIDList(0x0006) */
   0x35,0x09,            /* DataElSeq 9 bytes */
   0x09,0x65,0x6e,       /* CsrUint16 0x656e */
   0x09,0x00,0x6a,       /* CsrUint16 0x006a */
   0x09,0x01,0x00,       /* CsrUint16 0x0100 */

   0x09,0x00,0x09,       /* BluetoothProfileDescriptorList(0x0009) */
   0x35,0x08,            /* DataElSeq 8 bytes */
   0x35,0x06,            /* DataElSeq 6 bytes */
   0x19,0x11,0x24,       /* uuid16_t HID(0x1124) */
   0x09,0x01,0x01,       /* CsrUint16 0x0101 */

   0x09,0x00,0x0d,       /* AdditionalProtocolDescriptorList(0x000d) */
   0x35,0x0f,            /* DataElSeq 15 bytes */
   0x35,0x0d,            /* DataElSeq 13 bytes */
   0x35,0x06,            /* DataElSeq 6 bytes */
   0x19,0x01,0x00,       /* uuid16_t L2CAP(0x0100) */
   0x09,0x00,0x13,       /* CsrUint16 0x0013 (HID_Interrupt PSM)*/
   0x35,0x03,            /* DataElSeq 3 bytes */
   0x19,0x00,0x11,       /* uuid16_t HIDP(0x0011) */

   0x09,0x02,0x01,       /* HIDParserVersion(0x0201) = "0x0111" */
   0x09,0x01,0x11,       /* CsrUint16 0x0111 */

   0x09,0x02,0x02,       /* HIDDeviceSubclass(0x0202) = "0x00" */
   0x08,0x00,            /* CsrUint8 0x00 (uncategorized) */

   0x09,0x02,0x03,       /* HIDCountryCode(0x0203) = "0x33" */
   0x08,0x33,            /* CsrUint8 0x33 */

   0x09,0x02,0x04,       /* HIDVirtualCable(0x0204) = "true" */
   0x28,0x01,            /* CsrBool true */

   0x09,0x02,0x05,       /* HIDReconnectInitiate(0x0205) = "true" */
   0x28,0x01,            /* CsrBool true */

    0x09,0x02,0x06,       /* HIDDescriptorList(0x0206) */
    0x35,0xB0,            /* DataElSeq 176 bytes */
    0x35,0xAE,            /* DataElSeq 174 bytes */
    0x08,0x22,            /* CsrUint8 0x22 (Report Descriptor) */

    0x25,0xAA,            /* HID Descriptor Length 170 bytes */

    /* Start of Raw Data  - size 170 bytes */
    0x05,0x20,            /* USAGE_PAGE 0x20 (HID Sensors page (0x20)) */
    0x09,0xE1,            /* USAGE 0x01 (HID_USAGE_SENSOR_TYPE_OTHER_CUSTOM) */
    0xA1,0x01,            /* COLLECTION (Begin Application collection) */

        0x85,0x02,            /* REPORT_ID 0x02 (Feature report 2 (read only)) */

        /* Magic value: "#AndroidHeadTraker#1.0" */
        0x0A,0x08,0x03, 	  /* HID_USAGE_SENSOR_PROPERTY_SENSOR_DESCRIPTION  */
        0x15,0x00,            /* LOGICAL_MIN 0 */
        0x25,0xFF,            /* LOGICAL_MAX 255 (Max value of each uint 8 0xff) */
        0x75,0x08,            /* REPORT_SIZE 8 */
        0x95,0x17,            /* REPORT_COUNT 23 (1 report = 8 bits) */
        0xB1,0x03,            /* FEATURE(Const_Var_Abs) */

        /* UUID */
        0x0A,0x02,0x03,       /* HID_USAGE_SENSOR_PROPERTY_PERSISTENT_UNIQUE_ID */
        0x15,0x00,            /* LOGICAL_MIN 0 */
        0x25,0xFF,            /* LOGICAL_MAX 255 (Max value of each uint 8 0xff) */
        0x75,0x08,            /* REPORT_SIZE 8 */
        0x95,0x10,            /* REPORT_COUNT 16 (1 report = 8 bits) */
        0xB1,0x03,            /* FEATURE(Const_Var_Abs) */

        0x85,0x01,            /* REPORT_ID 0x01 (Feature report 1 (read/write)) */

        /* 1-bit on/off reporting state */
        0x0A,0x16,0x03,	      /* HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE */
        0x15,0x00,            /* LOGICAL_MIN 0 (Logical min is 0x0) */
        0x25,0x01,            /* LOGICAL_MAX 1 (Logical max is 0x1) */
        0x75,0x01,            /* REPORT_SIZE 1 */
        0x95,0x01,            /* REPORT_COUNT 1 (1 report = 8 bits) */
        0xA1,0x02,            /* COLLECTION (Logical) */
          0x0A,0x40,0x08,       /* HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_NO_EVENTS  */
          0x0A,0x41,0x08,       /* HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_ALL_EVENTS */
          0xB1,0x00,            /* FEATURE(Data_Arr_Abs) */
        0xC0,                 /* END_COLLECTION */

        /* 1-bit on/off power state */
        0x0A,0x19,0x03,		 /* HID_USAGE_SENSOR_PROPERTY_POWER_STATE */
        0x15,0x00,            /* LOGICAL_MIN 0 (Logical min is 0x0) */
        0x25,0x01,            /* LOGICAL_MAX 1 (Logical max is 0x1) */
        0x75,0x01,            /* REPORT_SIZE 1 */
        0x95,0x01,            /* REPORT_COUNT 1 (1 report = 8 bits) */
        0xA1,0x02,            /* COLLECTION (Logical) */
          0x0A,0x55,0x08,       /* HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D4_POWER_OFF  */
          0x0A,0x51,0x08,       /* HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D0_FULL_POWER */
          0xB1,0x00,            /* FEATURE(Data_Arr_Abs) */
        0xC0,                 /* END_COLLECTION */

        /* 6-bit reporting interval [0x00 - 0x3F] corresponding to [10ms - 100ms] */
        0x0A,0x0E,0x03,   	 /* HID_USAGE_SENSOR_PROPERTY_REPORT_INTERVAL */
        0x15,0x00,            /* LOGICAL_MIN  0 (Logical min is 0x00) */
        0x25,0x3F,            /* LOGICAL_MAX 63 (Logical max is 0x3F) */
        0x35,0x0A,            /* PHYSICAL_MIN  10 (Physical min is 0x0A) */
        0x45,0x64,            /* PHYSICAL_MAX 100 (Physical max is 0x64) */
        0x75,0x06,            /* REPORT_SIZE 1 */
        0x95,0x01,            /* REPORT_COUNT 1 */
        0x66,0x01,0x10,       /* HID_USAGE_SENSOR_UNITS_SECOND */
        0x55,0x0D,            /* UNIT_EXPONENT 10^-3 */
        0xB1,0x02,            /* FEATURE(Data_Var_Abs) */

        /* Input Report 1 */

        /* Orientation as rotation vector (scaled to [-p1..pi] rad) */
        0x0A,0x44,0x05,       /* HID_USAGE_SENSOR_DATA_CUSTOM_VALUE_1 */
        0x16,0x01,0x80,       /* LOGICAL_MIN -32767 */
        0x26,0xFF,0x7F,       /* LOGICAL_MAX  32767 */
        0x37,0x60,0x4F,0x46,0xED,  /* PHYSICAL_MIN -314159265 */
        0x47,0xA1,0xB0,0xB9,0x12,  /* PHYSICAL_MAX  314159265 */
        0x55,0x08,            /* UNIT_EXPONENT 10^-8 */
        0x75,0x10,            /* REPORT_SIZE 16 */
        0x95,0x03,            /* REPORT_COUNT 3 */
        0x81,0x02,            /* INPUT(Data_Var_Abs) */

        /* Angular velocity as rotation vector (scaled to [-32..32] rad/sec) */
        0x0A,0x45,0x05,       /* HID_USAGE_SENSOR_DATA_CUSTOM_VALUE_2 */
        0x16,0x01,0x80,       /* LOGICAL_MIN -32767 */
        0x26,0xFF,0x7F,       /* LOGICAL_MAX  32767 */
        0x35,0xE0,            /* PHYSICAL_MIN -32 (Physical min is 0xE0) */
        0x45,0x20,            /* PHYSICAL_MAX  32 (Physical max is 0x20) */
        0x55,0x00,            /* UNIT_EXPONENT 10^0 */
        0x75,0x10,            /* REPORT_SIZE 16 */
        0x95,0x03,            /* REPORT_COUNT 3 */
        0x81,0x02,            /* INPUT(Data_Var_Abs) */

        /* Reference frame reset counter */
        0x0A,0x46,0x05,       /* HID_USAGE_SENSOR_DATA_CUSTOM_VALUE_3 */
        0x15,0x00,            /* LOGICAL_MIN 0 */
        0x25,0xFF,            /* LOGICAL_MAX 255 (Max value of each uint 8 0xff) */
        0x35,0x00,            /* PHYSICAL_MIN 0 (Physical min is 0x00) */
        0x45,0xFF,            /* PHYSICAL_MAX 255 (Physical max is 0xFF) */
        0x55,0x00,            /* UNIT_EXPONENT 10^0 */
        0x75,0x08,            /* REPORT_SIZE 8 */
        0x95,0x01,            /* REPORT_COUNT 1 */
        0x81,0x02,            /* INPUT(Data_Var_Abs) */

    0xC0,                 /* END_COLLECTION */
    /* End of Raw Data */

   0x09,0x02,0x07,       /* HIDLANGIDBaseList(0x0207) */
   0x35,0x08,            /* DataElSeq 8 bytes */
   0x35,0x06,            /* DataElSeq 6 bytes */
   0x09,0x04,0x09,       /* CsrUint16 0x0409 */
   0x09,0x01,0x00,       /* CsrUint16 0x0100 */

   0x09,0x02,0x09,       /* HIDBatteryPower(0x0209) = "true" */
   0x28,0x01,            /* CsrBool true */

   0x09,0x02,0x0a,       /* HIDRemoteWake(0x020a) = "false" */
   0x28,0x00,            /* CsrBool false */

   0x09,0x02,0x0c,       /* HIDSupervisionTimeout(0x020c) = "0x1f40" */
   0x09,0x1f,0x40,       /* CsrUint16 0x1f40 */

   0x09,0x02,0x0d,       /* HIDNormallyConnectable(0x020d) = "true" */
   0x28,0x01,            /* CsrBool true */

   0x09,0x02,0x0e,       /* HIDBootDevice(0x020e) = "false" */
   0x28,0x00,            /* CsrBool false */

   0x09,0x02,0x0f,       /* HIDSSRHostMaxLatency (0x020f) */
   0x09,0x1f,0x40,       /* CsrUint16 0x1f40 (5s) */

   0x09,0x02,0x10,       /* HIDSSRHostMinTimeout (0x0210) */
   0x09,0x1f,0x40,       /* CsrUint16 0x1f40 (5s) */

    0x09,0x01,0x00,       /* ServiceName(0x0100) = "Headset" */
    0x25,0x07,            /* String length 7 */
    'H', 'e', 'a', 'd', 's', 'e', 't',

   0x09,0x01,0x01,       /* ServiceDescription(0x0101) = "Motion RDP" */
   0x25,0x0a,            /* String length 10 */
   'M', 'o', 't', 'i', 'o', 'n', ' ', 'R', 'D', 'P',

   0x09,0x01,0x02,      /* ProviderName(0x0102) = ”qualcomm” */
   0x25,0x08,           /* String length 8 */
   'q', 'u', 'a', 'l', 'c', 'o', 'm', 'm'
};
#endif /* INCLUDE_HIDD_PROFILE */

#ifdef INCLUDE_DEVICE_INFO_SERVICE_RECORD
static const uint8 spatial_audio_pnp_record[] =
{
  0x09, /* ServiceClassIDList(0x0001) */
    0x00,
    0x01,
  0x35, /* DataElSeq 3 bytes */
  0x03,
    0x19, /* uuid PnPInformation(0x1200) */
    0x12,
    0x00,
  0x09, /* ProtocolDescriptorList(0x0004) */
    0x00,
    0x04,
  0x35, /* DataElSeq 13 bytes */
  0x0d,
    0x35, /* DataElSeq 6 bytes */
    0x06,
      0x19, /* uuid L2CAP(0x0100) */
      0x01,
      0x00,
      0x09, /* uint16 0x0001 */
        0x00,
        0x01,
    0x35, /* DataElSeq 3 bytes */
    0x03,
      0x19, /* uuid SDP(0x0001) */
      0x00,
      0x01,
  0x09, /* LanguageBaseAttributeIDList(0x0006) */
    0x00,
    0x06,
  0x35, /* DataElSeq 9 bytes */
  0x09,
    0x09, /* uint16 0x656e */
      0x65,
      0x6e,
    0x09, /* uint16 0x006a */
      0x00,
      0x6a,
    0x09, /* uint16 0x0100 */
      0x01,
      0x00,
  0x09, /* BluetoothProfileDescriptorList(0x0009) */
    0x00,
    0x09,
  0x35, /* DataElSeq 8 bytes */
  0x08,
    0x35, /* DataElSeq 6 bytes */
    0x06,
      0x19, /* uuid PnPInformation(0x1200) */
      0x12,
      0x00,
      0x09, /* uint16 0x0100 */
        0x01,
        0x00,
  0x09, /* ServiceDescription(0x0101) = "Motion RDP" */
    0x01,
    0x01,
  0x25, /* String length 10 */
  0x0a,
    'M','o','t','i','o','n',' ','R','D','P',
  0x09, /* SpecificationID(0x0200) = "0x0100" */
    0x02,
    0x00,
  0x09, /* uint16 0x0100 */
    0x01,
    0x00,
  0x09, /* VendorID(0x0201) = "0x05C6" */
    0x02,
    0x01,
  0x09, /* uint16 0x05c6 */
    0x05,
    0xc6,
  0x09, /* ProductID(0x0202) = "0x9801" */
    0x02,
    0x02,
  0x09, /* uint16 0x9801 */
    0x98,
    0x01,
  0x09, /* Version(0x0203) = "0x0001" */
    0x02,
    0x03,
  0x09, /* uint16 0x0001 */
    0x00,
    0x01,
  0x09, /* PrimaryRecord(0x0204) = "TRUE" */
    0x02,
    0x04,
  0x28, /* bool true */
    0x01,
  0x09, /* VendorIDSource(0x0205) = "0x0002" */
    0x02,
    0x05,
  0x09, /* uint16 0x0002 */
    0x00,
    0x02,
}; /* 103 bytes */
#endif

#define SPATIAL_AUDIO_REPORT_RATE 100

/*! Motion sensor chip details passed to the domains layer. */
static SPATIAL_DATA_MOTION_SENSOR_CHIP_CONFIG_T motion_sensor_chip_config;

/* Static report data for external applications to fetch when they want. */
/* Changes to this must be agreed with all external usage. */
static SPATIAL_DATA_REPORT_DATA_IND_T spatial_data_report;

/*! Local functions */
static bool headsetSpatialAudio_ContextFrameworkProvider(unsigned * context_data, uint8 context_data_size);
static void headsetSpatialAudio_Register(void);
static void headsetSpatialAudio_TaskHandler(Task task, MessageId id, Message message);
static TaskData headsetSpatialAudioTask = {headsetSpatialAudio_TaskHandler};



bool HeadsetSpatialAudio_Init(Task task)
{
    UNUSED(task);
    DEBUG_LOG_DEBUG("%s", __func__);

    SpatialData_InitData(task);

    headsetSpatialAudio_Register();

    ContextFramework_RegisterContextProvider(context_spatial_audio_info, headsetSpatialAudio_ContextFrameworkProvider);
    return TRUE;
}

bool HeadsetSpatialAudio_Enable(uint16 sensor_sample_rate_hz, spatial_data_report_id_t report_id)
{
    DEBUG_LOG_INFO("%s:Sensor sample rate:%d Data report:%d", __func__, sensor_sample_rate_hz, report_id);

    return SpatialData_Enable(spatial_data_enabled_local, sensor_sample_rate_hz, report_id);
}

bool HeadsetSpatialAudio_Disable(void)
{
    DEBUG_LOG_INFO("%s", __func__);

    return SpatialData_Enable(spatial_data_disabled, 0, 0);
}

SPATIAL_DATA_REPORT_DATA_IND_T *HeadsetSpatialAudio_GetLatestReport(void)
{
    return &spatial_data_report;
}

static bool headsetSpatialAudio_ContextFrameworkProvider(unsigned * context_data, uint8 context_data_size)
{
    DEBUG_LOG_VERBOSE("%s",__func__);
    PanicZero(context_data_size >= sizeof(context_spatial_audio_info_t));
    memset(context_data, 0, sizeof(context_spatial_audio_info_t));

    if (spatial_data_report.report_id == spatial_data_report_disabled)
    {
        DEBUG_LOG_VERBOSE("%s - report disabled so enabling", __func__);
        /* We do not attempt to change the report type if already enabled. */
        HeadsetSpatialAudio_Enable(SPATIAL_AUDIO_REPORT_RATE, spatial_data_report_1);
        return FALSE;
    }

    memcpy(context_data, &spatial_data_report, sizeof (context_spatial_audio_info_t));
    return TRUE;
}

static void headsetSpatialAudio_Register(void)
{
    DEBUG_LOG_DEBUG("%s",__func__);

    Bus bus_id;
    Sensor sensor_id;
    uint32 data_read;

    spatial_data_source_t data_source = spatial_data_head;

#ifdef PRODUCTION_TEST_MODE
    /* If we are running in production test mode, we don't want to enable
     * sensor hub as it uses the bitserial that is required by the
     * production test.
     */
    if (sm_boot_production_test_mode == appSmTestService_BootMode())
    {
        DEBUG_LOG_DEBUG("Not acquiring Sensor Hub due to production test");
        return;
    }
#endif

    /* Get the SPI/I2C and Sensor IDs for the motion sensor chip. */
    bus_id = AppSensorBus_Bus();

    /* We're using LSM6DS motion sensor chip. */
    sensor_id = SensorLsm6ds(bus_id);
    PanicFalse(sensor_id != 0);

    if (SensorGetConfiguration(sensor_id, SENSOR_CONFIG_LSM6DS_ACCEL_G, &data_read))
    {
        motion_sensor_chip_config.accel_scale = (uint16)data_read;
    }

    if (SensorGetConfiguration(sensor_id, SENSOR_CONFIG_LSM6DS_GYRO_DPS, &data_read))
    {
        motion_sensor_chip_config.gyro_scale = (uint16)data_read;
    }

    /* Initialise the motion sensor chip by passing the Bus/Sensor ID and config data to spatial data. */
    PanicFalse(SpatialData_InitMotionSensor(bus_id, sensor_id, &motion_sensor_chip_config));

#ifdef INCLUDE_DEVICE_INFO_SERVICE_RECORD
    /* Register PnP service record */
    ConnectionRegisterServiceRecord(&headsetSpatialAudioTask, sizeof(spatial_audio_pnp_record), spatial_audio_pnp_record);
#endif

#ifdef INCLUDE_HIDD_PROFILE
    /* Register HIDD with protocol descriptors. */
    PanicFalse(SpatialData_SetHiddSdp(sizeof(spatial_audio_hidd_service_record), spatial_audio_hidd_service_record));
#endif /* INCLUDE_HIDD_PROFILE */

    PanicFalse(SpatialData_Register(&headsetSpatialAudioTask, data_source));
}

static void headsetSpatialAudio_HandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    if (*prim == CSR_BT_CM_SDS_REGISTER_CFM)
    {
        DEBUG_LOG("%s: SDS_REGISTER_CFM %d", __func__,
                  (((const CsrBtCmSdsRegisterCfm *) message)->resultCode));
    }
    else
    {
        DEBUG_LOG("%s Unhandled CM Prim 0x%04x", __func__, *prim);
    }
    CmFreeUpstreamMessageContents((void * ) prim);
}


static void headsetSpatialAudio_TaskHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    rtime_t t_start = SystemClockGetTimerTime();

    switch (id)
    {
        case SPATIAL_DATA_ENABLED_STATUS_IND:
        {
            SPATIAL_DATA_ENABLE_STATUS_IND_T *spatial_message = (SPATIAL_DATA_ENABLE_STATUS_IND_T *)message;
            PanicNull(spatial_message);
            DEBUG_LOG_INFO("SPATIAL_DATA ENABLE status:%d with sampling rate:%dHz", spatial_message->enable_status, spatial_message->sensor_sample_rate_hz);
        }
        break;

        case SPATIAL_DATA_REPORT_DATA_IND:
        {
            SPATIAL_DATA_REPORT_DATA_IND_T *spatial_data = (SPATIAL_DATA_REPORT_DATA_IND_T *)message;
            PanicNull(spatial_data);

            static rtime_t prev_sensor_data_timestamp = 0;
            rtime_t delta_time = spatial_data->timestamp - prev_sensor_data_timestamp;

            memcpy(&spatial_data_report, spatial_data, sizeof(spatial_data_report));

            switch (spatial_data->report_id)
            {
                case spatial_data_report_1:
#ifdef INCLUDE_ATTITUDE_FILTER
                DEBUG_LOG_V_VERBOSE("t = %10u, SPATIAL_DATA_REPORT: ts:%10u Time Delta:%6d Source:%d Temp:%dC Gyro:X:%d Y:%d Z:%d Accel:X:%d Y:%d Z:%d v:%d Quat:W:%d X:%d Y:%d Z:%d",
                                  t_start,
                                  spatial_data->timestamp,
                                  delta_time,
                                  spatial_data->data_source,
                                  spatial_data->report_data.debug_data.temp,
                                  spatial_data->report_data.debug_data.gyro.x, spatial_data->report_data.debug_data.gyro.y, spatial_data->report_data.debug_data.gyro.z,
                                  spatial_data->report_data.debug_data.accel.x, spatial_data->report_data.debug_data.accel.y, spatial_data->report_data.debug_data.accel.z,
                                  spatial_data->report_data.debug_data.valid,
                                  spatial_data->report_data.debug_data.quaternion.w, spatial_data->report_data.debug_data.quaternion.x,
                                  spatial_data->report_data.debug_data.quaternion.y, spatial_data->report_data.debug_data.quaternion.z);
#else
                DEBUG_LOG_V_VERBOSE("t = %10u, spatial_data_report_1: ts:%10u Time Delta:%6d Source:%d Temp:%dC Gyro:X:%d Y:%d Z:%d Accel:X:%d Y:%d Z:%d",
                                    t_start,
                                    spatial_data->timestamp,
                                    delta_time,
                                    spatial_data->data_source,
                                    spatial_data->report_data.raw_data.temp,
                                    spatial_data->report_data.raw_data.gyro.x, spatial_data->report_data.raw_data.gyro.y, spatial_data->report_data.raw_data.gyro.z,
                                    spatial_data->report_data.raw_data.accel.x, spatial_data->report_data.raw_data.accel.y, spatial_data->report_data.raw_data.accel.z);
#endif /* INCLUDE_ATTITUDE_FILTER */
                break;

                default:
                {
                    DEBUG_LOG_DEBUG("SPATIAL_DATA_REPORT:Unsupported report_id:%d",spatial_data->report_id);
                }

            }
            prev_sensor_data_timestamp = spatial_data->timestamp;

            ContextFramework_NotifyContextUpdate(context_spatial_audio_info);
        }
        break;

        case SPATIAL_DATA_ERROR_MESSAGE_IND:
        {
            SPATIAL_DATA_ERROR_IND_T *spatial_data = (SPATIAL_DATA_ERROR_IND_T *)message;
            PanicNull(spatial_data);
            DEBUG_LOG_ERROR("SPATIAL_DATA_ERROR_MESSAGE_IND:%d", spatial_data->spatial_error);
        }
        break;

#ifdef INCLUDE_ATTITUDE_FILTER
        case SPATIAL_DATA_SET_SLOW_FILTER_TO_NORMAL:
        {
            DEBUG_LOG_DEBUG("t = %10u %s:SPATIAL_DATA_SET_SLOW_FILTER_TO_NORMAL", SystemClockGetTimerTime(), __func__);
            Attitude_HeadAutoCalFilterSet(1, 1, 13); /* Default recentring (1 / 2^13) correction every sample. */
        }
        break;
#endif /* INCLUDE_ATTITUDE_FILTER */

#ifdef USE_SYNERGY
        case CM_PRIM:
            headsetSpatialAudio_HandleCmPrim(message);
            break;
#endif

        default:
        {
            DEBUG_LOG_DEBUG("%s:Unhandled message:%d", __func__, id);
        }
        break;
    }
}

#endif /* INCLUDE_SPATIAL_DATA */
#endif /* INCLUDE_SPATIAL_AUDIO */
