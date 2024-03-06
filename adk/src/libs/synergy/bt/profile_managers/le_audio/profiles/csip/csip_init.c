/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_csis_client.h"

#include "csip.h"
#include "csip_debug.h"
#include "csip_init.h"
#include "gatt_service_discovery_lib.h"

CsipMainInst *csipMain;

/******************************************************************************/
void csipSendInitCfm(Csip * csipInst, CsipStatus status)
{
    CsipInitCfm *message = (CsipInitCfm *) CsrPmemZalloc(sizeof(CsipInitCfm));

    message->id = CSIP_INIT_CFM;
    message->status = status;


    if (csipInst == NULL)
    {
        CsrPmemFree(message);
    }
    else
    {
        message->prflHndl = csipInst->csipSrvcHdl;
        message->cid = csipInst->cid;
        CsipMessageSend(csipInst->app_task, message);
    }
}


/****************************************************************************/
static void csipCsisInitReq(Csip *csipInst,
                          CsipInitData *clientInitParams,
                          CsipHandles *deviceData)
{
    GattCsisClientInitParams csisInitData;
    GattCsisClientDeviceData csisDeviceData;

    csisInitData.cid = clientInitParams->cid;
    csisInitData.startHandle = deviceData->csisHandle.startHandle;
    csisInitData.endHandle = deviceData->csisHandle.endHandle;

    csisDeviceData.csisSirkHandle = deviceData->csisHandle.csisSirkHandle;
    csisDeviceData.csisLockCcdHandle = deviceData->csisHandle.csisLockCcdHandle;
    csisDeviceData.csisSizeCcdHandle = deviceData->csisHandle.csisSizeCcdHandle;
    csisDeviceData.csisSirkCcdHandle = deviceData->csisHandle.csisSirkCcdHandle;
    csisDeviceData.csisSizeHandle = deviceData->csisHandle.csisSizeHandle;
    csisDeviceData.csisLockHandle =  deviceData->csisHandle.csisLockHandle;
    csisDeviceData.csisRankHandle =  deviceData->csisHandle.csisRankHandle;

    GattCsisClientInit(csipInst->lib_task, &csisInitData, &csisDeviceData);
}

static void initCsipProfileHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ProfileHandleListElm_t *cElem = (ProfileHandleListElm_t *) elem;

    cElem->profileHandle = 0;
}

void CsipInit(void **gash)
{
    csipMain = CsrPmemZalloc(sizeof(*csipMain));
    *gash = csipMain;

    CsrCmnListInit(&csipMain->profileHandleList, 0, initCsipProfileHandleList, NULL);
    CsrBtGattRegisterReqSend(CSR_BT_CSIP_IFACEQUEUE,
                             0);
}

/***************************************************************************/
void CsipInitReq(AppTask appTask,
             CsipInitData *clientInitParams,
             CsipHandles *deviceData)
{
    Csip *csipInst = NULL;
    ProfileHandleListElm_t *elem = NULL;
    CsipProfileHandle profileHndl = 0;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        CSIP_PANIC("Application Task NULL\n");
    }

    elem = CSIP_ADD_SERVICE_HANDLE(csipMain->profileHandleList);
    profileHndl = ADD_CSIP_CLIENT_INST(csipInst);
    elem->profileHandle = profileHndl;

    if (csipInst)
    {
        /* Reset all the service library memory */
        memset(csipInst, 0, sizeof(Csip));

        /* Set up library handler for external messages */
        csipInst->lib_task = CSR_BT_CSIP_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        csipInst->app_task = appTask;

        csipInst->cid = clientInitParams->cid;

        csipInst->csipSrvcHdl = profileHndl;

        csipSendInitCfm(csipInst, CSIP_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            /* It's a peer device: we already know handles, no need to do discovery */
            csipInst->isPeerDevice = TRUE;

            csipInst->startHandle = deviceData->csisHandle.startHandle;
            csipInst->endHandle = deviceData->csisHandle.endHandle;

            /* We can start now the initialisation of all the necessary client:
             * we start with the CSIS Client.
             */
            csipCsisInitReq(csipInst, clientInitParams, deviceData);
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_CSIS_SRVC;
            /* Find handle value range for the CSIP from GATT SD */

            GattServiceDiscoveryFindServiceRange(CSR_BT_CSIP_IFACEQUEUE, csipInst->cid, srvcIds);
        }
    }
    else
    {
        CSIP_ERROR("Memory alllocation of CSIP Profile instance failed!\n");
        csipSendInitCfm(csipInst, CSIP_STATUS_FAILED);
    }
}


/****************************************************************************/
void csipHandleCsisClientInitResp(Csip *csipInst,
                                const GattCsisClientInitCfm * message)
{
    if(message->status == GATT_CSIS_CLIENT_STATUS_SUCCESS)
    {
        csipInst->csisSrvcHdl = message->srvcHndl;

        if (csipInst->secondaryServiceReq && !csipInst->isPeerDevice)
        {
            /* ToDo - Use gatt api to discover
            GattFindIncludedServicesRequest(&csipInst->lib_task,
                                            csipInst->cid,
                                            csipInst->start_handle,
                                            csipInst->end_handle);
                                            */
        }
        else if (!csipInst->secondaryServiceReq)
        {
            /* The applicantion didn't request the discovery of the secondary service:
             * we can send the confirmation of the initialisation
             */
            csipSendInitCfm(csipInst, CSIP_STATUS_SUCCESS);
        }
    }
    else
    {
        /* The initialisation of CSIS Client failed:
         * we need to destroy all the existed instance lists.*/
        csipSendInitCfm(csipInst, CSIP_STATUS_FAILED);
    }
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void CsipDeinit(void **gash)
{
    CsrCmnListDeinit(&csipMain->profileHandleList);
    CsrPmemFree(csipMain);
}
#endif
