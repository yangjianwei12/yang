/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_pmem.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"
#include "gatt_pacs_client_util.h"
#include "gatt_pacs_client_debug.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_pacs_client_msg_handler.h"

/* Global pacs service instance */
extern PacsC *mainInst;

static void  pacsGattDiscoverRequest(GPacsC *const pacsClient)
{
    /* Start finding out the characteristics handles for pacs service */

    CsrBtGattDiscoverAllCharacOfAServiceReqSend(pacsClient->srvcElem->gattId,
                                                pacsClient->srvcElem->cid,
                                                pacsClient->startHandle,
                                                pacsClient->endHandle);
}


static void pacsAddPacRecordHandles(CsrBtPacRecordCharacElement* elem, uint16 valueHandle, uint16 ccdHandle, uint8 recordId)
{
    elem->recordId = recordId;
    elem->valueHandle = valueHandle;
    elem->pacRecordCccdHandle = ccdHandle;
}

/******************************************************************************/

void GattPacsClientInitReq(AppTask appTask,
                           const GattPacsClientInitData *initData,
                           const GattPacsClientDeviceData *deviceData)
{
    ServiceHandle clientHandle = 0;
    uint8 index;
    GattClientRegistrationParams regParams;
    GPacsC* pacsClient = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_PACS_CLIENT_PANIC("Pacs:  Invalid task\n\n");
        return;
    }

    clientHandle = getPacsServiceHandle(&pacsClient, &(mainInst->clientHandleList));

    GATT_PACS_CLIENT_INFO("Pacs:  GattPacsClientInit\n\n");

    if(pacsClient == NULL)
    {
        GattPacsClientInitCfm* cfm = (GattPacsClientInitCfm *)CsrPmemZalloc(sizeof(GattPacsClientInitCfm));

        cfm->cid = initData->cid;
        cfm->id = GATT_PACS_CLIENT_INIT_CFM;
        cfm->clientHandle = 0;
        cfm->status = GATT_PACS_CLIENT_STATUS_FAILED;

        GATT_PACS_CLIENT_ERROR(" PACS Client initialization failed \n\n");

        CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
        return;
    }

    /* Keep track on application task as all the notifications and indication need to be send to here */
    pacsClient->app_task = appTask;
    /* Store library task for external message reception */
    pacsClient->lib_task = CSR_BT_PACS_CLIENT_IFACEQUEUE;  /* PACS client interface queue */
    pacsClient->writeSinkCccdCount = 0;
    pacsClient->writeSourceCccdCount = 0;

    pacsClient->startHandle = initData->startHandle;
    pacsClient->endHandle = initData->endHandle;
    pacsClient->notifyEnable = FALSE;
    pacsClient->notifyType = 0;
    pacsClient->notifyTypeEnabled = 0;

    /*Store the PACS characteristics handle with Invalid Value */
    pacsClient->pacsSinkAudioLocationHandle = INVALID_PACS_HANDLE;
    pacsClient->pacsSinkAudioLocationCcdHandle = INVALID_PACS_HANDLE;

    pacsClient->pacsSourceAudioLocationHandle = INVALID_PACS_HANDLE;
    pacsClient->pacsSourceAudioLocationCcdHandle = INVALID_PACS_HANDLE;

    pacsClient->pacsAvailableAudioContextHandle = INVALID_PACS_HANDLE;
    pacsClient->pacsAvailableAudioContextCcdHandle = INVALID_PACS_HANDLE;

    pacsClient->pacsSupportedAudioContextHandle = INVALID_PACS_HANDLE;
    pacsClient->pacsSupportedAudioContextCcdHandle = INVALID_PACS_HANDLE;

    CsrCmnListInit(&pacsClient->sinkPacRecordList, 0, InitPacRecordCharcList, NULL);
    pacsClient->sinkPacRecordCount = 0;

    CsrCmnListInit(&pacsClient->sourcePacRecordList, 0, InitPacRecordCharcList, NULL);
    pacsClient->sourcePacRecordCount = 0;

    pacsClient->srvcElem->cid = initData->cid;
    pacsClient->srvcElem->service_handle = clientHandle;

    regParams.cid = initData->cid;
    regParams.startHandle = initData->startHandle;
    regParams.endHandle = initData->endHandle;

    /* If the device is already known, get the persistent data */

    if (deviceData)
    {
        /* Don't need to discover data from the device; use the data supplied instead */
        pacsClient->sinkPacRecordCount = deviceData->sinkPacRecordCount;
        pacsClient->sourcePacRecordCount = deviceData->sourcePacRecordCount;

        pacsClient->pacsSinkAudioLocationHandle = deviceData->pacsSinkAudioLocationHandle;
        pacsClient->pacsSinkAudioLocationCcdHandle = deviceData->pacsSinkAudioLocationHandle;

        pacsClient->pacsSourceAudioLocationHandle = deviceData->pacsSourceAudioLocationHandle;
        pacsClient->pacsSourceAudioLocationCcdHandle = deviceData->pacsSourceAudioLocationCcdHandle;

        pacsClient->pacsAvailableAudioContextHandle = deviceData->pacsAvailableAudioContextHandle;
        pacsClient->pacsAvailableAudioContextCcdHandle = deviceData->pacsAvailableAudioContextCcdHandle;

        pacsClient->pacsSupportedAudioContextHandle = deviceData->pacsSupportedAudioContextHandle;
        pacsClient->pacsSupportedAudioContextCcdHandle = deviceData->pacsSupportedAudioContextCcdHandle;

        pacsClient->endHandle = deviceData->endHandle;
        pacsClient->startHandle = deviceData->startHandle;

        for (index = 0; index < deviceData->sinkPacRecordCount; index++)
        {
            CsrBtPacRecordCharacElement* elem = (CsrBtPacRecordCharacElement*)ADD_PAC_RECORD(pacsClient->sinkPacRecordList);

            pacsAddPacRecordHandles(elem, deviceData->pacsSinkPacRecordHandle[index],
                                     deviceData->pacsSinkPacRecordCcdHandle[index], index + 1);
        }

        for (index = 0; index < deviceData->sourcePacRecordCount; index++)
        {
            CsrBtPacRecordCharacElement* elem = (CsrBtPacRecordCharacElement*)ADD_PAC_RECORD(pacsClient->sourcePacRecordList);

            pacsAddPacRecordHandles(elem, deviceData->pacsSourcePacRecordHandle[index],
                                       deviceData->pacsSourcePacRecordCcdHandle[index], index + 1);
        }
    }

    if (GattRegisterPacsClient(&regParams, pacsClient))
    {
        if (!deviceData)
        {
            pacsGattDiscoverRequest(pacsClient);
        }
        else
        {
            pacsClientSendInitCfm(pacsClient, GATT_PACS_CLIENT_STATUS_SUCCESS);
        }
    }
    else
    {
        GATT_PACS_CLIENT_ERROR("Register with the GATT failed!\n");
        pacsClientSendInitCfm(pacsClient, GATT_PACS_CLIENT_STATUS_FAILED);
    }
}

void GattPacsClientTerminateReq(ServiceHandle clientHandle)
{
    if(clientHandle)
    {
        GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);
        if (pacsClient == NULL)
        {
            GATT_PACS_CLIENT_ERROR("PACS:  invalid client Instance\n\n");
            PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        }
        else
        {
            CsrBtGattUnregisterReqSend(pacsClient->srvcElem->gattId);
            pacsClientSendTerminateCfm(pacsClient);
        }
    }
}

void GattPacsClientReadPacRecordReq(ServiceHandle clientHandle,
                                   GattPacsClientType type)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }
    GATT_PACS_CLIENT_INFO("Pacs: Pac Record Type 0x%x \n\n",type);

    switch (type)
    {
        case GATT_PACS_CLIENT_SINK:
        {
            if (pacsClient->sinkPacRecordCount)
            {
                pacsClient->readSinkRecordCount = pacsClient->sinkPacRecordCount;
                CsrCmnListIterate(&pacsClient->sinkPacRecordList,
                                  pacRecordRead,
                                  (void*)pacsClient);
            }
            else
            {
                pacsClientSendPacsCapabilitiesCfm(pacsClient->srvcElem->service_handle, 
                                      pacsClient->app_task, GATT_PACS_CLIENT_STATUS_FAILED, 
									  GATT_PACS_CLIENT_SINK,0, 0, NULL, FALSE);
            }
            break;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {
            if (pacsClient->sourcePacRecordCount)
            {
                pacsClient->readSourceRecordCount = pacsClient->sourcePacRecordCount;
                CsrCmnListIterate(&pacsClient->sourcePacRecordList,
                                   pacRecordRead,
                                   (void*)pacsClient);
            }
            else
            {
                pacsClientSendPacsCapabilitiesCfm(pacsClient->srvcElem->service_handle,
                                                pacsClient->app_task, GATT_PACS_CLIENT_STATUS_FAILED,
												GATT_PACS_CLIENT_SOURCE,0, 0, NULL, FALSE);
            }
            break;
        }
        default:
        {
            /* invalid  type*/
            GATT_PACS_CLIENT_WARNING("INVALID type \n\n");
            pacsClientSendPacsCapabilitiesCfm(pacsClient->srvcElem->service_handle,
                                        pacsClient->app_task, GATT_PACS_CLIENT_STATUS_FAILED,
                                        INVALID_PACS_CLIENT_TYPE,0, 0, NULL, FALSE);
        }
    }
}


void GattPacsClientReadAudioLocationReq(ServiceHandle clientHandle,
                                       GattPacsClientType type)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    GATT_PACS_CLIENT_INFO("Pacs: Read Audio Location 0x%x \n\n",type);

    switch (type)
    {
        case GATT_PACS_CLIENT_SINK:
        {
            if (pacsClient->pacsSinkAudioLocationHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattReadReqSend(pacsClient->srvcElem->gattId,
                                    pacsClient->srvcElem->cid,
                                    pacsClient->pacsSinkAudioLocationHandle,
                                    0);
            }
            else
            {
                pacsClientSendAudioLocationReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                                                   0, GATT_PACS_CLIENT_STATUS_FAILED, GATT_PACS_CLIENT_SINK);
            }
            break;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {
            if (pacsClient->pacsSourceAudioLocationHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattReadReqSend(pacsClient->srvcElem->gattId,
                                    pacsClient->srvcElem->cid,
                                    pacsClient->pacsSourceAudioLocationHandle,
                                    0);
            }
            else
            {
                pacsClientSendAudioLocationReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                                                   0, GATT_PACS_CLIENT_STATUS_FAILED, GATT_PACS_CLIENT_SOURCE);
            }
            break;
        }
        default:
        {
            /* invalid  type*/
            GATT_PACS_CLIENT_WARNING("INVALID type \n\n");
            pacsClientSendAudioLocationReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                                                    0, GATT_PACS_CLIENT_STATUS_FAILED, INVALID_PACS_CLIENT_TYPE);
        }
    }
}

void GattPacsClientReadAudioContextReq(ServiceHandle clientHandle,
                                      GattPacsClientAudioContext context)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    GATT_PACS_CLIENT_INFO("Pacs:  Read Audio context 0x%x\n\n", context);


    switch (context)
    {
        case GATT_PACS_CLIENT_AVAILABLE:
        {

            if (pacsClient->pacsAvailableAudioContextHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattReadReqSend(pacsClient->srvcElem->gattId,
                    pacsClient->srvcElem->cid,
                    pacsClient->pacsAvailableAudioContextHandle,
                    0);
            }
            else
            {
                pacsClientSendAudioContextReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                		GATT_PACS_CLIENT_AVAILABLE, 0, GATT_PACS_CLIENT_STATUS_FAILED);
            }
            break;
        }
        case GATT_PACS_CLIENT_SUPPORTED:
        {
            if (pacsClient->pacsSupportedAudioContextHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattReadReqSend(pacsClient->srvcElem->gattId,
                    pacsClient->srvcElem->cid,
                    pacsClient->pacsSupportedAudioContextHandle,
                    0);
            }
            else
            {
                pacsClientSendAudioContextReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                		GATT_PACS_CLIENT_SUPPORTED, 0, GATT_PACS_CLIENT_STATUS_FAILED);
            }
            break;
        }
        default:
        {
            /* invalid  context*/
            GATT_PACS_CLIENT_WARNING("INVALID audio context \n\n");
            pacsClientSendAudioContextReadCfm(pacsClient->srvcElem->service_handle, pacsClient->app_task,
                                                   INVALID_PACS_CONTEXT_TYPE, 0, GATT_PACS_CLIENT_STATUS_FAILED);
        }

    }
}

void GattPacsClientWriteAudioLocationReq(ServiceHandle clientHandle,
                                        GattPacsClientType type,
                                        uint32 value)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);
    uint16 valueLength = sizeof(value);
    uint8* locValue = (uint8*)(CsrPmemZalloc(valueLength));

    locValue[0] = (uint8)(value & 0x000000ff);
    locValue[1] = (uint8)((value >> 8) & 0x0000ff00);
    locValue[2] = (uint8)((value >> 16) & 0x00ff0000);
    locValue[3] = (uint8)((value >> 24) & 0xff000000u);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    GATT_PACS_CLIENT_INFO("Pacs: Write Audio Location 0x%x \n\n",type);

    switch (type)
    {
        case GATT_PACS_CLIENT_SINK:
        {
            if (pacsClient->pacsSinkAudioLocationHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattWriteReqSend(pacsClient->srvcElem->gattId,
                                    pacsClient->srvcElem->cid,
                                    pacsClient->pacsSinkAudioLocationHandle,
                                    0,
                                    valueLength,
                                    locValue);
            }
            else
            {
                pacsClientSendWriteCfm(pacsClient->app_task,
                                 GATT_PACS_CLIENT_STATUS_FAILED,
                                 pacsClient->srvcElem->service_handle);
            }
            break;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {
            if (pacsClient->pacsSourceAudioLocationHandle != INVALID_PACS_HANDLE)
            {
                CsrBtGattWriteReqSend(pacsClient->srvcElem->gattId,
                                    pacsClient->srvcElem->cid,
                                    pacsClient->pacsSourceAudioLocationHandle,
                                    0,
                                    valueLength,
                                    locValue);
            }
            else
            {
                pacsClientSendWriteCfm(pacsClient->app_task,
                                       GATT_PACS_CLIENT_STATUS_FAILED,
                                       pacsClient->srvcElem->service_handle);
            }
            break;
        }
        default:
        {
            /* invalid  type*/
            GATT_PACS_CLIENT_WARNING("INVALID type \n\n");
            pacsClientSendWriteCfm(pacsClient->app_task,
                          GATT_PACS_CLIENT_STATUS_FAILED,
                          pacsClient->srvcElem->service_handle);
        }
    }
}

bool GattPacsClientFindAudioRoleReq(ServiceHandle clientHandle,
                                   GattPacsClientType type)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        return FALSE;
    }

    switch (type)
    {
        case GATT_PACS_CLIENT_SINK:
        {
            if (pacsClient->sinkPacRecordCount)
                return TRUE;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {
            if (pacsClient->sourcePacRecordCount)
                return TRUE;
        }
        default:
            return FALSE;
    }
}

void GattPacsClientRegisterForNotification(ServiceHandle clientHandle,
                                           GattPacsNotificationType notifyType,
                                           bool notifyEnable)
{
    PacsClientInternalMsgNotificationReq *req = (PacsClientInternalMsgNotificationReq *)
                                          CsrPmemZalloc(sizeof(PacsClientInternalMsgNotificationReq));

    req->id = PACS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ;
    req->clientHandle = clientHandle;
    req->enable = notifyEnable;
    req->type = notifyType;

    CsrSchedMessagePut(CSR_BT_PACS_CLIENT_IFACEQUEUE, PACS_CLIENT_PRIM, req);
}
/******************************************************************************/
