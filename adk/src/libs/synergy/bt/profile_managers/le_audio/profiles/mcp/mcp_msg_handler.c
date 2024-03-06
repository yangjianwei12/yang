/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp_private.h"
#include "mcp_debug.h"
#include "mcp_init.h"
#include "mcp_destroy.h"
#include "mcp_indication.h"
#include "mcp_read.h"
#include "mcp_write.h"
#include "mcp_notification.h"
#include "mcp_common.h"
#include "gatt_service_discovery_lib.h"

CsrBool mcpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profile_handle = *(ServiceHandle *)data;

    if (profile_hndl_elm)
        return (profile_hndl_elm->profile_handle == profile_handle);

    return FALSE;
}

CsrBool mcpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    MCP *mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if (mcpInst)
        return (mcpInst->cid == btConnId);

    return FALSE;
}

CsrBool mcpProfileHndlFindByMcsSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle mcs_srvc_hndl = *(ServiceHandle *)data;
    MCP *mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if (mcpInst)
        return (mcpInst->mcsSrvcHndl == mcs_srvc_hndl);

    return FALSE;
}

/****************************************************************************/
static void handleGattSrvcDiscMsg(McpMainInst *inst, Msg *msg)
{
    MCP *mcpInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = MCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList, cfm->cid);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                uint16 index, count;

                count = cfm->srvcInfoCount;
                mcpInst->mcsCounter = cfm->srvcInfoCount;
                mcpInst->mcsNum = cfm->srvcInfoCount;

                for (index = 0; index < count; index++)
                {
                    GattMcsClientInitData init_data;
                    MCP_DEBUG("(MCP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                               cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                    init_data.cid = cfm->cid;
                    init_data.startHandle = cfm->srvcInfo[index].startHandle;
                    init_data.endHandle = cfm->srvcInfo[index].endHandle;

                    GattMcsClientInitReq(mcpInst->libTask, &init_data, NULL);
                }
                free(cfm->srvcInfo);
            }
            else
            {
                mcpSendInitCfm(mcpInst, MCP_STATUS_DISCOVERY_ERR);
                MCP_REMOVE_SERVICE_HANDLE(inst->profileHandleList, mcpInst->mcpSrvcHndl);
                FREE_MCP_CLIENT_INST(mcpInst->mcpSrvcHndl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
        }
        break;
    }
}

/*************************************************************/
static void mcpHandleGattMcsClientMsg(McpMainInst *inst, void *msg)
{
    MCP * mcpInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattMcsClientMessageId *prim = (GattMcsClientMessageId *)msg;

    MCP_INFO("mcpHandleGattMcsClientMsg MESSAGE:GattMcsClientMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_MCS_CLIENT_INIT_CFM:
        {
            const GattMcsClientInitCfm* message;
            message = (GattMcsClientInitCfm*) msg;
            /* Find mcp instance using connection_id_t */
            elem = MCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                       message->cid);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
                mcpHandleMcsClientInitResp(mcpInst,
                                           (const GattMcsClientInitCfm *)msg);
        }
        break;

        case GATT_MCS_CLIENT_TERMINATE_CFM:
        {
            const GattMcsClientTerminateCfm* message;
            message = (GattMcsClientTerminateCfm*) msg;
            /* Find mcp instance using connection_id_t */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                if (mcpInst->appTask != CSR_SCHED_QID_INVALID)
                {
                    mcpHandleMcsClientTerminateResp(mcpInst,
                                            (const GattMcsClientTerminateCfm *)msg);
                }
                else
                {
                    MCP_REMOVE_SERVICE_HANDLE(inst->profileHandleList, mcpInst->mcpSrvcHndl);
                    FREE_MCP_CLIENT_INST(mcpInst->mcpSrvcHndl);
                }
            }
        }
        break;

        case GATT_MCS_CLIENT_NTF_IND:
        {
            const GattMcsClientNtfInd* message;
            message = (GattMcsClientNtfInd*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleMcsNtfInd(mcpInst,
                                   (const GattMcsClientNtfInd *)msg);
            }
        }
        break;

        case GATT_MCS_CLIENT_NTF_CFM:
        {
            const GattMcsClientNtfCfm* message;
            message = (GattMcsClientNtfCfm*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleMcsNtfCfm(mcpInst,
                                   (const GattMcsClientNtfCfm *)msg);
            }
        }
        break;

        case GATT_MCS_CLIENT_GET_MEDIA_PLAYER_ATTRIBUTE_CFM:
        {
            const GattMcsClientGetMediaPlayerAttributeCfm* message;
            message = (GattMcsClientGetMediaPlayerAttributeCfm*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleReadCharacCfm(mcpInst, message);
            }

            if (message->value != NULL)
            {
                free(message->value);
            }
        }
        break;

        case GATT_MCS_CLIENT_SET_MEDIA_PLAYER_ATTRIBUTE_CFM:
        {
            const GattMcsClientSetMediaPlayerAttributeCfm* message;
            message = (GattMcsClientSetMediaPlayerAttributeCfm*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleWriteCharacCfm(mcpInst,
                                        (const GattMcsClientSetMediaPlayerAttributeCfm*)msg);
            }
        }
        break;

        case GATT_MCS_CLIENT_MEDIA_PLAYER_ATTRIBUTE_IND:
        {
            const GattMcsClientMediaPlayerAttributeInd* message;
            message = (GattMcsClientMediaPlayerAttributeInd*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleCharacIndCfm(mcpInst, message);
            }

            if (message->value != NULL)
            {
                free(message->value);
            }
        }
        break;

        case GATT_MCS_CLIENT_SET_MEDIA_CONTROL_POINT_CFM:
        {
            const GattMcsClientSetMediaControlPointCfm* message;
            message = (GattMcsClientSetMediaControlPointCfm*) msg;

            /* Find mcp instance using mcs service handle */
            elem = MCP_FIND_PROFILE_HANDLE_BY_MCS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
                mcpInst = FIND_MCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (mcpInst)
            {
                mcpHandleMediaControlPointCfm(mcpInst,
                                              (const GattMcsClientSetMediaControlPointCfm *)msg);
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT MCS Client message */
            MCP_WARNING("Gatt MCS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}


/***************************************************************************/
static void  mcpHandleInternalMessage(McpMainInst *inst, void *msg)
{
    MCP_DEBUG("mpHandleInternalMessage Message \n");
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}

/****************************************************************************/
void mcpMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    McpMainInst *inst = (McpMainInst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleGattSrvcDiscMsg(inst, msg);
                break;
            case MCP_PRIM:
                mcpHandleInternalMessage(inst, msg);
                break;
            case MCS_CLIENT_PRIM:
                mcpHandleGattMcsClientMsg(inst, msg);
                break;
            default:
                MCP_DEBUG("Profile Msg not handled \n");
                break;
        }
        SynergyMessageFree(eventClass, msg);
    }
}
