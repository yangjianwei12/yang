/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       dfu_protocol_message_handler.c
    \ingroup    dfu_protocol_message_handler
    \brief      Implemetation of the message handler APIs for the dfu_protocol module
*/

#include "dfu_protocol_log.h"

#include "dfu_protocol_message_handler.h"

#include "dfu_protocol_client_config.h"
#include "dfu_protocol_client_notifier.h"
#include "dfu_protocol_data.h"
#include "dfu_protocol_message_dispatcher.h"
#include "dfu_protocol_sm.h"
#include "dfu_protocol_sm_types.h"
#include "dfu_protocol_upgrade_config.h"

#include "dfu_peer.h"

#include <bt_device.h>
#include <byte_utils.h>
#include <panic.h>
#include <upgrade_protocol.h>

static uint8 host_sync_cfm_resume_point;

static void dfuProtocol_MessageHandler(Task task, MessageId id, Message message);
static TaskData dfu_protocol_message_handler_task_data = { .handler = dfuProtocol_MessageHandler };

static void dfuProtocol_InternalMessageHandler(Task task, MessageId id, Message message);
static TaskData dfu_protocol_internal_message_handler_task_data = { .handler = dfuProtocol_InternalMessageHandler };


static uint16 dfuProtocol_Get4BytesIn16BitLittleEndian(uint8 * src, uint16 byte_index, uint32 * dst)
{
    *dst =  ((uint32) src[byte_index] << 24);
    *dst |= ((uint32) src[byte_index + 1] << 16);
    *dst |= ((uint32) src[byte_index + 2] << 8);
    *dst |= (uint32) src[byte_index + 3];
    return 4;
}

static void dfuProtocol_HandleUpgradeHostSyncCfm(uint16 size_data, uint8 * data)
{
    uint16 size_to_read = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE + UPGRADE_MSG_RESUME_POINT_SIZE;
    PanicFalse(size_data >= size_to_read);
    
    ByteUtilsGet1ByteFromStream(&data[0]);
    ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE]);
    
    /* The resume point is used when handling UPGRADE_HOST_START_CFM to determine whether to respond
     * with UPGRADE_HOST_START_DATA_REQ, UPGRADE_HOST_IS_CSR_VALID_DONE_REQ,
     * UPGRADE_HOST_TRANSFER_COMPLETE_RES, or UPGRADE_HOST_PROCEED_TO_COMMIT */
    host_sync_cfm_resume_point = ByteUtilsGet1ByteFromStream(&data[UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE]);

    dfu_protocol_events_t sync_event = dfu_protocol_host_sync_complete_event;

    if(DfuProtocol_DidActiveClientCauseReboot())
    {
        sync_event = dfu_protocol_host_sync_complete_post_reboot_event;
    }

    DEBUG_LOG_DEBUG("dfuProtocol_HandleUpgradeHostSyncCfm enum:UpdateResumePoint:%d", (UpdateResumePoint)host_sync_cfm_resume_point);

    StateMachine_Update(DfuProtocol_GetStateMachine(), sync_event, NULL);
}

static void dfuProtocol_HandleUpgradeHostStartCfm(void)
{
    dfu_protocol_events_t start_event = dfu_protocol_begin_transfer_event;

    if(host_sync_cfm_resume_point == UPGRADE_RESUME_POINT_PRE_REBOOT)
    {
        start_event = dfu_protocol_transfer_complete_event;
    }
    else if(host_sync_cfm_resume_point == UPGRADE_RESUME_POINT_PRE_VALIDATE)
    {
        start_event = dfu_protocol_validate_event;
    }
    else if(DfuProtocol_DidActiveClientCauseReboot())
    {
        start_event = dfu_protocol_commit_request_event;
    }
    else
    {
        /* We are not waiting for validation, reboot, or a post-reboot commit.
         * Default to requested for data transfer to start */
    }

    StateMachine_Update(DfuProtocol_GetStateMachine(), start_event, NULL);
}

static void dfuProtocol_HandleUpgradeHostIsCsrValidDoneCfm(uint16 size_data, uint8 * data)
{
    uint16 size_to_read = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE + UPGRADE_MSG_BACK_OFF_TIME_SIZE;
    PanicFalse(size_data >= size_to_read);

    ByteUtilsGet1ByteFromStream(&data[0]);
    ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE]);

    uint16 back_off_time = ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE]);

    MessageSendLater(DfuProtocol_GetInternalMessageHandlerTask(), DFU_PROTOCOL_RETRY_VALIDATION_REQUEST_IND, NULL, back_off_time);
}

static void dfuProtocol_HandleUpgradeHostErrorwarnInd(uint16 size_data, uint8 * data)
{
    uint16 error_code;
    uint16 size_to_read = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE + UPGRADE_MSG_ERROR_CODE_SIZE;
    PanicFalse(size_data >= size_to_read);
    
    ByteUtilsGet1ByteFromStream(&data[0]);
    ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE]);
    
    error_code = ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE]);

    DEBUG_LOG_VERBOSE("dfuProtocol_HandleUpgradeHostErrorwarnInd enum:UpgradeHostErrorCode:%d", (UpgradeHostErrorCode)error_code);

    MessageCancelAll(DfuProtocol_GetInternalMessageHandlerTask(), DFU_PROTOCOL_RETRY_VALIDATION_REQUEST_IND);
    DfuProtocol_SendErrorIndToActiveClient((UpgradeHostErrorCode)error_code);
    DfuProtocol_ErrorWarnResponse(error_code);
}

static void dfuProtocol_HandleUpgradeHostDataBytesReq(uint16 size_data, uint8 * data)
{
    uint16 size_to_read = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE + UPGRADE_MSG_NUMBER_OF_BYTES_SIZE + UPGRADE_MSG_START_OFFSET_SIZE;
    PanicFalse(size_data >= size_to_read);

    /* We do not care about the ID or length */
    uint16 byte_index = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE;
    uint32 number_of_bytes;
    uint32 start_offset;

    byte_index += dfuProtocol_Get4BytesIn16BitLittleEndian(data, byte_index, &number_of_bytes);
    byte_index += dfuProtocol_Get4BytesIn16BitLittleEndian(data, byte_index, &start_offset);

    DEBUG_LOG("dfuProtocol_HandleUpgradeHostDataBytesReq number_of_bytes 0x%X start_offset 0x%X", number_of_bytes, start_offset);

    dfu_protocol_data_request_params_t data_request_params =
    {
        .number_of_bytes = number_of_bytes,
        .start_offset = start_offset
    };

    StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_data_request_event, (void *)&data_request_params);
}

static void dfuProtocol_HandleUpgradeHostSilentCommitSupportedCfm(uint16 size_data, uint8 *data)
{
    uint8 is_silent_commit_supported;
    uint16 size_to_read = UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE + UPGRADE_MSG_SILENT_COMMIT_SUPPORTED_SIZE;
    PanicFalse(size_data >= size_to_read);

    UNUSED(size_data);

    ByteUtilsGet1ByteFromStream(&data[0]);
    ByteUtilsGet2BytesFromStream(&data[UPGRADE_MSG_ID_SIZE]);

    is_silent_commit_supported = ByteUtilsGet1ByteFromStream(&data[UPGRADE_MSG_ID_SIZE + UPGRADE_MSG_LENGTH_SIZE]);
    
    DfuProtocol_SetUpgradeSupportsSilentCommit(is_silent_commit_supported);
    DEBUG_LOG_VERBOSE("dfuProtocol_HandleUpgradeHostSilentCommitSupportedCfm is_silent_commit_supported %d", is_silent_commit_supported);
}

static void dfuProtocol_HandleUpgradeHostTransferCompleteInd(void)
{
    DfuProtocol_SendTransferCompleteIndToActiveClient();
    StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_transfer_complete_event, NULL);
}

static void dfuProtocol_HandleUpgradeTransportDataInd(UPGRADE_TRANSPORT_DATA_IND_T *msg)
{
    DEBUG_LOG_DEBUG("dfuProtocol_HandleUpgradeTransportDataInd: enum:UpgradeMsgHost:%d", msg->data[0] + UPGRADE_HOST_MSG_BASE);
    switch(msg->data[0] + UPGRADE_HOST_MSG_BASE)
    {
        case UPGRADE_HOST_SYNC_CFM:
            dfuProtocol_HandleUpgradeHostSyncCfm(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_START_CFM:
            dfuProtocol_HandleUpgradeHostStartCfm();
            break;

        case UPGRADE_HOST_COMMIT_REQ:
            DfuProtocol_SendCommitReqToActiveClient();
            break;

        case UPGRADE_HOST_ERRORWARN_IND:
            dfuProtocol_HandleUpgradeHostErrorwarnInd(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_DATA_BYTES_REQ:
            dfuProtocol_HandleUpgradeHostDataBytesReq(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_IS_CSR_VALID_DONE_CFM:
            dfuProtocol_HandleUpgradeHostIsCsrValidDoneCfm(msg->size_data, msg->data);
            break;

        case UPGRADE_HOST_TRANSFER_COMPLETE_IND:
            dfuProtocol_HandleUpgradeHostTransferCompleteInd();
            break;

        case UPGRADE_HOST_ABORT_CFM:
            StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_complete_event, NULL);
            break;

        case UPGRADE_HOST_SILENT_COMMIT_CFM:
            StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_silent_commit_event, NULL);
            break;

        case UPGRADE_HOST_SILENT_COMMIT_SUPPORTED_CFM:
            dfuProtocol_HandleUpgradeHostSilentCommitSupportedCfm(msg->size_data, msg->data);
            break;

        default:
            DEBUG_LOG_ERROR("dfuProtocol_HandleUpgradeTransportDataInd unexpected host msg");
            /* Ignore the unexpected message. */
            break;
    }
}

static void dfuProtocol_HandleUpgradeTransportConnectCfm(UPGRADE_TRANSPORT_CONNECT_CFM_T *msg)
{
    UNUSED(msg);
    dfu_protocol_events_t event = DfuProtocol_DidActiveClientCauseReboot() ? dfu_protocol_transport_connected_post_reboot_event : dfu_protocol_transport_connected_event;
    StateMachine_Update(DfuProtocol_GetStateMachine(), event, NULL);
}

static void dfuProtocol_HandleUpgradeTransportDisconnectCfm(UPGRADE_TRANSPORT_DISCONNECT_CFM_T *msg)
{
    DEBUG_LOG_VERBOSE("dfuProtocol_HandleUpgradeTransportDisconnectCfm: msg->status %d", msg->status);
    dfu_protocol_transport_disconnected_params_t params = { .is_upgrade_successful = msg->status };
    MessageCancelAll(DfuProtocol_GetInternalMessageHandlerTask(), DFU_PROTOCOL_RETRY_VALIDATION_REQUEST_IND);
    StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_transport_disconnected_event, (void *)&params);
}

static void dfuProtocol_HandleUpgradeTransportDataCfm(UPGRADE_TRANSPORT_DATA_CFM_T *msg)
{
    if(msg && msg->data)
    {
        uint16 msg_id = ByteUtilsGet1ByteFromStream(msg->data) + UPGRADE_HOST_MSG_BASE;

        if(msg_id == UPGRADE_HOST_DATA)
        {
            DfuProtocol_ResetDataPayload();
        }
        else
        {
            free((void *)msg->data);
        }
    }

    if ((DfuProtocol_GetRemainingNumberOfBytesRequested() > 0) && (DfuProtocol_GetLengthOfAvailableDataInBytes() > 0))
    {
        DfuProtocol_SendUpdateHostData();
    }
}

static void dfuProtocol_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        /* Handle an external message from the upgrade library. */
        case UPGRADE_TRANSPORT_DATA_IND:
            dfuProtocol_HandleUpgradeTransportDataInd((UPGRADE_TRANSPORT_DATA_IND_T *) message);
            break;

        case UPGRADE_TRANSPORT_CONNECT_CFM:
            DEBUG_LOG_DEBUG("dfuProtocol_MessageHandler UPGRADE_TRANSPORT_CONNECT_CFM");
            dfuProtocol_HandleUpgradeTransportConnectCfm((UPGRADE_TRANSPORT_CONNECT_CFM_T *) message);
            break;

        case UPGRADE_TRANSPORT_DISCONNECT_CFM:
            DEBUG_LOG_DEBUG("dfuProtocol_MessageHandler UPGRADE_TRANSPORT_DISCONNECT_CFM");
            dfuProtocol_HandleUpgradeTransportDisconnectCfm((UPGRADE_TRANSPORT_DISCONNECT_CFM_T *) message);
            break;

        case UPGRADE_TRANSPORT_DATA_CFM:
            DEBUG_LOG_DEBUG("dfuProtocol_MessageHandler UPGRADE_TRANSPORT_DATA_CFM");
            dfuProtocol_HandleUpgradeTransportDataCfm((UPGRADE_TRANSPORT_DATA_CFM_T *) message);
            break;

#ifdef INCLUDE_DFU_PEER
        /* Handle an external message from the upgrade peer. */
        case DFU_PEER_INIT_CFM:
            /* Ignore */
            break;

        case DFU_PEER_STARTED:
            DEBUG_LOG_DEBUG("dfuProtocol_MessageHandler DFU_PEER_STARTED");
            
            /* If we were in the process of aborting this will prompt us to proceed, it is ignored if an abort was not previously triggered */
            MessageCancelAll(DfuProtocol_GetInternalMessageHandlerTask(), DFU_PROTOCOL_ABORT_WAITING_FOR_PEER_TIMEOUT);
            StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_abort_continue_event, NULL);

            if(DfuProtocol_DidActiveClientCauseReboot() && BtDevice_IsMyAddressPrimary())
            {
                dfu_protocol_start_params_t start_params = { 0, Upgrade_GetContext() };
                StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_start_post_reboot_event, &start_params);
            }
            break;
#endif /* INCLUDE_DFU_PEER */

        default:
            DEBUG_LOG_VERBOSE("dfuProtocol_MessageHandler unexpected msg 0x%04x", id);
            /* Ignore the unexpected message. */
            break;
    }
}

static void dfuProtocol_InternalMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case DFU_PROTOCOL_ABORT_WAITING_FOR_PEER_TIMEOUT:
        {
            StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_abort_continue_event, NULL);
        }
        break;

        case DFU_PROTOCOL_RETRY_VALIDATION_REQUEST_IND:
        {
            StateMachine_Update(DfuProtocol_GetStateMachine(), dfu_protocol_validate_event, NULL);
        }
        break;
    }
}

void DfuProtocol_MessageHandlerInit(void)
{
#ifdef INCLUDE_DFU_PEER
    DfuPeer_ClientRegister((Task)&dfu_protocol_message_handler_task_data);
#endif
}

Task DfuProtocol_GetMessageHandlerTask(void)
{
    return (Task)&dfu_protocol_message_handler_task_data;
}

Task DfuProtocol_GetInternalMessageHandlerTask(void)
{
    return (Task)&dfu_protocol_internal_message_handler_task_data;
}
