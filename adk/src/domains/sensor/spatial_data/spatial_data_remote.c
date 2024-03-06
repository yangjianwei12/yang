/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       spatial_data_remote.c
    \ingroup    spatial_data
    \brief      Provides spatial data transport with remote device over HID.
*/

#ifdef INCLUDE_SPATIAL_DATA

#ifdef INCLUDE_HIDD_PROFILE

#include "attitude_filter.h"
#include "spatial_data.h"
#define DEBUG_LOG_MODULE_NAME spatial_data
#include "spatial_data_private.h"
#include "hidd_profile.h"
#include "csr_bt_profiles.h"

/*! HIDD Feature Reports ID */
typedef enum 
{
    head_tracking_sensor_reporting  = 0x01,
    head_tracking_sensor_info       = 0x02
} spatial_data_hidd_feature_report_id_t;

/* HIDD report lengths - Head Tracking Report */
#define SPATIAL_DATA_HIDD_REPORT_1_LEN   13

/* HID report lengths for Head Tracking Protocol */
#define HEAD_TRACKING_SENSOR_INFO_LEN         (39)
#define HEAD_TRACKING_SENSOR_REPORTING_LEN     (1)

#define HEAD_TRACKING_VERSION_LEN (23)
#define HEAD_TRACKING_VERSION "#AndroidHeadTracker#1.0"

/* Feature report bits corresponding to reporting and power */
#define HEAD_TRACKING_REPORTING_ALL   (1<<0)
#define HEAD_TRACKING_REPORTING_NONE  (0<<0)
#define HEAD_TRACKING_POWER_FULL      (1<<1)
#define HEAD_TRACKING_POWER_OFF       (0<<1)

/* Flush time of 20ms (32 BT slots) */
#define SPATIAL_DATA_HIDD_INT_FLUSH_TIMEOUT (32)

static void spatialData_HiddTaskHandler(Task task, MessageId id, Message message);
static TaskData spatialDataHiddTask = {spatialData_HiddTaskHandler};

bool SpatialData_SetHiddSdp(uint16 data_len, const uint8 *data_ptr)
{
    DEBUG_LOG_DEBUG("%s",__func__);

    if (!data_ptr)
    {
        DEBUG_LOG_WARN("%s:Input data_ptr is NULL",__func__);
        return FALSE;
    }

    /* Register HIDD profile with the input descriptors */
    HiddProfile_RegisterDevice(&spatialDataHiddTask,
                               data_len,
                               data_ptr,
                               SPATIAL_DATA_HIDD_INT_FLUSH_TIMEOUT);

    return TRUE;
}

void SpatialData_SendSensorDataToRemoteDevice(const motion_sensor_data_t *sensor_data)
{
    DEBUG_LOG_VERBOSE("%s", __func__);

    if(!sensor_data)
    {
        DEBUG_LOG_ERROR("%s:Input sensor_data is NULL", __func__);
        Panic();
    }

    /* Check if HIDD is connected. */
    if (!HiddProfile_IsConnected())
    {
        DEBUG_LOG_WARN("%s:HIDD not connected for sending reports", __func__);

        /* Stop Sensor data as we have disconnected */
        if (spatial_data.report_id != spatial_data_report_disabled)
        {
            SpatialData_Enable(FALSE, SPATIAL_DATA_DEFAULT_SAMPLE_RATE_HZ, spatial_data.report_id);
            DEBUG_LOG_INFO("%s:Disabled Spatial Data", __func__);
        }

        return;
    }

    switch (spatial_data.report_id)
    {
        case spatial_data_report_1:
        {            
            att_head_orientation_rotation_vector_t rot_vector;            

            /* Get the latest Head Rotation Vector */
            SpatialData_AttitudeFilterHeadRotationVectorGet(&rot_vector);

            /* Only send valid rotation data that is non-zero */
            if (rot_vector.valid &&
               (((rot_vector.rads_q16[0] & 0xffff) != 0) ||
                ((rot_vector.rads_q16[1] & 0xffff) != 0) ||
                ((rot_vector.rads_q16[2] & 0xffff) != 0)))
            {
                uint8 *data = malloc(sizeof(uint8) * SPATIAL_DATA_HIDD_REPORT_1_LEN);
                int16 value;

                if (!data)
                {
                    DEBUG_LOG_WARN("%s:Memory creation failed for HIDD report 1", __func__);
                    return;
                }

                /* Copy Orientation data  */
                value = (int16)rot_vector.rads_q16[0];
                data[0] = (uint8)(value & 0xff);
                data[1] = (uint8)((value >> 8) & 0xff);

                value = (int16)rot_vector.rads_q16[1];
                data[2] = (uint8)(value & 0xff);
                data[3] = (uint8)((value >> 8) & 0xff);

                value = (int16)rot_vector.rads_q16[2];
                data[4] = (uint8)(value & 0xff);
                data[5] = (uint8)((value >> 8) & 0xff);

                /* Copy Angular Velocity data */
                value = (int16)rot_vector.rads_q16_per_s[0];
                data[6] = (uint8)(value & 0xff);
                data[7] = (uint8)((value >> 8) & 0xff);

                value = (int16)rot_vector.rads_q16_per_s[1];
                data[8] = (uint8)(value & 0xff);
                data[9] = (uint8)((value >> 8) & 0xff);

                value = (int16)rot_vector.rads_q16_per_s[2];
                data[10] = (uint8)(value & 0xff);
                data[11] = (uint8)((value >> 8) & 0xff);

                /* Copy Reference Frame Count data */
                data[12] = spatial_data.reset_count;

                /* Send the HID report */
                if (HiddProfile_DataReq((uint8)spatial_data.report_id, SPATIAL_DATA_HIDD_REPORT_1_LEN, data))
                {
                    DEBUG_LOG_INFO("Sent spatial_data_report_1:");
                    DEBUG_LOG_DATA_INFO(data, SPATIAL_DATA_HIDD_REPORT_1_LEN);
                }
                else
                {
                    DEBUG_LOG_INFO("%s: No data sent: v:%d x:%d y:%d z:%d", __func__,
                                   rot_vector.valid,
                                   rot_vector.rads_q16[0],
                                   rot_vector.rads_q16[1],
                                   rot_vector.rads_q16[2]);
                }

                free(data);
            }
            else
            {
                DEBUG_LOG_INFO("%s:rot_vector data not valid", __func__);
            }            
        }
        break;

        default:
        {
           /* Unsupported HIDD report type */
           DEBUG_LOG_WARN("%s:Unsupported HIDD report type:%d for forwarding data",__func__, spatial_data.report_id);
        }
        break;
    }
}


static void spatialData_HiddTaskHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case HIDD_PROFILE_DATA_CFM:
        {
            DEBUG_LOG_VERBOSE("%s:HIDD_PROFILE_DATA_CFM",__func__);
        }
        break;

        case HIDD_PROFILE_CONNECT_IND:
        {
            DEBUG_LOG_INFO("%s:HIDD_PROFILE_CONNECT_IND",__func__);

            HIDD_PROFILE_CONNECT_IND_T *conn_ind = (HIDD_PROFILE_CONNECT_IND_T*)message;
            PanicNull(conn_ind);

            /* Reset the count for the new HID connection */
            spatial_data.reset_count = 0;

            /* Reset the power and reporting status */
            spatial_data.power_enabled = FALSE;
            spatial_data.reporting_enabled = FALSE;

            /* Create a wall-clock time conversion handle for the L2CAP Sink for the HIDD connection. */
            if (conn_ind->status == hidd_profile_status_connected)
            {
                spatial_data.hidd_wallclock_enabled = 
                    RtimeWallClockEnable(&spatial_data.hidd_wallclock_state, StreamL2capSink(conn_ind->connid));
            }
        }
        break;

        case HIDD_PROFILE_DISCONNECT_IND:
        {
            DEBUG_LOG_INFO("%s:HIDD_PROFILE_DISCONNECT_IND",__func__);
        }
        break;

        case HIDD_PROFILE_GET_REPORT_IND:
        {
            DEBUG_LOG_INFO("%s:HIDD_PROFILE_GET_REPORT_IND",__func__);
            HIDD_PROFILE_GET_REPORT_IND_T *ind = (HIDD_PROFILE_GET_REPORT_IND_T*)message;

            PanicNull(ind);
            DEBUG_LOG_INFO("%s:Report Type:0x%x, Report ID:0x%x, Report Size:0x%x",__func__, ind->report_type, ind->reportid, ind->size);

            switch(ind->report_type)
            {
                case hidd_profile_report_type_feature:
                {
                    switch(ind->reportid)
                    {
                        case head_tracking_sensor_info:
                        {
                            static const uint8 uuid_const[] = {
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x42,0x54
                                 }; /* 0,0,0,0,0,0,0,0,'B','T' */
                            bdaddr primary = {0};

                            /* Send a control response data */
                            uint8 *data = PanicUnlessMalloc(HEAD_TRACKING_SENSOR_INFO_LEN);

                            memset(data, 0, HEAD_TRACKING_SENSOR_INFO_LEN);

                            /* Copy the head tracking version */
                            memcpy(&data[0], HEAD_TRACKING_VERSION, HEAD_TRACKING_VERSION_LEN);

                            /* Copy the UUID const section */
                            memcpy(&data[HEAD_TRACKING_VERSION_LEN], uuid_const, sizeof(uuid_const));

                            /* Copy the 6 byte BT MAC in Big Endian format [nap | uap | lap] */
#ifdef INCLUDE_STEREO
                            PanicFalse(appDeviceGetMyBdAddr(&primary));
#else
                            PanicFalse(appDeviceGetPrimaryBdAddr(&primary));
#endif /* INCLUDE_STEREO */
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+0] = (uint8)((primary.nap >> 8) & 0xff);
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+1] = (uint8)(primary.nap & 0xff);
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+2] = (uint8)(primary.uap & 0xff);
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+3] = (uint8)((primary.lap >> 16) & 0xff);
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+4] = (uint8)((primary.lap >>  8) & 0xff);
                            data[HEAD_TRACKING_VERSION_LEN + sizeof(uuid_const)+5] = (uint8)((primary.lap >>  0) & 0xff);

                            DEBUG_LOG_INFO("head_tracking_sensor_info:");
                            DEBUG_LOG_DATA_INFO(data, HEAD_TRACKING_SENSOR_INFO_LEN);

                            /* Send the report */
                            HiddProfile_DataRes(hidd_profile_report_type_feature, ind->reportid, HEAD_TRACKING_SENSOR_INFO_LEN, data);
                            free(data);
                        }
                        break;

                        case head_tracking_sensor_reporting:
                        {
                            /* Send a control response data */
                            uint8 *data = PanicUnlessMalloc(HEAD_TRACKING_SENSOR_REPORTING_LEN);
                            uint16 interval;

                            memset(data, 0, HEAD_TRACKING_SENSOR_REPORTING_LEN);

                            /* bit0 Reporting State: 0=None, 1=All */
                            /* bit1 Power State    : 0=Off,  1=Full */
                            /* bit2-7 Interval     : 0=10ms - 0x63=100ms */

                            if (spatial_data.reporting_enabled)
                            {
                                data[0] |= HEAD_TRACKING_REPORTING_ALL;
                            }

                            if (spatial_data.power_enabled)
                            {
                                data[0] |= HEAD_TRACKING_POWER_FULL;
                            }


                            /* If the rate hasn't been set... set the default */
                            if (!spatial_data.sensor_sample_rate_hz)
                            {
                                spatial_data.sensor_sample_rate_hz = SPATIAL_DATA_DEFAULT_SAMPLE_RATE_HZ;
                            }

                            /* Convert to 6-bit ms interval with range 10ms - 100ms */
                            #define INTERVAL_MIN   (10) /* ms */
                            #define INTERVAL_MAX  (100) /* ms */
                            #define INTERVAL_RANGE (63) /* 6-bits*/

                            interval = (((((1000 * INTERVAL_RANGE)/ spatial_data.sensor_sample_rate_hz) /* multiply by physical range and convert to ms */
                                        - INTERVAL_MIN)   /* scale down to min interval */                                        
                                        + (INTERVAL_MAX - INTERVAL_MIN - 1)) /* round up */
                                        / (INTERVAL_MAX - INTERVAL_MIN));    /* scale to interval range */

                            data[0] |= (interval & 0x3f) << 2;

                            DEBUG_LOG_INFO("head_tracking_sensor_reporting: 0x%02x", data[0]);

                            /* Send the report */
                            HiddProfile_DataRes(hidd_profile_report_type_feature, ind->reportid, HEAD_TRACKING_SENSOR_REPORTING_LEN, data);
                            free(data);

                        }
                        break;

                        default:
                        {
                            DEBUG_LOG_WARN("%s:Unsupported GET_REPORT Feature report ID:0x%x", __func__,ind->reportid);
                            HiddProfile_Handshake(hidd_profile_handshake_invalid_report_id);
                        }
                        break;
                    }
                    break;
                }
                default:
                {
                    DEBUG_LOG_WARN("%s:Unhandled HIDD_PROFILE_GET_REPORT_IND type:0x%x",__func__, ind->report_type);
                    HiddProfile_Handshake(hidd_profile_handshake_invalid_parameter);
                }
                break;

            }
            break;
        }
        case HIDD_PROFILE_ACTIVATE_CFM:
        {
            DEBUG_LOG_INFO("%s:HIDD_PROFILE_ACTIVATE_CFM",__func__);
        }
        break;

        case HIDD_PROFILE_DEACTIVATE_CFM:
        {
            DEBUG_LOG_INFO("%s:HIDD_PROFILE_DEACTIVATE_CFM",__func__);
        }
        break;

        case HIDD_PROFILE_SET_REPORT_IND:
        {
            DEBUG_LOG_INFO("%s;HIDD_PROFILE_SET_REPORT_IND",__func__);
            HIDD_PROFILE_SET_REPORT_IND_T *ind = (HIDD_PROFILE_SET_REPORT_IND_T*)message;

            PanicNull(ind);
            DEBUG_LOG_INFO("%s:Report Type:0x%x, Report ID:0x%x, Report Len:0x%x",__func__, ind->type, ind->reportid, ind->reportLen);
            DEBUG_LOG_DATA(ind->data, ind->reportLen);

            switch(ind->type)
            {
                case hidd_profile_report_type_feature:
                {
                    switch(ind->reportid)
                    {
                        case head_tracking_sensor_reporting:
                        {
                            spatial_data_enable_status_t enable = spatial_data_disabled;
                            uint8 data;

                            if(ind->reportLen >= 1)
                            {
                                data = ind->data[0];
                            }
                            else
                            {
                                HiddProfile_Handshake(hidd_profile_handshake_invalid_parameter);
                                break;
                            }

                            /* bit0 Reporting State: 0=None, 1=All */
                            /* bit1 Power State    : 0=Off,  1=Full */                            
                            spatial_data.power_enabled = (data & HEAD_TRACKING_POWER_FULL) ? TRUE : FALSE;
                            spatial_data.reporting_enabled = (data & HEAD_TRACKING_REPORTING_ALL) ? TRUE : FALSE;


                            /* Turn on reporting if both power and reporting have been enabled */
                            if ((spatial_data.power_enabled) &&
                                (spatial_data.reporting_enabled))
                            {
                                enable = spatial_data_enabled_remote;
                            }

                            /* bit2-7 Interval     : 0=10ms - 0x63=100ms */
                            /* Currenlty the interval value is not used. */

                            /* Enable/disable spatial audio. */
                            if (SpatialData_Enable(enable, SPATIAL_DATA_DEFAULT_SAMPLE_RATE_HZ, spatial_data_report_1))
                            {
                                DEBUG_LOG("%s: SpatialData_Enable returned success (%d) ",__func__,enable);
                            }
                            else
                            {
                                DEBUG_LOG("%s: SpatialData_Enable returned failure (%d) ",__func__,enable);
                            }

                            HiddProfile_Handshake(hidd_profile_handshake_success);
                        }
                        break;

                        default:
                        {
                            DEBUG_LOG_WARN("%s: Unsupported SET_REPORT Feature report ID:0x%x",__func__, ind->reportid);
                            HiddProfile_Handshake(hidd_profile_handshake_invalid_report_id);
                        }
                        break;
                    }
                }
                break;

                default: 
                {
                    DEBUG_LOG_WARN("%s:Unsupported SET_REPORT type:0x%x",__func__, ind->type);
                    HiddProfile_Handshake(hidd_profile_handshake_invalid_parameter);
                }
                break;
            }
            free(ind->data);
        }
        break;

        case HIDD_PROFILE_DATA_IND:
        {
            DEBUG_LOG_VERBOSE("%s:HIDD_PROFILE_DATA_IND",__func__);
            HIDD_PROFILE_DATA_IND_T *ind = (HIDD_PROFILE_DATA_IND_T*)message;

            PanicNull(ind);
            DEBUG_LOG_VERBOSE("%s:Report Type:0x%x, Report ID:0x%x, Report Len:0x%x",__func__, ind->type, ind->reportid,  ind->reportLen);
            DEBUG_LOG_DATA(ind->data, ind->reportLen);
            free(ind->data);
        }
        break;

        default:
        {
            DEBUG_LOG_WARN("%s:HIDD_PROFILE Unhandled message id:0x%x", __func__, id);
        }
        break;
    }
}

#endif /* INCLUDE_HIDD_PROFILE */

#endif /* INCLUDE_SPATIAL_DATA */
