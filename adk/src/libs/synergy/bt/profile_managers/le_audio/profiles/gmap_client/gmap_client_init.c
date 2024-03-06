/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #3 $
******************************************************************************/

#include "gmap_client_lib.h"
#include "gmap_client_debug.h"
#include "gmap_client_init.h"
#include "gatt_service_discovery_lib.h"

GmapClientMainInst *gmapClientMain;

/******************************************************************************/
void gmapClientSendInitCfm(GMAP *gmapClientInst, GmapClientStatus status)
{
    GmapClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

    message->id = GMAP_CLIENT_INIT_CFM;
    message->status = status;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;
    message->gmasServices = gmapClientInst->supportedGmasSrvcs;

    GmapClientMessageSend(gmapClientInst->appTask, message);
}

/***************************************************************************/
void GmapClientInitReq(AppTask appTask,
                       GmapClientInitData *clientInitParams,
                       GmapClientHandles *deviceData)
{
    GMAP *gmapClientInst = NULL;
    GmapClientProfileHandle profileHndl = 0;
    GmapClientProfileHandleListElm *elem = NULL;
    GmapClientCidListElm *cidElm = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GMAP_CLIENT_PANIC("Application Task NULL\n");
    }

    elem = ADD_GMAP_CLIENT_SERVICE_HANDLE(gmapClientMain->profileHandleList);
    profileHndl = ADD_GMAP_CLIENT_INST(gmapClientInst);
    elem->profileHandle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(gmapClientInst, 0, sizeof(GMAP));

        /* Set up library handler for external messages */
        gmapClientInst->libTask = CSR_BT_GMAP_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gmapClientInst->appTask = appTask;

        CsrCmnListInit(&gmapClientInst->gmapClientCidList.cidList, 0, NULL, NULL);
        cidElm = ADD_GMAP_CLIENT_CID_ELEM(gmapClientInst->gmapClientCidList.cidList);
        cidElm->cid = clientInitParams->cid;

        gmapClientInst->gmapSrvcHndl = profileHndl;

        gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            GattGmasClientInitData initData;

            initData.cid = cidElm->cid;
            initData.startHandle = deviceData->gmasClientHandle[0].startHandle;
            initData.endHandle = deviceData->gmasClientHandle[0].endHandle;

            GattGmasClientInitReq(gmapClientInst->libTask,
                                  &initData,
                                  &(deviceData->gmasClientHandle[0]));
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_GMAS_SRVC | GATT_SD_QGMAS_SRVC;
            /* Find handle value range for the GMAS Server from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_GMAP_CLIENT_IFACEQUEUE, cidElm->cid, srvcIds);
        }
    }
    else
    {
        GMAP_CLIENT_PANIC("Memory allocation of GMAP Client Profile instance failed!\n");
    }
}

bool GmapClientAddNewDevice(GmapClientProfileHandle profileHandle,
                            uint32 cid)
{
    GMAP * gmapClientInst = NULL;
    GmapClientCidListElm *cidElm = NULL;

    gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    if (gmapClientInst)
    {
        cidElm = ADD_GMAP_CLIENT_CID_ELEM(gmapClientInst->gmapClientCidList.cidList);
        cidElm->cid = cid;

        return TRUE;
    }

    return FALSE;
}


GattGmasClientDeviceData * GmapClientGetDevicedata(GmapClientProfileHandle profileHandle)
{
    GMAP *gmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);
    GattGmasClientDeviceData *deviceData = NULL;

    if (gmapClientInst)
    {
        deviceData = GattGmasClientGetDeviceDataReq(gmapClientInst->gmasSrvcHndl);
    }
    else
    {
        GMAP_CLIENT_ERROR("Invalid profile handle\n");
    }
    return deviceData;
}


/****************************************************************************/
void gmapClientHandleGmasClientInitResp(GMAP *gmapClientInst,
                                        const GattGmasClientInitCfm * message)
{
    if(message->status == GATT_GMAS_CLIENT_STATUS_SUCCESS)
    {
        /* Additional parameter for simple handling in single instance case */
        gmapClientInst->gmasSrvcHndl = message->srvcHndl;

        gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_SUCCESS);
    }
    else
    {
        gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_FAILED);
        REMOVE_GMAP_CLIENT_SERVICE_HANDLE(gmapClientMain->profileHandleList, gmapClientInst->gmapSrvcHndl);
        FREE_GMAP_CLIENT_INST(gmapClientInst->gmapSrvcHndl);
    }
}

static void gmapClientInitProfileHandleList(CsrCmnListElm_t *elem)
{
    GmapClientProfileHandleListElm *cElem = (GmapClientProfileHandleListElm *) elem;

    cElem->profileHandle = 0;
}

void gmapClientInit(void **gash)
{
    gmapClientMain = CsrPmemZalloc(sizeof(*gmapClientMain));
    *gash = gmapClientMain;

    CsrCmnListInit(&gmapClientMain->profileHandleList, 0, gmapClientInitProfileHandleList, NULL);
    CsrBtGattRegisterReqSend(CSR_BT_GMAP_CLIENT_IFACEQUEUE,
                             0);
}

GmapClientMainInst *gmapClientGetMainInstance(void)
{
    return gmapClientMain;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************/
void gmapClientDeInit(void **gash)
{
    CsrCmnListDeinit(&gmapClientMain->profileHandleList);
    CsrPmemFree(gmapClientMain);
}
#endif
