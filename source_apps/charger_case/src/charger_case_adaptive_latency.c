/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Adaptive latency component of the charger case
*/

#include <av.h>
#include <unexpected_message.h>
#include <kymera_adaptation_audio_protected.h>
#include <device_list.h>
#include <device_properties.h>

#include "charger_case_adaptive_latency.h"
#include "charger_case_a2dp_source.h"

/*! This timer is active during streaming to check the current link status and update the latency */
#define chargerCaseConfigAdaptiveLatencyCheckMs()  100

/*! Mininum interval in seconds between latency adjustment messages */
#define MIN_ADJUST_INTERVAL D_SEC(2)
#define DEFAULT_LATENCY 0
#define DEFAULT_RSSI -40
#define DEFAULT_LINK_QUALITY 65535
#define LINK_QUALITY_THRESHOLD 65000
#define FILTER_PREV 0.99
#define FILTER_NEW (1.0 - FILTER_PREV)
#define LATENCY_ADJUST_UPPER_LIMIT (MIN_ADJUST_INTERVAL/chargerCaseConfigAdaptiveLatencyCheckMs())
#define LATENCY_ADJUST_LOWER_LIMIT (-LATENCY_ADJUST_UPPER_LIMIT)
#define RSSI_LOWER_LIMIT -80
#define RSSI_UPPER_LIMIT -40
#define RSSI_LEVEL1 -50
#define RSSI_LEVEL2 -60
#define RSSI_LEVEL3 -70
#define RSSI_LATENCY_STEP 20
#define LINK_QUALITY_LATENCY_STEP 10

typedef struct
{
   float filtered_rssi;
   uint16_t filtered_link_quality;
   int16_t update_counter;
} AdaptiveLatency_t;

typedef struct
{
    TaskData task;
    AdaptiveLatency_t adaptive_latency;
} charger_case_adaptive_latency_data_t;

charger_case_adaptive_latency_data_t adaptive_latency_data;

static void chargerCaseAdaptiveLatency_Start (void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseAdaptiveLatency_Start");

    appKymeraSetTargetLatency(DEFAULT_LATENCY);
    adaptive_latency_data.adaptive_latency.filtered_rssi = (float)DEFAULT_RSSI;
    adaptive_latency_data.adaptive_latency.filtered_link_quality = DEFAULT_LINK_QUALITY;
    adaptive_latency_data.adaptive_latency.update_counter = 0;

    MessageCancelAll(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY);
    MessageSendLater(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY, NULL, chargerCaseConfigAdaptiveLatencyCheckMs());
}

static void chargerCaseAdaptiveLatency_End (void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseAdaptiveLatency_End");

    MessageCancelAll(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY);
}

/*! \brief Filters input RSSI value over time and provides an latency update based on the filtered RSSI.

    \param rssi - Current link RSSI value
    \param target_latency - Target latency based on RSSI value

    \returns void
 */
static void chargerCaseAdaptiveLatency_UpdateRssi(int16_t rssi, uint16_t* target_latency)
{
    /* The RSSI values are supplied every chargerCaseConfigAdaptiveLatencyCheckMs() but we
     * don't want to adjust the latency too quickly, so we will filter the channel metrics
     * to avoid instability.
     */
    float filtered_rssi = adaptive_latency_data.adaptive_latency.filtered_rssi;

    filtered_rssi = (FILTER_PREV * filtered_rssi) + (FILTER_NEW * (float)rssi);
    adaptive_latency_data.adaptive_latency.filtered_rssi = filtered_rssi;
    /* Keep the filtered value within reasonable bounds */
    if (filtered_rssi > RSSI_UPPER_LIMIT)
    {
        filtered_rssi = RSSI_UPPER_LIMIT;
    }
    else if (filtered_rssi < RSSI_LOWER_LIMIT)
    {
        filtered_rssi = RSSI_LOWER_LIMIT;
    }

    /* Adjust the target latency based on the filtered RSSI */
    if (filtered_rssi > RSSI_LEVEL1)
    {
        *target_latency = DEFAULT_LATENCY;
    }
    else if (filtered_rssi > RSSI_LEVEL2 && filtered_rssi <= RSSI_LEVEL1)
    {
        *target_latency = DEFAULT_LATENCY + (1 * RSSI_LATENCY_STEP);
    }
    else if (filtered_rssi > RSSI_LEVEL3 && filtered_rssi <= RSSI_LEVEL2)
    {
        *target_latency = DEFAULT_LATENCY + (2 * RSSI_LATENCY_STEP);
    }
    else /* RSSI <= RSSI_LEVEL3 */
    {
        *target_latency = DEFAULT_LATENCY + (3 * RSSI_LATENCY_STEP);
    }

    DEBUG_LOG_V_VERBOSE("chargerCaseAdaptiveLatency_UpdateRssi target: %d", *target_latency);
}

/*! \brief Filters input link quality value over time and provides an latency update based on the filtered link quality.

    \param link_quality - Current link quality value
    \param target_latency - Target latency based on link quality value

    \returns void
 */
static void chargerCaseAdaptiveLatency_UpdateQuality(uint16_t link_quality, uint16_t* target_latency)
{
    /* The Link Quality values are supplied every chargerCaseConfigAdaptiveLatencyCheckMs() but
     * we don't want to adjust the latency too quickly, so we will filter the channel metrics
     * to avoid instability.
     * If the filtered link quality fall belows a defined threshold we want to add additional latency to
     * the target value.
     */

    uint16_t filtered_link_quality = adaptive_latency_data.adaptive_latency.filtered_link_quality;

    filtered_link_quality = (FILTER_PREV * filtered_link_quality) + (FILTER_NEW * link_quality);
    adaptive_latency_data.adaptive_latency.filtered_link_quality = filtered_link_quality;

    if (filtered_link_quality < LINK_QUALITY_THRESHOLD)
    {
        *target_latency = *target_latency + LINK_QUALITY_LATENCY_STEP;
        DEBUG_LOG_V_VERBOSE("chargerCaseAdaptiveLatency_UpdateQuality target: %d", *target_latency);
    }
}

/*! \brief Reads RSSI and link quality and adjusts required target latency based on these values.

    \returns void
 */
static void chargerCaseAdaptiveLatency_HandleAdaptiveLatency (void)
{
    uint16_t link_quality;
    int16_t rssi;
    tp_bdaddr tp_addr;
    uint16_t target_latency = DEFAULT_LATENCY;
    uint16_t current_latency = appKymeraGetCurrentLatency();

    uint8 type_sink = DEVICE_TYPE_SINK;
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type_sink, sizeof(uint8));

    if(chargerCaseConfigAdaptiveLatencyCheckMs())
    {
        if (BtDevice_GetTpBdaddrForDevice(device,&tp_addr))
        {
            if (VmBdAddrGetRssi(&tp_addr, &rssi))
            {
                DEBUG_LOG_V_VERBOSE("chargerCaseAdaptiveLatency_HandleAdaptiveLatency - Link RSSI: %d", rssi);
                chargerCaseAdaptiveLatency_UpdateRssi(rssi, &target_latency);
            }
            if (VmGetAclLinkQuality(&tp_addr, &link_quality))
            {
                DEBUG_LOG_V_VERBOSE("chargerCaseAdaptiveLatency_HandleAdaptiveLatency - Link Quality: %d",link_quality);
                chargerCaseAdaptiveLatency_UpdateQuality(link_quality, &target_latency);
            }
            if (target_latency != current_latency)
            {
                /* Target latency is different from the current latency, so we need to adjust
                 * the current latency, but we don't want to move the latency more than 1ms
                 * per 2s so we need to count the number of time slices until we adjust it */
                int16_t update_counter = adaptive_latency_data.adaptive_latency.update_counter;

                if (target_latency > current_latency)
                {
                    if (update_counter < 0)
                    {
                        /* Change in direction of update so zero counter */
                        update_counter = 0;
                    }
                    /* Count up for latency increase */
                    update_counter++;
                }
                else /* (target_latency < current_latency) */
                {
                    if (update_counter > 0)
                    {
                        /* Change in direction of update so zero counter */
                        update_counter = 0;
                    }
                    /* Count down for latency decrease */
                    update_counter--;
                }
                if (update_counter == LATENCY_ADJUST_UPPER_LIMIT)
                {
                    /* The time slices count equals the update interval time in the positive direction
                     * so we increment the latency target and call the method to update the latency */
                    current_latency++;
                    adaptive_latency_data.adaptive_latency.update_counter = 0;
                    appKymeraSetTargetLatency(current_latency);
                    DEBUG_LOG_DEBUG("chargerCaseAdaptiveLatency_HandleAdaptiveLatency Add Latency: %d", current_latency);
                }
                else if (update_counter == LATENCY_ADJUST_LOWER_LIMIT)
                {
                    /* The time slices count equals the update interval time in the negative direction
                     * so we decrement the latency target and call the method to update the latency */
                     current_latency--;
                    adaptive_latency_data.adaptive_latency.update_counter = 0;
                    appKymeraSetTargetLatency(current_latency);
                    DEBUG_LOG_DEBUG("chargerCaseAdaptiveLatency_HandleAdaptiveLatency Add Latency: %d", current_latency);
                }
                else
                {
                    /* The time slices count hasn't reached the update interval time yet, update count variable */
                     adaptive_latency_data.adaptive_latency.update_counter = update_counter;
                }
            }
            else
            {
                /* The target based on link metrics is the same as the current latency so
                 * reset the counter */
                adaptive_latency_data.adaptive_latency.update_counter = 0;
            }
            /* Kick off timer to call this function again after an interval */
            MessageCancelAll(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY);
            MessageSendLater(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY, NULL, chargerCaseConfigAdaptiveLatencyCheckMs());
        }
        else
        {
            /* Device ID is not valid, cancel latency adaption */
            MessageCancelAll(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY);
        }
    }
    else
    {
        /* Latency adjust period not defined, canel latency adaption */
        MessageCancelAll(&adaptive_latency_data.task, CHARGER_CASE_ADAPTIVE_LATENCY);
    }
}



static void chargerCaseAdaptiveLatency_HandleAvA2dpAudioConnected(AV_A2DP_AUDIO_CONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseAdaptiveLatency_HandleAvA2dpAudioConnected: audioSource %d", msg->audio_source);
    source_defined_params_t a2dp_audio_source_paramaters;
    AudioSources_GetConnectParameters(msg->audio_source, &a2dp_audio_source_paramaters);
    a2dp_connect_parameters_t * a2dp_connect_params = a2dp_audio_source_paramaters.data;

    /* If we're streaming with aptX adaptive then start the adaptive latency adjustment */
    if (a2dp_connect_params->seid == AV_SEID_APTX_ADAPTIVE_SRC)
    {
        chargerCaseAdaptiveLatency_Start();
    }
}

static void chargerCaseAdaptiveLatency_HandleAvA2dpAudioDisconnected(AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseAdaptiveLatency_HandleAvA2dpAudioDisconnected: audioSource %d", msg->audio_source);

    /* Stop the adaptive latency adjustment */
    chargerCaseAdaptiveLatency_End();
}

static void chargerCase_AdaptiveTaskHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    case AV_A2DP_AUDIO_CONNECTED:
        chargerCaseAdaptiveLatency_HandleAvA2dpAudioConnected((AV_A2DP_AUDIO_CONNECT_MESSAGE_T *) message);
    break;
    case AV_A2DP_AUDIO_DISCONNECTED:
        chargerCaseAdaptiveLatency_HandleAvA2dpAudioDisconnected((AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *) message);
    break;
    case CHARGER_CASE_ADAPTIVE_LATENCY:
        chargerCaseAdaptiveLatency_HandleAdaptiveLatency();
    break;
    default:
        UnexpectedMessage_HandleMessage(id);
    break;
    }

}

bool ChargerCase_AdaptiveLatencyInit(Task init_task)
{
    UNUSED(init_task);
    adaptive_latency_data.task.handler = chargerCase_AdaptiveTaskHandler;
    appAvStatusClientRegister(&adaptive_latency_data.task);

    return TRUE;
}


