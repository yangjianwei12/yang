/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       handover_profile.c
    \ingroup    handover_profile
    \brief      Implementation of the Handover Profile public APIs.
*/

#ifdef INCLUDE_MIRRORING

#include "handover_profile_private.h"
#include "system_state.h"
#include "sdp.h"
#include "bt_device.h"

#include <message_broker.h>
#include <handover_if.h>

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(HANDOVER_PROFILE, HANDOVER_PROFILE_MESSAGE_END)

#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(handover_profile_messages_t)
LOGGING_PRESERVE_MESSAGE_TYPE(handover_profile_internal_msgs_t)

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static void handoverProfile_HandleMessage(Task task, MessageId id, Message message);

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/
/*! Check if the state is Disconnecting and message is NOT one of the 
    connection library L2CAP disconnect messages (CL_L2CAP_DISCONNECT_CFM or
    CL_L2CAP_DISCONNECT_IND. */
#ifdef USE_SYNERGY
#define handoverProfile_IsDisconnecting(id, message, ho_inst) \
    (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_DISCONNECTING \
     && (id) == CM_PRIM \
     && *(CsrBtCmPrim *) (message) != CSR_BT_CM_L2CA_DISCONNECT_IND \
     && *(CsrBtCmPrim *) (message) != CSR_BT_CM_L2CA_CONNECT_CFM \
     && *(CsrBtCmPrim *) (message) != CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM)
#else
#define handoverProfile_IsDisconnecting(id, message, ho_inst) \
    (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_DISCONNECTING \
     && (id) != CL_L2CAP_DISCONNECT_CFM \
     && (id) != CL_L2CAP_DISCONNECT_IND \
     && (id) != CL_L2CAP_CONNECT_CFM ? TRUE: FALSE)
#endif

/******************************************************************************
 * Global and Local Declarations
 ******************************************************************************/
/* Handover Profile task data. */
handover_profile_task_data_t ho_profile;

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
#ifdef USE_SYNERGY
static bool handoverProfile_HandleConnectionMessage(MessageId id, Message message)
{   
    if(id == CM_PRIM)
    {        
        CsrBtCmPrim *prim = (CsrBtCmPrim *) message;
        DEBUG_LOG("handoverProfile_HandleConnectionMessage Message MESSAGE:CsrBtCmPrim:0x%x", *prim);

        if (*prim == CSR_BT_CM_SDS_REGISTER_CFM)
        {
            HandoverProfile_HandleCmSdsRegisterCfm((const CsrBtCmSdsRegisterCfm *) message);
        }
        else if (CsrBtUtilSdcVerifyCmMsg(prim))
        {
            handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

            CsrBtUtilSdcCmMsgHandler(ho_inst,
                                     ho_inst->sdp_search_data,
                                     prim);
        }
        else
        {
            switch (*prim)
            {
                case CSR_BT_CM_L2CA_REGISTER_CFM:
                    HandoverProfile_HandleCmL2caRegisterCfm((const CsrBtCmL2caRegisterCfm *) message);
                    break;

                case CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND:
                    HandoverProfile_HandleCmL2caConnectAcceptInd((const CsrBtCmL2caConnectAcceptInd *) message);
                    break;

                case CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM:
                    HandoverProfile_HandleCmL2caConnectAcceptCfm((const CsrBtCmL2caConnectAcceptCfm *) message);
                    break;

                case CSR_BT_CM_L2CA_CONNECT_CFM:
                    HandoverProfile_HandleCmL2caConnectCfm((const CsrBtCmL2caConnectCfm *) message);
                    break;

                case CSR_BT_CM_L2CA_DISCONNECT_IND:
                    HandoverProfile_HandleCmL2caDisconnectInd((const CsrBtCmL2caDisconnectInd *) message);
                    break;

                case CSR_BT_CM_SET_EVENT_MASK_CFM:
                    break;

                default:
                    DEBUG_LOG("handoverProfile_HandleConnectionMessage Unhandled CM Prim 0x%04x",
                              *prim);
                    break;
            }
        }
        
        CmFreeUpstreamMessageContents((void * ) prim);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#else
static bool handoverProfile_HandleConnectionMessage(MessageId id, Message message)
{
    if(id >= CL_MESSAGE_BASE && id <= CL_MESSAGE_TOP)
    {
        DEBUG_LOG("handoverProfile_HandleConnectionMessage Message MESSAGE:ConnectionMessageId:0x%x", id);        
        switch (id)
        {
            /* Connection library messages */
            case CL_L2CAP_REGISTER_CFM:
                HandoverProfile_HandleClL2capRegisterCfm((const CL_L2CAP_REGISTER_CFM_T *)message);
                break;

            case CL_SDP_REGISTER_CFM:
                HandoverProfile_HandleClSdpRegisterCfm((const CL_SDP_REGISTER_CFM_T *)message);
                break;

            case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
                HandoverProfile_HandleClSdpServiceSearchAttributeCfm((const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
                break;

            case CL_L2CAP_CONNECT_IND:
                HandoverProfile_HandleL2capConnectInd((const CL_L2CAP_CONNECT_IND_T *)message);
                break;

            case CL_L2CAP_CONNECT_CFM:
                HandoverProfile_HandleL2capConnectCfm((const CL_L2CAP_CONNECT_CFM_T *)message);
                break;

            case CL_L2CAP_DISCONNECT_IND:
                HandoverProfile_HandleL2capDisconnectInd((const CL_L2CAP_DISCONNECT_IND_T *)message);
                break;

            case CL_L2CAP_DISCONNECT_CFM:
                HandoverProfile_HandleL2capDisconnectCfm((const CL_L2CAP_DISCONNECT_CFM_T *)message);
                break;
            default:
                break;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif

/*! 
    \brief Handover Profile task message handler

    Handles all the messages sent to the handover profile task.

    \param[in] task      Task data
    \param[in] id        Message ID \ref MessageId.
    \param[in] message   Message.

*/
static void handoverProfile_HandleMessage(Task task,
    MessageId id,
    Message message)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    UNUSED(task);

    /* If the state is Disconnecting, then reject all messages except
        CL_L2CAP_DISCONNECT_CFM.*/
    if(handoverProfile_IsDisconnecting(id, message, ho_inst))
    {
        DEBUG_LOG("handoverProfile_HandleMessage handoverProfile_IsDisconnecting dropping id MESSAGE:handover_profile_internal_msgs_t:0x%x",
                  id);
        return;
    }

    if(handoverProfile_HandleConnectionMessage(id, message))
    {
        return;
    }

    DEBUG_LOG("handoverProfile_HandleMessage Message MESSAGE:handover_profile_internal_msgs_t:0x%x", id);
    switch (id)
    {

        case MESSAGE_MORE_DATA:
        {
            const MessageMoreData *mmd = (const MessageMoreData *)message;
            handoverProtocol_HandleMessage(mmd->source);
            break;
        }

        /* Internal Handover Profile Messages */
        case HANDOVER_PROFILE_INTERNAL_STARTUP_REQ:
            HandoverProfile_HandleInternalStartupRequest((HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T *)message);
            break;

        case HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ:
            HandoverProfile_HandleInternalShutdownReq();
            break;
#ifdef USE_SYNERGY
        case HANDOVER_PROFILE_INTERNAL_CLOSE_SDP_REQ:
            HandoverProfile_HandleInternalCloseSdpReq();
            break;
#endif
        default:
            DEBUG_LOG("handoverProfile_HandleMessage Unhandled message 0x%x",id);
            break;
    }
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/

bool HandoverProfile_Init(Task init_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    memset(ho_inst, 0, sizeof(*ho_inst));

    UNUSED(init_task);

    ho_inst->task.handler = handoverProfile_HandleMessage;
    TaskList_Initialise(&ho_inst->handover_client_tasks);
    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_INITIALISING);
    return TRUE;
}

void HandoverProfile_ClientRegister(Task client_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    TaskList_AddTask(&ho_inst->handover_client_tasks, client_task);
}

void HandoverProfile_ClientUnregister(Task client_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    TaskList_RemoveTask(&ho_inst->handover_client_tasks, client_task);
}

void HandoverProfile_HandleSubsystemVersionInfo(const MessageSubsystemVersionInfo *info)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    if (info->ss_id == 2)
    {
        DEBUG_LOG("HandoverProfile_HandleSubsystemVersionInfo btss: 0x%x 0x%x", info->fw_rom_version, info->patched_fw_version);
        /* Store the BT firmware versions. The BT firmware versions on primary
        and secondary must match for handover to be allowed */
        ho_inst->btss_rom_version = info->fw_rom_version;
        ho_inst->btss_patch_version = info->patched_fw_version;
    }
}

void HandoverProfile_Connect(Task task, const bdaddr *peer_addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    PanicNull((void*)peer_addr);

    DEBUG_LOG("HandoverProfile_Connect - startup");

    ho_inst->is_primary = TRUE;

    /* Store peer device BD-Addr */
    memcpy(&ho_inst->peer_addr, peer_addr, sizeof(bdaddr));

    /* Send internal message to enter connecting state */
    HandoverProfile_Startup(task, peer_addr);
}

void HandoverProfile_Disconnect(Task task)
{
    DEBUG_LOG("HandoverProfile_Disconnect");
    HandoverProfile_Shutdown(task);
}

handover_profile_status_t HandoverProfile_Handover(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    handover_profile_status_t result = HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;

    if (ho_inst->state == HANDOVER_PROFILE_STATE_CONNECTED)
    {
        if (ho_inst->is_primary)
        {
            result = handoverProfile_HandoverAsPrimary();
        }
    }
    return result;
}


/* Provide an implementation of the handover_if.h utility API that
   allows handover clients to find out the type of handover in progress.
*/
handover_type_t handover_getType(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    return ho_inst->handover_type;
}

void HandoverProfile_RegisterAcceptorCb(const handover_profile_acceptor_cb_t *callback)
{
    Handover_GetTaskData()->handover_acceptor_cb = callback;
}

#endif /* INCLUDE_MIRRORING */
