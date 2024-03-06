/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       spatial_data.h
    \defgroup   spatial_data Spatial Data
    @{
        \ingroup    sensor_domain
        \brief      Provides Spatial data support.

                    This module is responsible for returning spatial data (raw motion sensor or orientation) by enabling motion sensor chip
                    and converting the raw motion data to head or left/right earbud orientation using attitude filter algorithm.
*/

#ifndef SPATIAL_DATA_H
#define SPATIAL_DATA_H

#ifdef INCLUDE_SPATIAL_DATA

#include "domain_message.h"

#include <sensor.h>
#include <rtime.h>
#include <message.h>

/* Maximum number of bytes that can be transmitted along with the orientation data to other earbud.
 * The sync bytes are limited to fit the combined data into one BT slot.
 */
#define SPATIAL_DATA_MAX_ALLOWED_SYNC_DATA_LENGTH_BYTES  20

/*! \brief Spatial data messages sent to the registered client task */
typedef enum
{
    /*! Spatial data enable status */
    SPATIAL_DATA_ENABLED_STATUS_IND = SPATIAL_DATA_MESSAGE_BASE,

    /*! Spatial data containing the requested data report.*/
    SPATIAL_DATA_REPORT_DATA_IND,

    /*! Spatial data sync data with timestamp */
    SPATIAL_DATA_SYNC_DATA_IND,

    /*! Spatial data error indication */
    SPATIAL_DATA_ERROR_MESSAGE_IND,

    /*! Set autocal back to slow recentring after a fast recentre. */
    SPATIAL_DATA_SET_SLOW_FILTER_TO_NORMAL,

    SPATIAL_DATA_MESSAGE_END
} spatial_data_messages_t;

/*! \brief Spatial data enable status */
typedef enum
{
    /*! Spatial data disabled */
    spatial_data_disabled,

    /*! Spatial data enabled for local audio processing */
    spatial_data_enabled_local,

    /*! Spatial data enabled for remote audio processing */
    spatial_data_enabled_remote
} spatial_data_enable_status_t;

/*! \brief Spatial data error values */
typedef enum
{
    /*! Spatial data disabled in secondary when it's enabled in the primary */
    spatial_data_secondary_disabled,

    /*! Spatial data enabled in secondary when it's not enabled in the primary */
    spatial_data_secondary_enabled,

    /*! Attempt to enable spatial data in secondary failed in primary */
    spatial_data_secondary_enable_attempt_failed,

    /*! Attempt to disable spatial data in secondary failed in primary */
    spatial_data_secondary_disable_attempt_failed
} spatial_data_error_t;

/*! \brief Spatial data source */
typedef enum
{
    /*! Spatial data for the left earbud */
    spatial_data_left,

    /*! Spatial data for the right earbud */
    spatial_data_right,

    /*! Spatial data for the head */
    spatial_data_head
} spatial_data_source_t;

/*! \brief Spatial data reports IDs */
typedef enum
{
    /* NOTE: Do not change the below enum values as it's also
     * used for sending data to a remote entity (e.g. phone).
     */

    /*! Report disabled */
    spatial_data_report_disabled =   0x00,

    /*! Head Trackig Input Report : Orientation, Angular Velocitiy, Reference Frame Count */
    spatial_data_report_1        =   0x01,
} spatial_data_report_id_t;

/*! \brief Motion data (accelerometer, gyro, magnetometer) */
typedef struct
{
    /*! X-axis */
    int16 x;

    /*! Y-axis */
    int16 y;

    /*! Z-axis */
    int16 z;
} motion_data_t;

/*! \brief Quaternion data */
typedef struct
{
    /*! W (real) value */
    int16 w;

    /*! X-axis */
    int16 x;

    /*! Y-axis */
    int16 y;

    /*! Z-axis */
    int16 z;
} quaternion_data_t;

/*! \brief Spatial data structure for motion sensor readings and orientation info */
typedef struct
{
    /*! Current value of temperature */
    int16 temp;

    /*! Current values of Gyroscope */
    motion_data_t gyro;

    /*! Current values of Accelerometer */
    motion_data_t accel;

    /*! Quaternion values */
    bool valid;
    quaternion_data_t quaternion;
} spatial_data_report_debug_data_t;

/*! \brief Spatial data message indication containing report data. Changes to this must be agreed with all external usage. */
typedef struct
{
    /*! Spatial data report ID. */
    spatial_data_report_id_t  report_id;

    /*! Spatial data source. */
    spatial_data_source_t data_source;

    /*! Current value of motion sensor timestamp */
    uint32 timestamp;

    struct report_data
    {
        spatial_data_report_debug_data_t      debug_data;
    } report_data;

} SPATIAL_DATA_REPORT_DATA_IND_T;

/*! \brief Spatial data indication message with sync data */
typedef struct
{
    /*! Local timestamp (synchronised with the other earbud) to act on the value  */
    rtime_t  timestamp;

    /* ! Sync data command. */
    uint8 command;

    /*! Data length */
    uint16 data_len;

    /*! Data */
    uint8 data[SPATIAL_DATA_MAX_ALLOWED_SYNC_DATA_LENGTH_BYTES];
} SPATIAL_DATA_SYNC_DATA_IND_T;

/*! \brief Spatial data enabled/disabled indication message */
typedef struct
{
    /*! Spatial data enable status */
    spatial_data_enable_status_t enable_status;

    /*! The sensor sampling interval configured by the driver when spatial data is enabled */
    uint16 sensor_sample_rate_hz;
} SPATIAL_DATA_ENABLE_STATUS_IND_T;

/*! \brief Spatial data error message indication containing error events */
typedef struct
{
    /*! Error message */
    spatial_data_error_t spatial_error;
} SPATIAL_DATA_ERROR_IND_T;

/*! \brief Spatial data motion sensor chip configuration to be sent to remote device (phone/PC) over HID */
typedef struct
{
    /*! Accelerometer scale configuration */
    uint16 accel_scale;

    /*! Gyrometer scale configuration */
    uint16 gyro_scale;
} SPATIAL_DATA_MOTION_SENSOR_CHIP_CONFIG_T;

#ifdef INCLUDE_SENSOR_PROFILE
/*! \brief Is spatial data enabled and is this primary.

    \param None

    \return TRUE if spatial data enabled and this is primary.
*/
bool SpatialData_EnabledPrimary(void);
#endif /* INCLUDE_SENSOR_PROFILE */

/*! \brief Initialise spatial data functionality

    \param task Unused parameter

    \return None
*/
void SpatialData_InitData(Task task);

/*! \brief Initialise motion sensor functionality in spatial data

    \param bus_id SPI/I2C Bus ID connected to the motion sensor chip
    \param sensor_id Motion sensor ID
    \param config_ptr - Pointer passing the motion sensor chip configuration.

    \return TRUE if initialisation is successful, else FALSE
*/
bool SpatialData_InitMotionSensor(Bus bus_id, Sensor sensor_id, const SPATIAL_DATA_MOTION_SENSOR_CHIP_CONFIG_T *config_ptr);

/*! \brief Register with spatial data layer

    NOTE: Only one client task can be registered with spatial data at a time.

    \param client_task Client task for forwarding the spatial data messages
    \param data_source Data source generating spatial data

    \return TRUE if registration is successful, else FALE
*/
bool SpatialData_Register(Task client_task, spatial_data_source_t data_source);

/*! \brief setup HIDD with SDP/HID descriptors

    This function will register/setup HIDD profile with the input SDP/HID descriptors during the device init phase.

    \param data_len Length of the input SDP/HIDD descriptor
    \param data_ptr Pointer to the input SDP/HIDD descriptor

    \return TRUE if the setup is successful, else FALSE.
*/
bool SpatialData_SetHiddSdp(uint16 data_len, const uint8 *data_ptr);

/*! \brief Enable/disable spatial data

    This function will enable/disable spatial data by controlling motion sensor chip.
    SPATIAL_DATA_ENABLED_STATUS_IND message shall be sent to the registered client when spatial data is enabled/disabled.

    The SPATIAL_DATA_ENABLED_STATUS_IND will also contain the sensor sample rate actually configured by the driver 
    closest to the input value requested in this function.

    The spatial data report (SPATIAL_DATA_REPORT_DATA_IND message) at regular intervals (same as the configured sensor sample rate) will be
    sent to the registered client application.

    In earbud use cases, enabling/disabling spatial data should be done only in primary.
    Once the spatial data is enabled/disabled in the primary, the spatial_data module in the primary will automatically enable/disable
    the spatial audio in the secondary over the BT sensor profile in the buddy link.
    The client application in both primary and secondary will receive the SPATIAL_DATA_ENABLED_STATUS_IND and SPATIAL_DATA_REPORT_DATA_IND.

    \param enable_input - Spatial data enable input
    \param sensor_sample_rate_hz - Sensor sample rate in Hz. Spatial data will set the default value (100Hz) if the input is zero.
    \param report_id - Requested data report type.

    \return TRUE if enable/disable is initiated successfully, else returns FALSE.
*/
bool SpatialData_Enable(spatial_data_enable_status_t enable_input, uint16 sensor_sample_rate_hz, spatial_data_report_id_t report_id);

/*! \brief Send synchronised data during spatial data

    This function will send synchronised data between earbuds during spatial data when invoked by upper application.
    The input data will be sent along with the orientation data combined into one ACL packet (fitting in one BT slot)
    in the next BT sniff subrate instance in the buddy link.
    
    Upper application can use this function in order to perform synchronised activities in the earbuds during spatial data.
    The local time to act on the input data shall be sent to the registered client in 
    SPATIAL_DATA_SYNC_DATA_IND message along with the input sync_data later on.

    NOTE: The function will return FALSE if there's already a data queued (waiting for the next BT sniff instance) for sending
    to the other earbud. It's expected that the application shall wait for the "SPATIAL_DATA_SYNC_DATA_IND" for the previous data 
    before attempting to send the next one.

    \param data_len - Length of the input data
    \param data_ptr - Pointer to the input data

    \return Return TRUE if data is queued for sending, else returns FALSE.
*/
bool SpatialData_SendSynchronisedData(uint16 data_len, const uint8 *data_ptr);

/*! \brief Reset attitude filter data processing during spatial data 

    Causes a fast recentre to upright facing forward.

    \return TRUE if reset is successful, else FALSE.
*/
bool SpatialData_AttitudeFilterReset(void);

#endif /* INCLUDE_SPATIAL_DATA */

#endif /* SPATIAL_DATA_H */

/*! @} */