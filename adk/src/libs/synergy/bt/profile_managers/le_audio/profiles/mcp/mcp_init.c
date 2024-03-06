/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp.h"
#include "mcp_debug.h"
#include "mcp_init.h"
#include "mcp_common.h"
#include "gatt_service_discovery_lib.h"

McpMainInst *mcpMain;

/******************************************************************************/
void mcpSendInitCfm(MCP * mcpInst, McpStatus status)
{
    McpInitCfm *message = CsrPmemAlloc(sizeof(*message));

    message->id = MCP_INIT_CFM;
    message->status = status;
    message->prflHndl = mcpInst->mcpSrvcHndl;;

    if(status == MCP_STATUS_SUCCESS)
    {
        uint8 mcsCount = 0;
        ServiceHandle *mcs_srvc_hndl = CsrPmemAlloc((sizeof(ServiceHandle))*(mcpInst->mcsNum));

        McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;
        ServiceHandle *temp = mcs_srvc_hndl;

        /* This code wil propagate all mcs client instance service handle for a particular device/mcp instance */
        while (ptr && mcsCount < mcpInst->mcsNum)
        {
            (*temp) = ptr->srvcHndl;
            temp++;
            ptr = ptr->next;
            mcsCount++;
        }

        message->mcsInstCount = mcpInst->mcsNum;
        message->mcsSrvcHandle = mcs_srvc_hndl;
    }
    else
    {
        message->mcsInstCount = 0;
        message->mcsSrvcHandle = NULL;
    }

    McpMessageSend(mcpInst->appTask, message);
}

/***************************************************************************/
void McpInitReq(AppTask appTask,
                McpInitData *clientInitParams,
                McpHandles *deviceData)
{
    MCP *mcpInst = NULL;
    McpProfileHandle profileHndl = 0;
    uint8 i;
    ProfileHandleListElm_t *elem = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        MCP_PANIC("Application Task NULL\n");
    }

    /* Check for profile handle list for the given cid and if entry is already exisiting then simply return  with failure */
    elem = MCP_FIND_PROFILE_HANDLE_BY_BTCONNID(mcpMain->profileHandleList, clientInitParams->cid);

    if (elem)
    {
        McpInitCfm* message = CsrPmemAlloc(sizeof(*message));

        message->id = MCP_INIT_CFM;
        message->status = MCP_STATUS_FAILED;
        message->prflHndl = 0;
        message->mcsInstCount = 0;
        message->mcsSrvcHandle = NULL;

        McpMessageSend(appTask, message);
        return;
    }

    elem = MCP_ADD_SERVICE_HANDLE(mcpMain->profileHandleList);
    profileHndl = ADD_MCP_CLIENT_INST(mcpInst);
    elem->profile_handle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(mcpInst, 0, sizeof(MCP));

        /* Set up library handler for external messages */
        mcpInst->libTask = CSR_BT_MCP_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        mcpInst->appTask = appTask;

        mcpInst->cid = clientInitParams->cid;

        mcpInst->mcpSrvcHndl = profileHndl;

        mcpSendInitCfm(mcpInst, MCP_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            if (deviceData->mcsInstCount)
            {
                mcpInst->mcsNum = deviceData->mcsInstCount;

                mcpInst->mcsCounter = deviceData->mcsInstCount;

                for (i = 0; i < deviceData->mcsInstCount; i++)
                {
                    GattMcsClientInitData init_data;

                    init_data.cid = mcpInst->cid;
                    init_data.startHandle = deviceData->mcsHandle[i].startHandle;
                    init_data.endHandle = deviceData->mcsHandle[i].endHandle;

                    GattMcsClientInitReq(mcpInst->libTask,
                                         &init_data,
                                         &(deviceData->mcsHandle[i]));
                }
            }
            else
            {
                /* No instance of primary service in the remote device */
                mcpSendInitCfm(mcpInst, MCP_STATUS_INVALID_PARAMETER);
            }
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_GMCS_SRVC;
            /* Find handle value range for the MCP from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_MCP_IFACEQUEUE, mcpInst->cid, srvcIds);
        }
    }
    else
    {
        McpInitCfm *message = CsrPmemAlloc(sizeof(*message));

        message->id = MCP_INIT_CFM;
        message->status = MCP_STATUS_FAILED;
        message->prflHndl = 0;
        McpMessageSend(appTask, message);
    }
}


/****************************************************************************/
void mcpHandleMcsClientInitResp(MCP *mcpInst,
                                const GattMcsClientInitCfm * message)
{
    if(message->status == GATT_MCS_CLIENT_STATUS_SUCCESS)
    {
        /* The initialisation of a MCS client instance is done */
        mcpInst->mcsCounter -= 1;

        /* Additional parameter for simple handling in single instance case */
        mcpInst->mcsSrvcHndl = message->srvcHndl;

        /* We have to save the service handle of the client instance in a list, because when
         * we finish all the MCP client initialisations, we have to send this list to the application.
         * The application need to know there are more instance in order to handle them in all the
         * MCP procedure it wants to perform.
         */
        mcpAddElementMcsSrvcHndlList(mcpInst, message->srvcHndl);

        if (!mcpInst->mcsCounter)
        {
            mcpSendInitCfm(mcpInst, MCP_STATUS_SUCCESS);
        }

    }
    else
    {
        mcpSendInitCfm(mcpInst, MCP_STATUS_FAILED);
        GattMcsClientTerminateReq(message->srvcHndl);
        mcpInst->appTask = CSR_SCHED_QID_INVALID;
    }
}

static void initProfileHandleList(CsrCmnListElm_t *elem)
{
    ProfileHandleListElm_t *cElem = (ProfileHandleListElm_t *) elem;

    cElem->profile_handle = 0;
}

void mcpInit(void **gash)
{
    mcpMain = CsrPmemAlloc(sizeof(*mcpMain));
    *gash = mcpMain;

    CsrCmnListInit(&mcpMain->profileHandleList, 0, initProfileHandleList, NULL);
}

McpMainInst *mcpGetMainInstance(void)
{
    return mcpMain;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************/
void mcpDeInit(void **gash)
{
    CsrCmnListDeinit(&mcpMain->profileHandleList);
    CsrPmemFree(mcpMain);
}
#endif
