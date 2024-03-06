/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#include "gatt_mics_client.h"
#include "gatt_aics_client.h"

#include "micp_private.h"
#include "micp_debug.h"
#include "micp_init.h"
#include "micp_destroy.h"
#include "micp_indication.h"
#include "micp_read.h"
#include "micp_write.h"
#include "micp_notification.h"
#include "micp_common.h"
#include "gatt_service_discovery_lib.h"

CsrBool micpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profile_handle = *(ServiceHandle *)data;

    return (profile_hndl_elm->profile_handle == profile_handle);
}

CsrBool micpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    MICP *micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(micp_inst)
    {
        return (micp_inst->cid == btConnId);
    }

    return FALSE;
}

CsrBool micpProfileHndlFindByMicsSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle mics_srvc_hndl = *(ServiceHandle *)data;
    MICP *micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(micp_inst)
        return (micp_inst->mics_srvc_hdl == mics_srvc_hndl);

    return FALSE;
}

/****************************************************************************/
static void handleGattSrvcDiscMsg(micp_main_inst *inst, void *msg)
{
    MICP *micp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = MICP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list, cfm->cid);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (micp_inst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                GattMicsClientInitData init_data;
                MICP_DEBUG("(MICP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                    cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                init_data.cid = cfm->cid;
                init_data.startHandle = cfm->srvcInfo[0].startHandle;
                init_data.endHandle = cfm->srvcInfo[0].endHandle;

                micp_inst->start_handle = cfm->srvcInfo[0].startHandle;
                micp_inst->end_handle = cfm->srvcInfo[0].endHandle;

                GattMicsClientInitReq(micp_inst->lib_task, &init_data, NULL);
                CsrPmemFree(cfm->srvcInfo);
            }
            else
            {
                micpSendInitCfm(micp_inst, MICP_STATUS_DISCOVERY_ERR);
                MICP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, micp_inst->micp_srvc_hdl);
                FREE_MICP_CLIENT_INST(micp_inst->micp_srvc_hdl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            MICP_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void micpHandleGattMicsClientMsg(micp_main_inst *inst, void *msg)
{
    MICP * micp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattMicsClientMessageId *prim = (GattMicsClientMessageId *)msg;

    MICP_INFO("micpHandleGattMicsClientMsg: MESSAGE:GattMicsClientMessageId [0x%x]", *prim);

    switch (*prim)
    {
        case GATT_MICS_CLIENT_INIT_CFM:
        {
            const GattMicsClientInitCfm* message;
            message = (GattMicsClientInitCfm*) msg;
            /* Find micp instance using connection_id_t */
            elem = MICP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list,
                                                       message->cid);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (micp_inst)
                micpHandleMicsClientInitResp(micp_inst,
                                       (const GattMicsClientInitCfm *)msg);
        }
        break;

        case GATT_MICS_CLIENT_TERMINATE_CFM:
        {
            const GattMicsClientTerminateCfm* message;
            message = (GattMicsClientTerminateCfm*) msg;
            /* Find micp instance using connection_id_t */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->srvcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsClientTerminateResp(micp_inst,
                                                (const GattMicsClientTerminateCfm *)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }

            MICP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, elem->profile_handle);
        }
        break;

        case GATT_MICS_CLIENT_NTF_CFM:
        {
            const GattMicsClientNtfCfm* message;
            message = (GattMicsClientNtfCfm*) msg;

            /* Find micp instance using mics service handle */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsNtfCfm(micp_inst, (const GattMicsClientNtfCfm *)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }
        }
        break;

        case GATT_MICS_CLIENT_READ_MUTE_VALUE_CCC_CFM:
        {
            const GattMicsClientReadMuteValueCccCfm* message;
            message = (GattMicsClientReadMuteValueCccCfm*) msg;

            /* Find micp instance using mics service handle */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsReadMuteValueCccCfm(micp_inst,
                                                  (const GattMicsClientReadMuteValueCccCfm *)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }
        }
        break;

        case GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM:
        {
            const GattMicsClientReadMuteValueCfm* message;
            message = (GattMicsClientReadMuteValueCfm*) msg;

            /* Find micp instance using mics service handle */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->svcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsReadMuteValueCfm(micp_inst,
                                               (const GattMicsClientReadMuteValueCfm*)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }
        }
        break;

        case GATT_MICS_CLIENT_SET_MUTE_VALUE_CFM:
        {
            const GattMicsClientSetMuteValueCfm* message;
            message = (GattMicsClientSetMuteValueCfm*) msg;

            /* Find micp instance using mics service handle */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsSetMuteValueOp(micp_inst,
                                                (const GattMicsClientSetMuteValueCfm *)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }
        }
        break;

        case GATT_MICS_CLIENT_MUTE_VALUE_IND:
        {
            const GattMicsClientMuteValueInd* message;
            message = (GattMicsClientMuteValueInd*) msg;

            /* Find micp instance using mics service handle */
            elem = MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(inst->profile_handle_list,
                                                       message->srvcHndl);
            if (elem)
                micp_inst = FIND_MICP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if(micp_inst)
            {
                micpHandleMicsMuteValueInd(micp_inst,
                                           (const GattMicsClientMuteValueInd*)msg);
            }
            else
            {
                MICP_PANIC("Invalid MICP Profile instance\n");
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT MICS Client message */
            MICP_WARNING("Gatt MICS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

/****************************************************************************/
static void micpHandleGattAicsClientMsg(micp_main_inst *inst, void *msg)
{
    MICP * micp_inst = NULL;
    GattAicsClientMessageId *prim = (GattAicsClientMessageId *)msg;

    if(micp_inst)
    {
        switch (*prim)
        {
            case GATT_AICS_CLIENT_INIT_CFM:
            {
                micpHandleAicsClientInitResp(micp_inst,
                                            (const GattAicsClientInitCfm *) msg);
            }
            break;

            case GATT_AICS_CLIENT_TERMINATE_CFM:
            {
                micpHandleAicsClientTerminateResp(micp_inst,
                                                 (const GattAicsClientTerminateCfm *)msg);
            }
            break;

            default:
            {
                /* Unrecognised GATT AICS Client message */
                MICP_WARNING("Gatt AICS Client Msg not handled [0x%x]\n", *prim);
            }
            break;
        }
    }
    else
    {
        MICP_PANIC("Invalid MICP Profile instance\n");
    }

    CSR_UNUSED(inst);
}

/***************************************************************************/
static void  micpHandleInternalMessage(micp_main_inst *inst, void *msg)
{
    MICP * micp_inst = NULL;
    MicpInternalMsg *prim = (MicpInternalMsg *)msg;

    MICP_INFO("micpHandleInternalMessage:Message id:micp_internal_msg_t (%d)\n", *prim);

    if (inst)
    {
        switch(*prim)
        {
            case MICP_INTERNAL_SET_MUTE_VALUE:
            {
                MICP_INTERNAL_SET_MUTE_VALUE_T *message = (MICP_INTERNAL_SET_MUTE_VALUE_T *)msg;

                micp_inst = FIND_MICP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);

                if(micp_inst)
                {
                    micp_inst->pending_op = MICP_PENDING_SET_MUTE_VALUE_OP;

                    micpMicsControlPointOp(message->prfl_hndl,
                                         MICP_SET_MUTE_VALUE_OP,
                                         message->mute_value);
                }
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                MICP_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}

/****************************************************************************/
void micpMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    micp_main_inst *inst = (micp_main_inst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleGattSrvcDiscMsg(inst, msg);
                break;
            case MICP_PRIM:
                micpHandleInternalMessage(inst, msg);
                break;
            case MICS_CLIENT_PRIM:
                micpHandleGattMicsClientMsg(inst, msg);
                break;
            case AICS_CLIENT_PRIM:
                micpHandleGattAicsClientMsg(inst, msg);
                break;
            default:
                MICP_WARNING("Profile Msg not handled \n");
        }

        SynergyMessageFree(eventClass, msg);
    }
}

