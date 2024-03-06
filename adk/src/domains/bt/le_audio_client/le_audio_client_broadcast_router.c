/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of LE audio client broadcast router
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "le_audio_client_broadcast_router.h"
#include "le_audio_client_context.h"

/*! \brief Get LE audio client router context */
#define leaClientBcastRouter_GetContext() (&leAudioClient_GetContext()->router_data)

/*! \brief Get LE audio client router TMAP context */
#define leaClientBcastRouter_GetTmapContext() (&leaClientBcastRouter_GetContext()->tmap_data)

/*! \brief Get LE audio client router PBP context */
#define leaClientBcastRouter_GetPbpContext() (&leaClientBcastRouter_GetContext()->pbp_data)

/*! \brief Get LE audio client router mode */
#define leaClientBcastRouter_GetMode() (leaClientBcastRouter_GetContext()->mode)

/*! \brief Set LE audio client router mode */
#define leaClientBcastRouter_SetMode(mode) (leaClientBcastRouter_GetContext()->mode = mode)

/*! \brief Check if broadcast router is in TMAP mode */
#define leaClientBcastRouter_IsInTmapMode() (leaClientBcastRouter_GetMode() == \
                                                    LEA_CLIENT_BCAST_ROUTER_MODE_TMAP)

/*! \brief Suffix to use for broadcast source name if in PBP mode */
#define LEA_CLIENT_PBP_SOURCE_NAME_SUFFIX       "_PBP"
#define LEA_CLIENT_PBP_SOURCE_NAME_SUFFIX_LEN   (sizeof(LEA_CLIENT_PBP_SOURCE_NAME_SUFFIX))

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE to the task */
static void leAudioClientBroadcastRouter_SendInitComplete(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE to the task */
static void leAudioClientBroadcastRouter_SendSrcConfigComplete(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM to the task */
static void leAudioClientBroadcastRouter_SendSrcStreamStartCfm(uint8 num_bis,
                                                               const uint16 *bis_handles,
                                                               uint16 sampling_frequency,
                                                               uint8 frame_duration,
                                                               uint16 octets_per_frame,
                                                               uint32 transport_latency_big,
                                                               uint16 iso_interval,
                                                               bool succeeded)
{
    uint8 index;

    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    message->num_bis = num_bis;
    message->sampling_frequency = sampling_frequency;
    message->frame_duration = frame_duration;
    message->octets_per_frame = octets_per_frame;

    for(index = 0; index < message->num_bis; index++)
    {
       message->bis_handles[index] = bis_handles[index];
    }

    message->transport_latency_big = transport_latency_big;
    message->iso_interval = iso_interval;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM to the task */
static void leAudioClientBroadcastRouter_SendSrcStreamStopCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE to the task */
static void leAudioClientBroadcastRouter_SendSrcStreamRemoveCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM to the task */
static void leAudioClientBroadcastRouter_SendStartScanCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT to the task */
static void leAudioClientBroadcastRouter_SendScanReport(uint32 cid,
                                                        TYPED_BD_ADDR_T source_addr,
                                                        uint8 adv_sid,
                                                        uint8 adv_handle,
                                                        bool collocated,
                                                        uint32 broadcast_id,
                                                        uint8 num_subgroup,
                                                        BapBigSubgroup *subgroup_info)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT);

    message->mode = leaClientBcastRouter_GetMode();
    message->cid = cid;
    message->source_addr = source_addr;
    message->adv_sid = adv_sid;
    message->adv_handle = adv_handle;
    message->collocated = collocated;
    message->broadcast_id = broadcast_id;
    message->num_subgroup = num_subgroup;
    message->subgroup_info = subgroup_info;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM to the task */
static void leAudioClientBroadcastRouter_SendScanStopCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM to the task */
static void leAudioClientBroadcastRouter_SendRegisterNotificationCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM to the task */
static void leAudioClientBroadcastRouter_SendAddSourceCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM to the task */
static void leAudioClientBroadcastRouter_SendModifySourceCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM to the task */
static void leAudioClientBroadcastRouter_SendRemoveSourceCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND to the task */
static void leAudioClientBroadcastRouter_SendBrsInd(uint8 source_id,
                                                    typed_bdaddr source_address,
                                                    uint8 adv_sid,
                                                    uint8 pa_sync_state,
                                                    uint8 big_encryption,
                                                    uint32 broadcast_id,
                                                    bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    message->adv_sid = adv_sid;
    message->big_encryption = big_encryption;
    message->broadcast_id = broadcast_id;
    message->pa_sync_state = pa_sync_state;
    message->source_address = source_address;
    message->source_id = source_id;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND to the task */
static void leAudioClientBroadcastRouter_SendBrsReadInd(uint8 source_id,
                                                        typed_bdaddr source_address,
                                                        uint8 adv_sid,
                                                        uint8 pa_sync_state,
                                                        uint8 big_encryption,
                                                        uint32 broadcast_id,
                                                        bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;
    message->adv_sid = adv_sid;
    message->big_encryption = big_encryption;
    message->broadcast_id = broadcast_id;
    message->pa_sync_state = pa_sync_state;
    message->source_address = source_address;
    message->source_id = source_id;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND, message);
}

/*! \brief Send LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM to the task */
static void leAudioClientBroadcastRouter_SendBrsReadCfm(bool succeeded)
{
    MAKE_LEA_CLIENT_INTERNAL_MESSAGE(LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM);

    message->mode = leaClientBcastRouter_GetMode();
    message->status = succeeded ? LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS :
                                  LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_FAILED;

    MessageSend(leAudioClient_GetTask(), LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM, message);
}

/*! \brief Process TMAP Domain broadcast messages */
static void leAudioClientBroadcastRouter_ProcessTmapBroadcastMessage(const tmap_client_broadcast_msg_t *message)
{
    /* Process TMAP streaming related messages regardless of state */
    switch (message->id)
    {
        case TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_BROADCAST_INIT_COMPLETE_T *msg = &message->body.init_complete;

            if (msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS)
            {
                /* Store the received broadcast source handle */
                leaClientBcastRouter_GetTmapContext()->handle = msg->handle;

                /* Now intialise PBP */
                PbpClientSource_Init();
            }
            else
            {
                /* Send init failure message */
                leAudioClientBroadcastRouter_SendInitComplete(FALSE);
            }
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE:
        {
            const TMAP_CLIENT_MSG_ID_BROADCAST_CONFIG_COMPLETE_T *msg = &message->body.cfg_complete;

            leAudioClientBroadcastRouter_SendSrcConfigComplete(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM:
        {
            const TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_START_CFM_T *msg = &message->body.stream_start;

            leAudioClientBroadcastRouter_SendSrcStreamStartCfm(msg->num_bis,
                                                               &msg->bis_handles[0],
                                                               msg->sampling_frequency,
                                                               msg->frame_duration,
                                                               msg->octets_per_frame,
                                                               msg->transport_latency_big,
                                                               msg->iso_interval,
                                                               msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM:
        {
            const TMAP_CLIENT_MSG_ID_BROADCAST_STREAM_STOP_CFM_T *msg = &message->body.stream_stop;

            leAudioClientBroadcastRouter_SendSrcStreamStopCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_SRC_STREAM_REMOVE:
        {
            const TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM_T *msg = &message->body.src_stream_remove;

            leAudioClientBroadcastRouter_SendSrcStreamRemoveCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_START_SCAN_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_START_SCAN_CFM_T *msg = &message->body.scan_start;

            leAudioClientBroadcastRouter_SendStartScanCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_SCAN_REPORT:
        {
            const TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND_T *msg = &message->body.src_scan_report;

            leAudioClientBroadcastRouter_SendScanReport(msg->cid,
                                                        msg->source_addr,
                                                        msg->adv_sid,
                                                        msg->adv_handle,
                                                        msg->collocated,
                                                        msg->broadcast_id,
                                                        msg->num_subgroup,
                                                        msg->subgroup_info);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_STOP_SCAN_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_STOP_SCAN_CFM_T *msg = &message->body.scan_stop;

            leAudioClientBroadcastRouter_SendScanStopCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_REGISTER_GATT_NOTFN_CFM_T *msg = &message->body.register_gatt_notfn;

            leAudioClientBroadcastRouter_SendRegisterNotificationCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_ADD_SOURCE_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM_T *msg = &message->body.add_src;

            leAudioClientBroadcastRouter_SendAddSourceCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_MODIFY_SOURCE_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM_T *msg = &message->body.modify_src;

            leAudioClientBroadcastRouter_SendModifySourceCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_REMOVE_SOURCE_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM_T *msg = &message->body.remove_src;

            leAudioClientBroadcastRouter_SendRemoveSourceCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_IND:
        {
            const TMAP_CLIENT_BROADCAST_ASST_BRS_IND_T *msg = &message->body.brs_ind;

            leAudioClientBroadcastRouter_SendBrsInd(msg->source_id,
                                                    msg->source_address,
                                                    msg->adv_sid,
                                                    msg->pa_sync_state,
                                                    msg->big_encryption,
                                                    msg->broadcast_id,
                                                    msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_IND:
        {
            const TMAP_CLIENT_BROADCAST_ASST_BRS_READ_IND_T *msg = &message->body.brs_read_ind;

            leAudioClientBroadcastRouter_SendBrsReadInd(msg->source_id,
                                                        msg->source_address,
                                                        msg->adv_sid,
                                                        msg->pa_sync_state,
                                                        msg->big_encryption,
                                                        msg->broadcast_id,
                                                        msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        case TMAP_CLIENT_MSG_ID_BROADCAST_ASST_BRS_READ_CFM:
        {
            const TMAP_CLIENT_BROADCAST_ASST_BRS_READ_CFM_T *msg = &message->body.brs_read_cfm;

            leAudioClientBroadcastRouter_SendBrsReadCfm(msg->status == TMAP_CLIENT_BROADCAST_MSG_STATUS_SUCCESS);
        }
        break;

        default:
        break;
    }
}

/*! \brief Process PBP Domain messages */
static void leAudioClientBroadcastRouter_ProcessPbpMessage(const pbp_client_msg_t *message)
{
    /* Process TMAP streaming related messages regardless of state */
    switch (message->id)
    {
        case PBP_CLIENT_MSG_ID_INIT_COMPLETE:
        {
            const PBP_CLIENT_MSG_ID_INIT_COMPLETE_T *msg = &message->body.init_complete;

            if (msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS)
            {
                /* Store the received broadcast source handle */
                leaClientBcastRouter_GetPbpContext()->handle = msg->handle;
            }

            leAudioClientBroadcastRouter_SendInitComplete(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE:
        {
            const PBP_CLIENT_MSG_ID_SRC_CONFIG_COMPLETE_T *msg = &message->body.cfg_complete;

            leAudioClientBroadcastRouter_SendSrcConfigComplete(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM:
        {
            const PBP_CLIENT_MSG_ID_SRC_STREAM_START_CFM_T *msg = &message->body.stream_start;

            leAudioClientBroadcastRouter_SendSrcStreamStartCfm(msg->num_bis,
                                                               &msg->bis_handles[0],
                                                               msg->sampling_frequency,
                                                               msg->frame_duration,
                                                               msg->octets_per_frame,
                                                               msg->transport_latency_big,
                                                               msg->iso_interval,
                                                               msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM:
        {
            const PBP_CLIENT_MSG_ID_SRC_STREAM_STOP_CFM_T *msg = &message->body.stream_stop;

            leAudioClientBroadcastRouter_SendSrcStreamStopCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_SRC_STREAM_REMOVE:
        {
            const PBP_CLIENT_MSG_ID_SRC_REMOVE_STREAM_CFM_T *msg = &message->body.src_stream_remove;

            leAudioClientBroadcastRouter_SendSrcStreamRemoveCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_START_SCAN_CFM_T *msg = &message->body.scan_start;

            leAudioClientBroadcastRouter_SendStartScanCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_SCAN_REPORT:
        {
            const PBP_CLIENT_MSG_ID_ASST_SRC_REPORT_IND_T *msg = &message->body.src_scan_report;

            leAudioClientBroadcastRouter_SendScanReport(msg->cid,
                                                        msg->source_addr,
                                                        msg->adv_sid,
                                                        msg->adv_handle,
                                                        msg->collocated,
                                                        msg->broadcast_id,
                                                        msg->num_subgroup,
                                                        msg->subgroup_info);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_STOP_SCAN_CFM_T *msg = &message->body.scan_stop;

            leAudioClientBroadcastRouter_SendScanStopCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_REGISTER_NOTIFICATION_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_REGISTER_GATT_NOTFN_CFM_T *msg = &message->body.register_gatt_notfn;

            leAudioClientBroadcastRouter_SendRegisterNotificationCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_ADD_SOURCE_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_ADD_SRC_CFM_T *msg = &message->body.add_src;

            leAudioClientBroadcastRouter_SendAddSourceCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_MODIFY_SOURCE_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_MODIFY_SRC_CFM_T *msg = &message->body.modify_src;

            leAudioClientBroadcastRouter_SendModifySourceCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_REMOVE_SOURCE_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_REMOVE_SRC_CFM_T *msg = &message->body.remove_src;

            leAudioClientBroadcastRouter_SendRemoveSourceCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_BRS_IND:
        {
            const PBP_CLIENT_MSG_ID_ASST_BRS_IND_T *msg = &message->body.brs_ind;

            leAudioClientBroadcastRouter_SendBrsInd(msg->source_id,
                                                    msg->source_address,
                                                    msg->adv_sid,
                                                    msg->pa_sync_state,
                                                    msg->big_encryption,
                                                    msg->broadcast_id,
                                                    msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND:
        {
            const PBP_CLIENT_MSG_ID_ASST_BRS_READ_IND_T *msg = &message->body.brs_read_ind;

            leAudioClientBroadcastRouter_SendBrsReadInd(msg->source_id,
                                                        msg->source_address,
                                                        msg->adv_sid,
                                                        msg->pa_sync_state,
                                                        msg->big_encryption,
                                                        msg->broadcast_id,
                                                        msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM:
        {
            const PBP_CLIENT_MSG_ID_ASST_BRS_READ_CFM_T *msg = &message->body.brs_read_cfm;

            leAudioClientBroadcastRouter_SendBrsReadCfm(msg->status == PBP_CLIENT_MSG_STATUS_SUCCESS);
        }
        break;

        case PBP_CLIENT_MSG_ID_REGISTER_CAP_CFM:
        {
            const PBP_CLIENT_MSG_ID_CAP_REGISTER_CFM_T *msg = &message->body.cap_register_cfm;

            DEBUG_LOG("leAudioClientBroadcastRouter_ProcessPbpMessage register cap status %d",
                      msg->status);
        }
        break;

        default:
        break;
    }
}

void leAudioClientBroadcastRouter_Init(void)
{
    leaClientBcastRouter_GetTmapContext()->handle = LEA_CLIENT_INVALID_BCAST_SRC_HANDLE;

    TmapClientSourceBroadcast_RegisterCallback(leAudioClientBroadcastRouter_ProcessTmapBroadcastMessage);
    PbpClientSource_RegisterCallback(leAudioClientBroadcastRouter_ProcessPbpMessage);

    TmapClientSourceBroadcast_Init();
}

void leAudioClientBroadcastRouter_UpdateAdvSettings(const CapClientBcastSrcAdvParams *bcast_adv_settings)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSource_UpdateAdvSetting(bcast_adv_settings);
    }
    else
    {
        PbpClientSource_UpdateAdvSetting(bcast_adv_settings);
    }
}

void leAudioClientBroadcastRouter_SetBroadcastId(uint32 bcast_id)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_SetBcastId(leaClientBcastRouter_GetTmapContext()->handle, bcast_id);
    }
    else
    {
        PbpClientSourceBroadcast_SetBcastId(leaClientBcastRouter_GetPbpContext()->handle, bcast_id);
    }
}

void leAudioClientBroadcastRouter_SetMode(lea_client_bcast_router_mode_t mode)
{
    leaClientBcastRouter_SetMode(mode);
}

lea_client_bcast_router_mode_t leAudioClientBroadcastRouter_GetMode(void)
{
    return leaClientBcastRouter_GetMode();
}

void leAudioClientBroadcastRouter_Configure(uint32 presentation_delay,
                                            uint8 num_subgroup,
                                            const TmapClientBigSubGroup *subgroup_info,
                                            uint8 source_name_len,
                                            const char *source_name,
                                            uint8 broadcast_type,
                                            const CapClientBcastConfigParam *bcast_config_params)
{
    uint8 index;
    CapClientBigSubGroup pbp_subgroup_info;

    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_SetBcastConfigParams(leaClientBcastRouter_GetTmapContext()->handle,
                                                       bcast_config_params);
        TmapClientSourceBroadcast_Configure(leaClientBcastRouter_GetTmapContext()->handle,
                                            presentation_delay,
                                            num_subgroup,
                                            subgroup_info,
                                            source_name_len,
                                            source_name);
    }
    else
    {
        PanicFalse(broadcast_type == SQ_PUBLIC_BROADCAST || broadcast_type == HQ_PUBLIC_BROADCAST);

        /* Map to CapClientBigSubGroup */
        pbp_subgroup_info.config = subgroup_info->config;
        pbp_subgroup_info.numBis = subgroup_info->numBis;
        pbp_subgroup_info.targetLatency = subgroup_info->targetLatency;
        pbp_subgroup_info.lc3BlocksPerSdu = subgroup_info->lc3BlocksPerSdu;
        pbp_subgroup_info.useCase = subgroup_info->useCase;
        pbp_subgroup_info.metadataLen = subgroup_info->metadataLen;
        pbp_subgroup_info.metadata = NULL;

        for (index = 0; index < pbp_subgroup_info.numBis; index++)
        {
            pbp_subgroup_info.bisInfo[index].config = subgroup_info->bisInfo[index].config;
            pbp_subgroup_info.bisInfo[index].audioLocation = subgroup_info->bisInfo[index].audioLocation;
            pbp_subgroup_info.bisInfo[index].targetLatency = subgroup_info->bisInfo[index].targetLatency;
            pbp_subgroup_info.bisInfo[index].lc3BlocksPerSdu = subgroup_info->bisInfo[index].lc3BlocksPerSdu;
        }

        PbpClientSource_SetBcastConfigParams(leaClientBcastRouter_GetPbpContext()->handle, bcast_config_params);
        PbpClientSource_Configure(leaClientBcastRouter_GetPbpContext()->handle,
                                  presentation_delay,
                                  num_subgroup,
                                  &pbp_subgroup_info,
                                  source_name_len,
                                  source_name,
                                  broadcast_type,
                                  leAudioClient_GetBroadcastAudioConfig()->broadcast_code != NULL);
    }
}

void leAudioClientBroadcastRouter_StartStreaming(bool  encryption,
                                                 const uint8* broadcast_code)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_StartStreaming(leaClientBcastRouter_GetTmapContext()->handle,
                                                 encryption,
                                                 broadcast_code);
    }
    else
    {
        PbpClientSource_StartStreaming(leaClientBcastRouter_GetPbpContext()->handle,
                                       encryption,
                                       (uint8*)broadcast_code);
    }
}

void leAudioClientBroadcastRouter_StopStreaming(void)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_StopStreaming(leaClientBcastRouter_GetTmapContext()->handle);
    }
    else
    {
        PbpClientSource_StopStreaming(leaClientBcastRouter_GetPbpContext()->handle);
    }
}

void leAudioClientBroadcastRouter_RemoveStream(void)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_RemoveStream(leaClientBcastRouter_GetTmapContext()->handle);
    }
    else
    {
        PbpClientSource_RemoveStream(leaClientBcastRouter_GetPbpContext()->handle);
    }
}

void leAudioClientBroadcastRouter_UpdateMetadataInPeriodicTrain(uint8 metadata_len, uint8* metadata)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_UpdateStreamForSource(leaClientBcastRouter_GetTmapContext()->handle,
                                                        TMAP_CLIENT_CONTEXT_TYPE_MEDIA,
                                                        LE_AUDIO_CLIENT_BROADCAST_NUM_SUB_GROUPS,
                                                        metadata_len,
                                                        (const uint8*) metadata);
    }
    else
    {
        PbpClientSource_UpdateStreamForSource(leaClientBcastRouter_GetPbpContext()->handle,
                                              PBP_CONTEXT_TYPE_MEDIA,
                                              LE_AUDIO_CLIENT_BROADCAST_NUM_SUB_GROUPS,
                                              metadata_len,
                                              (uint8*) metadata);
    }
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
void leAudioClientBroadcastRouter_StartScanningForSource(ServiceHandle cap_group_id,
                                                         uint32 cid,
                                                         uint8 broadcast_type,
                                                         CapClientContext audio_context)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_StartScanningForSource(cap_group_id, cid, audio_context);
    }
    else
    {
        PanicFalse(broadcast_type == SQ_PUBLIC_BROADCAST || broadcast_type == HQ_PUBLIC_BROADCAST);

        PbpClientSource_StartScanningForSource(cap_group_id, cid, broadcast_type, audio_context);
    }
}

void leAudioClientBroadcastRouter_StopScanningForSource(ServiceHandle cap_group_id, uint32 cid)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_StopScanningForSource(cap_group_id, cid);
    }
    else
    {
        PbpClientSource_StopScanningForSource(cap_group_id, cid);
    }
}

void leAudioClientBroadcastRouter_AddSource(ServiceHandle cap_group_id,
                                            uint32 cid,
                                            typed_bdaddr source_taddr,
                                            uint8 adv_handle,
                                            uint8 adv_sid,
                                            uint32 broadcast_id,
                                            uint32 bis_index)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_AddSource(cap_group_id,
                                            cid,
                                            source_taddr,
                                            adv_handle,
                                            adv_sid,
                                            broadcast_id,
                                            bis_index);
    }
    else
    {
        PbpClientSource_AddSource(cap_group_id,
                                  cid,
                                  source_taddr,
                                  adv_handle,
                                  adv_sid,
                                  broadcast_id,
                                  bis_index);
    }
}

void leAudioClientBroadcastRouter_ModifySource(ServiceHandle cap_group_id,
                                               uint32 cid,
                                               uint8 adv_handle,
                                               uint8 adv_sid,
                                               uint32 bis_index,
                                               uint8 source_id,
                                               bool pa_sync_enable)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_ModifySource(cap_group_id,
                                               cid,
                                               adv_handle,
                                               adv_sid,
                                               bis_index,
                                               source_id,
                                               pa_sync_enable);
    }
    else
    {
        PbpClientSource_ModifySource(cap_group_id,
                                     cid,
                                     adv_handle,
                                     adv_sid,
                                     bis_index,
                                     source_id,
                                     pa_sync_enable);
    }
}

void leAudioClientBroadcastRouter_RemoveSource(ServiceHandle cap_group_id,
                                               uint32 cid,
                                               uint8 source_id)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_RemoveSource(cap_group_id, cid, source_id);
    }
    else
    {
        PbpClientSource_RemoveSource(cap_group_id, cid, source_id);
    }
}

void leAudioClientBroadcastRouter_RegisterForGattNotification(ServiceHandle cap_group_id,
                                                              uint32 cid,
                                                              uint8 source_id)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_RegisterForGattNotification(cap_group_id, cid, source_id);
    }
    else
    {
        PbpClientSource_RegisterForGattNotification(cap_group_id, cid, source_id);
    }
}

void leAudioClientBroadcastRouter_ReadReceiverSinkState(ServiceHandle cap_group_id, uint32 cid)
{
    if (leaClientBcastRouter_IsInTmapMode())
    {
        TmapClientSourceBroadcast_ReadReceiverSinkState(cap_group_id, cid);
    }
    else
    {
        PbpClientSource_ReadReceiverSinkState(cap_group_id, cid);
    }
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
