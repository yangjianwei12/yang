/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/

#include "tmap_client_lib.h"
#include "tmap_client_debug.h"
#include "tmap_client_init.h"
#include "gatt_service_discovery_lib.h"

TmapClientMainInst *tmapClientMain;

/******************************************************************************/
void tmapClientSendInitCfm(TMAP *tmapClientInst, TmapClientStatus status)
{
    TmapClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

    message->id = TMAP_CLIENT_INIT_CFM;
    message->status = status;
    message->prflHndl = tmapClientInst->tmapSrvcHndl;
    message->tmasSrvcHandle = tmapClientInst->tmasSrvcHndl;

    TmapClientMessageSend(tmapClientInst->appTask, message);
}

/***************************************************************************/
void TmapClientInitReq(AppTask appTask,
                       TmapClientInitData *clientInitParams,
                       TmapClientHandles *deviceData)
{
    TMAP *tmapClientInst = NULL;
    TmapClientProfileHandle profileHndl = 0;
    TmapClientProfileHandleListElm *elem = NULL;
    TmapClientCidListElm *cidElm = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        TMAP_CLIENT_PANIC("Application Task NULL\n");
    }

    elem = ADD_TMAP_CLIENT_SERVICE_HANDLE(tmapClientMain->profileHandleList);
    profileHndl = ADD_TMAP_CLIENT_INST(tmapClientInst);
    elem->profileHandle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(tmapClientInst, 0, sizeof(TMAP));

        /* Set up library handler for external messages */
        tmapClientInst->libTask = CSR_BT_TMAP_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        tmapClientInst->appTask = appTask;

        CsrCmnListInit(&tmapClientInst->tmapClientCidList.cidList, 0, NULL, NULL);
        cidElm = ADD_TMAP_CLIENT_CID_ELEM(tmapClientInst->tmapClientCidList.cidList);
        cidElm->cid = clientInitParams->cid;

        tmapClientInst->tmapSrvcHndl = profileHndl;

        tmapClientSendInitCfm(tmapClientInst, TMAP_CLIENT_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            GattTmasClientInitData initData;

            initData.cid = cidElm->cid;
            initData.startHandle = deviceData->tmasClientHandle[0].startHandle;
            initData.endHandle = deviceData->tmasClientHandle[0].endHandle;

            GattTmasClientInitReq(tmapClientInst->libTask,
                                  &initData,
                                  &(deviceData->tmasClientHandle[0]));
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_TMAS_SRVC;
            /* Find handle value range for the TMAP from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_TMAP_CLIENT_IFACEQUEUE, cidElm->cid, srvcIds);
        }
    }
    else
    {
        TMAP_CLIENT_PANIC("Memory allocation of TMAP Client Profile instance failed!\n");
    }
}

bool TmapClientAddNewDevice(TmapClientProfileHandle profileHandle,
                            uint32 cid)
{
    TMAP * tmapClientInst = NULL;
    TmapClientCidListElm *cidElm = NULL;

    tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    if (tmapClientInst)
    {
        cidElm = ADD_TMAP_CLIENT_CID_ELEM(tmapClientInst->tmapClientCidList.cidList);
        cidElm->cid = cid;

        return TRUE;
    }

    return FALSE;
}


GattTmasClientDeviceData * TmapClientGetDevicedata(TmapClientProfileHandle profileHandle)
{
    TMAP *tmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);
    GattTmasClientDeviceData *deviceData = NULL;

    if (tmapClientInst)
    {
        deviceData = GattTmasClientGetDeviceDataReq(tmapClientInst->tmasSrvcHndl);
    }
    else
    {
        TMAP_CLIENT_ERROR("Invalid profile handle\n");
    }
    return deviceData;
}

/****************************************************************************/
void tmapClientHandleTmasClientInitResp(TMAP *tmapClientInst,
                                        const GattTmasClientInitCfm * message)
{
    if(message->status == GATT_TMAS_CLIENT_STATUS_SUCCESS)
    {
        /* Additional parameter for simple handling in single instance case */
        tmapClientInst->tmasSrvcHndl = message->srvcHndl;

        tmapClientSendInitCfm(tmapClientInst, TMAP_CLIENT_STATUS_SUCCESS);
    }
    else
    {
        tmapClientSendInitCfm(tmapClientInst, TMAP_CLIENT_STATUS_FAILED);
        REMOVE_TMAP_CLIENT_SERVICE_HANDLE(tmapClientMain->profileHandleList, tmapClientInst->tmapSrvcHndl);
        FREE_TMAP_CLIENT_INST(tmapClientInst->tmapSrvcHndl);
    }
}

static void tmapClientInitProfileHandleList(CsrCmnListElm_t *elem)
{
    TmapClientProfileHandleListElm *cElem = (TmapClientProfileHandleListElm *) elem;

    cElem->profileHandle = 0;
}

void tmapClientInit(void **gash)
{
    tmapClientMain = CsrPmemZalloc(sizeof(*tmapClientMain));
    *gash = tmapClientMain;

    CsrCmnListInit(&tmapClientMain->profileHandleList, 0, tmapClientInitProfileHandleList, NULL);
    CsrBtGattRegisterReqSend(CSR_BT_TMAP_CLIENT_IFACEQUEUE,
                             0);
}

TmapClientMainInst *tmapClientGetMainInstance(void)
{
    return tmapClientMain;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************/
void tmapClientDeInit(void **gash)
{
    CsrCmnListDeinit(&tmapClientMain->profileHandleList);
    CsrPmemFree(tmapClientMain);
}
#endif
