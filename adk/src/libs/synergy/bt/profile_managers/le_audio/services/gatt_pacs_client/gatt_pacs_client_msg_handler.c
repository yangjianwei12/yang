/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_pmem.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"
#include "gatt_pacs_client_util.h"
#include "gatt_pacs_client_debug.h"
#include "gatt_pacs_client_private.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_pacs_client_msg_handler.h"


extern PacsC *mainInst;

/****************************************************************************/

void pacsClientWriteRequestSend(CsrBtGattId gattId,
                                connection_id_t cid,
                                uint16 handle,
                                bool notifyEnable)
{
    uint8* value = (uint8*)(CsrPmemZalloc(GATT_PACS_WRITE_CCD_VALUE_LENGTH));
    value[0] = notifyEnable ? CSR_BT_GATT_CLIENT_CHARAC_CONFIG_NOTIFICATION : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(gattId,
                         cid,
                         handle,
                         0,
                         GATT_PACS_WRITE_CCD_VALUE_LENGTH,
                         value);
}

void pacsClientSendPacsCapabilitiesCfm(ServiceHandle clientHandle,
                                      AppTask appTask,
                                      GattPacsClientStatus status,
                                      GattPacsClientType type,
                                      uint8 pacRecordCount,
                                      uint16 valueLength,
                                      uint8* value,
                                      bool moreToCome)
{
    GattPacsClientReadPacRecordCfm* cfm = (GattPacsClientReadPacRecordCfm*)
                                   CsrPmemZalloc(sizeof(GattPacsClientReadPacRecordCfm));

    cfm->clientHandle = clientHandle;
    cfm->moreToCome = moreToCome;
    cfm->id = GATT_PACS_CLIENT_READ_PAC_RECORD_CFM;
    cfm->record.pacRecordCount = pacRecordCount;
    cfm->record.valueLength = valueLength;
    cfm->record.value = value;
    cfm->status = status;
    cfm->type = type;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
}

void pacsClientSendAudioLocationReadCfm(ServiceHandle clientHandle,
                                       AppTask appTask,
                                       uint32 location,
                                       GattPacsClientStatus status,
                                       GattPacsClientType type)
{
    GattPacsClientReadAudioLocationCfm* cfm =
                             CsrPmemZalloc(sizeof(GattPacsClientReadAudioLocationCfm));
    cfm->clientHandle = clientHandle;
    cfm->id = GATT_PACS_CLIENT_READ_AUDIO_LOCATION_CFM;
    cfm->location = location;
    cfm->status = status;
    cfm->type = type;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
}

void pacsClientSendAudioContextReadCfm(ServiceHandle clientHandle,
                                     AppTask appTask,
                                     uint16 context,
                                     uint32 value,
                                     GattPacsClientStatus status)
{
    GattPacsClientReadAudioContextCfm* cfm =
                                       CsrPmemZalloc(sizeof(GattPacsClientReadAudioContextCfm));
    cfm->clientHandle = clientHandle;
    cfm->id = GATT_PACS_CLIENT_READ_AUDIO_CONTEXT_CFM;
    cfm->context = context;
    cfm->status = status;
    cfm->value = value;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
}

void pacsClientSendWriteCfm(AppTask appTask,
                            GattPacsClientStatus status,
                            ServiceHandle clientHandle)
{
    GattPacsClientWriteAudioLocationCfm* cfm =(GattPacsClientWriteAudioLocationCfm*)
                                      CsrPmemZalloc(sizeof(GattPacsClientWriteAudioLocationCfm));

    cfm->id = GATT_PACS_CLIENT_WRITE_AUDIO_LOCATION_CFM;
    cfm->status = status;
    cfm->clientHandle = clientHandle;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
}

static void pacsClientSendAudioContextNtfInd(ServiceHandle clientHandle,
                                              AppTask appTask,
                                              uint16 context,
                                              uint32 value)
{
    GattPacsClientAudioContextNotificationInd* ind =
               CsrPmemZalloc(sizeof(GattPacsClientAudioContextNotificationInd));

    ind->clientHandle = clientHandle;
    ind->id = GATT_PACS_CLIENT_AUDIO_CONTEXT_NOTIFICATION_IND;
    ind->context = context;
    ind->contextValue = value;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, ind);
}

static void pacsClientSendAudioLocationNtfInd(ServiceHandle clientHandle,
                                              AppTask appTask,
                                              uint32 location,
                                              GattPacsClientType type)
{
    GattPacsClientAudioLocationNotificationInd* ind =
        CsrPmemZalloc(sizeof(GattPacsClientAudioLocationNotificationInd));
    ind->clientHandle = clientHandle;
    ind->id = GATT_PACS_CLIENT_AUDIO_LOCATION_NOTIFICATION_IND;
    ind->location = location;
    ind->type = type;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, ind);
}

static void pacsClientSendPacRecordNtfInd(ServiceHandle clientHandle,
                                           AppTask appTask,
                                           GattPacsClientType type,
                                           uint8 recordCount,
                                           uint16 valueLength,
                                           uint8* value)
{
    GattPacsClientPacRecordNotificationInd* ind =
        CsrPmemZalloc(sizeof(GattPacsClientPacRecordNotificationInd));

    ind->id = GATT_PACS_CLIENT_PAC_RECORD_NOTIFICATION_IND;
    ind->clientHandle = clientHandle;
    ind->type = type;
    ind->record.pacRecordCount = recordCount;
    ind->record.value = value;
    ind->record.valueLength = valueLength;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, ind);

}

static void pacsClientSendRegisterForNotificationCfm(GPacsC *pacsClient,
                                                   GattPacsClientStatus status,
                                                   ServiceHandle clientHandle,
                                                   AppTask appTask)
{
    GattPacsClientNotificationCfm* notiCfm = (GattPacsClientNotificationCfm*)
        CsrPmemZalloc(sizeof(GattPacsClientNotificationCfm));

    notiCfm->clientHandle = pacsClient->srvcElem->service_handle;
    notiCfm->id = GATT_PACS_CLIENT_NOTIFICATION_CFM;
    notiCfm->status = status;
    notiCfm->clientHandle = clientHandle;

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, notiCfm);
}

static void pacsClientnotificationReqSend(GPacsC* const pacsClient)
{
    if (pacsClient->notifyType &
                GATT_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT)
    {
        pacsClientRegisterForAudioContextNotification(pacsClient->srvcElem->service_handle,
                                            GATT_PACS_CLIENT_AVAILABLE,
                                            pacsClient->notifyEnable);

    }
    else if (pacsClient->notifyType &
               GATT_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT)
    {
        pacsClientRegisterForAudioContextNotification(pacsClient->srvcElem->service_handle,
                                               GATT_PACS_CLIENT_SUPPORTED,
                                               pacsClient->notifyEnable);
    }
    else if (pacsClient->notifyType &
                          GATT_PACS_NOTIFICATION_SINK_PAC_RECORD)
    {
        pacsClientRegisterForPacRecordNotification(pacsClient->srvcElem->service_handle,
                                                GATT_PACS_CLIENT_SINK,
                                                pacsClient->notifyEnable);
    }
    else if (pacsClient->notifyType &
                         GATT_PACS_NOTIFICATION_SINK_AUDIO_LOCATION)
    {
        pacsClientRegisterForAudioLocationNotification(pacsClient->srvcElem->service_handle,
                                                 GATT_PACS_CLIENT_SINK,
                                                 pacsClient->notifyEnable);
    }
    else if (pacsClient->notifyType &
                          GATT_PACS_NOTIFICATION_SOURCE_PAC_RECORD)
    {
        pacsClientRegisterForPacRecordNotification(pacsClient->srvcElem->service_handle,
                                                  GATT_PACS_CLIENT_SOURCE,
                                                  pacsClient->notifyEnable);
    }
    else if (pacsClient->notifyType &
                           GATT_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION)
    {
        pacsClientRegisterForAudioLocationNotification(pacsClient->srvcElem->service_handle,
                                                     GATT_PACS_CLIENT_SOURCE,
                                                     pacsClient->notifyEnable);
    }
}


static GattPacsClientStatus pacsGetStatus(const CsrBtResultCode status)
{
    GattPacsClientStatus pacsStatus;

    switch (status)
    {

        case CSR_BT_GATT_RESULT_SUCCESS:
        case CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE:
            pacsStatus = GATT_PACS_CLIENT_STATUS_SUCCESS;
            break;

        case CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID:
            pacsStatus = GATT_PACS_CLIENT_STATUS_NO_CONNECTION;
            break;

        case CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED:
            pacsStatus = GATT_PACS_CLIENT_STATUS_NOT_ALLOWED;
            break;

        default:
            pacsStatus = GATT_PACS_CLIENT_STATUS_FAILED;
            break;
    }

    return pacsStatus;
}


static void pacsGattDiscoverCharacCfmHandler(GPacsC *const pacsClient,
                                       GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg)
{
    GATT_PACS_CLIENT_DEBUG("Pacs:  csrBtPacsGattDiscoverCharacCfmHandler\n\n");

    if (msg->status == ATT_RESULT_SUCCESS)
    {
        if (msg->uuid_type == ATT_UUID16)
        {
            switch (msg->uuid[0])
            {
                case CSR_BT_GATT_UUID_PACS_SINK:
                {
                    CsrBtPacRecordCharacElement* elem = (CsrBtPacRecordCharacElement*)ADD_PAC_RECORD(pacsClient->sinkPacRecordList);
                    elem->declarationHandle = msg->declaration;
                    elem->recordId = pacsClient->sinkPacRecordCount;
                    elem->valueHandle = msg->handle;
                    elem->pacRecordCccdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    pacsClient->sinkPacRecordCount++;
                    break;
                }
                case CSR_BT_GATT_UUID_PACS_SOURCE:
                {
                    CsrBtPacRecordCharacElement* elem = (CsrBtPacRecordCharacElement*)ADD_PAC_RECORD(pacsClient->sourcePacRecordList);
                    elem->declarationHandle = msg->declaration;
                    elem->recordId = pacsClient->sourcePacRecordCount;
                    elem->valueHandle = msg->handle;
                    elem->pacRecordCccdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    pacsClient->sourcePacRecordCount++;
                    break;
                }
                case CSR_BT_GATT_UUID_PACS_SINK_AUDIO_LOC:
                {
                    pacsClient->pacsSinkAudioLocationHandle = msg->handle;
                    pacsClient->pacsSinkAudioLocationCcdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;

                    break;
                }
                case CSR_BT_GATT_UUID_PACS_SOURCE_AUDIO_LOC:
                {
                    pacsClient->pacsSourceAudioLocationHandle = msg->handle;
                    pacsClient->pacsSourceAudioLocationCcdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    break;
                }
                case CSR_BT_GATT_UUID_PACS_SUPPORTED_AUDIO_CONTEXT:
                {
                    pacsClient->pacsSupportedAudioContextHandle = msg->handle;
                    pacsClient->pacsSupportedAudioContextCcdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    break;
                }
                case CSR_BT_GATT_UUID_PACS_AVAILABLE_AUDIO_CONTEXT:
                {
                    pacsClient->pacsAvailableAudioContextHandle = msg->handle;
                    pacsClient->pacsAvailableAudioContextCcdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    break;
                }
                default:
                    break;
            }
        }

        if (!msg->more_to_come)
        {
            if (((!pacsClient->sinkPacRecordCount) && (!pacsClient->sourcePacRecordCount))
                || (!pacsClient->pacsSupportedAudioContextHandle) ||
                (!pacsClient->pacsAvailableAudioContextHandle))
            {
                /* if any one of the handles not discovered, send failed error*/
                pacsClientSendInitCfm(pacsClient, CSR_BT_GATT_RESULT_INTERNAL_ERROR);
            }
            else
            {
                /* Do CCCD discovery*/
                CsrBtGattDiscoverAllCharacDescriptorsReqSend(pacsClient->srvcElem->gattId,
                                                             pacsClient->srvcElem->cid,
                                                             pacsClient->startHandle + 1,
                                                             pacsClient->endHandle);

            }
        }
    }
    else
    {
        /*send the error*/
        pacsClientSendInitCfm(pacsClient, msg->status);

    }
}


static void pacsGattDiscoverCharacDescCfmHandler(GPacsC* const pacsClient,
                             GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T* msg)
{
    if (msg->status == ATT_RESULT_SUCCESS)
    {
        if (msg->uuid_type == ATT_UUID16)
        {
            switch (msg->uuid[0])
            {
                case CSR_BT_GATT_UUID_PACS_SINK:
                case CSR_BT_GATT_UUID_PACS_SOURCE:
                case CSR_BT_GATT_UUID_PACS_SINK_AUDIO_LOC:
                case CSR_BT_GATT_UUID_PACS_SOURCE_AUDIO_LOC:
                case CSR_BT_GATT_UUID_PACS_AVAILABLE_AUDIO_CONTEXT:
                case CSR_BT_GATT_UUID_PACS_SUPPORTED_AUDIO_CONTEXT:
                {
                    pacsClient->pendingHandle = msg->handle;
                    break;
                }
                case CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC:
                {
                    if (pacsClient->pendingHandle == pacsClient->pacsSinkAudioLocationHandle)
                    {
                        pacsClient->pacsSinkAudioLocationCcdHandle = msg->handle;
                        pacsClient->pendingHandle = 0;
                    }
                    else if (pacsClient->pendingHandle == pacsClient->pacsSourceAudioLocationHandle)
                    {
                        pacsClient->pacsSourceAudioLocationCcdHandle = msg->handle;
                        pacsClient->pendingHandle = 0;
                    }
                    else if (pacsClient->pendingHandle == pacsClient->pacsSupportedAudioContextHandle)
                    {
                        pacsClient->pacsSupportedAudioContextCcdHandle = msg->handle;
                        pacsClient->pendingHandle = 0;
                    }
                    else if (pacsClient->pendingHandle == pacsClient->pacsAvailableAudioContextHandle)
                    {
                        pacsClient->pacsAvailableAudioContextCcdHandle = msg->handle;
                        pacsClient->pendingHandle = 0;
                    }
                    else
                    {
                        if (pacsClient->sinkPacRecordCount)
                        {
                            CsrCmnListElm_t* elem = (CsrCmnListElm_t*)(&pacsClient->sinkPacRecordList)->first;
                            for (;elem;elem = elem->next)
                            {
                                CsrBtPacRecordCharacElement* sinkPacRecordElem = (CsrBtPacRecordCharacElement*)elem;
                                if (pacsClient->pendingHandle == sinkPacRecordElem->valueHandle)
                                {
                                    sinkPacRecordElem->pacRecordCccdHandle = msg->handle;
                                    return;
                                }

                            }
                        }

                        if (pacsClient->sourcePacRecordCount)
                        {
                            CsrCmnListElm_t* elem = (CsrCmnListElm_t*)(&pacsClient->sourcePacRecordList)->first;
                            for (;elem;elem = elem->next)
                            {
                                CsrBtPacRecordCharacElement* sourcePacRecordElem = (CsrBtPacRecordCharacElement*)elem;
                                if (pacsClient->pendingHandle == sourcePacRecordElem->valueHandle)
                                {
                                    sourcePacRecordElem->pacRecordCccdHandle = msg->handle;
                                    return;
                                }
                            }
                        }

                        pacsClient->pendingHandle = 0;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (!msg->more_to_come)
    {
        if (!pacsClient->pacsSupportedAudioContextCcdHandle ||
             !pacsClient->pacsAvailableAudioContextCcdHandle)
        {
            pacsClientSendInitCfm(pacsClient, CSR_BT_GATT_RESULT_INTERNAL_ERROR);
        }
        else
        {
            /*send init cfm*/
            pacsClientSendInitCfm(pacsClient, CSR_BT_GATT_RESULT_SUCCESS);
        }
    }

}

static GattPacsClientType returnClientType(GPacsC *pacs,uint16_t handle)
{
    CsrCmnListElm_t* elem;
    if (!(pacs->sinkPacRecordCount) && !(pacs->sourcePacRecordCount))
        return INVALID_PACS_CLIENT_TYPE;

    elem = (CsrCmnListElm_t*)(&pacs->sinkPacRecordList)->first;

    for (;elem; elem = elem->next)
    {
        CsrBtPacRecordCharacElement* record = (CsrBtPacRecordCharacElement*)elem;
        if (record->valueHandle == handle)
            return GATT_PACS_CLIENT_SINK;
    }

    elem = (CsrCmnListElm_t*)(&pacs->sourcePacRecordList)->first;

    for (;elem; elem = elem->next)
    {
        CsrBtPacRecordCharacElement* record = (CsrBtPacRecordCharacElement*)elem;
        if (record->valueHandle == handle)
            return GATT_PACS_CLIENT_SOURCE;
    }

    return INVALID_PACS_CLIENT_TYPE;
}

static uint32 pacsClientPackUint8ToUint32(uint8 *value, 
                                          GattPacsClientStatus resultCode,
                                          uint16 valueLength)
{
    uint32 valuePacked = 0;

    if (resultCode == CSR_BT_GATT_RESULT_SUCCESS && (valueLength))
    {
        valuePacked = value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
    }

    return valuePacked;
}

static void pacsClientHandlePacRecordReadCfmAndNtfInd(GPacsC* pacsClient,
                                                uint16 handle,
                                                uint8* value,
                                                GattPacsClientStatus resultCode,
                                                uint16 valueLength,
                                                bool notification)
{
    /* Send read confirmation to application */
    uint8 recordCount = 0;
    uint8* record = NULL;
    bool moreToCome = FALSE;
    uint16 len = 0;
    GattPacsClientType type = INVALID_PACS_CLIENT_TYPE;
    ServiceHandle clientHandle = pacsClient->srvcElem->service_handle;

    type = returnClientType(pacsClient, handle);
    GATT_PACS_CLIENT_DEBUG("\nPacs: pacsClientHandlePacRecordReadAndNoti: ");

    if (type == GATT_PACS_CLIENT_SINK)
    {
        pacsClient->readSinkRecordCount--;
        moreToCome = (pacsClient->readSinkRecordCount > 0);
    }
    else if (type == GATT_PACS_CLIENT_SOURCE)
    {
        pacsClient->readSourceRecordCount--;
        moreToCome = (pacsClient->readSourceRecordCount > 0);
    }

    if (resultCode == CSR_BT_GATT_RESULT_SUCCESS && (valueLength))
    {
        len = valueLength - 1;

        if (len)
        {
            record = (uint8*)CsrPmemZalloc(len);

            if (value)
            {
				recordCount = value[0];
                CsrMemCpy(record, &value[1], len);
            }
        }
    }

    if (notification)
    {
        pacsClientSendPacRecordNtfInd(clientHandle, pacsClient->app_task,
                                       type, recordCount, valueLength, record);
    }
    else
    {
        pacsClientSendPacsCapabilitiesCfm(clientHandle,pacsClient->app_task, 
                                           pacsGetStatus(resultCode), type ,recordCount, len, record, moreToCome);
    }
}


static void pacsClientHandleAudioLocReadCfmAndNtfInd(GPacsC* pacsClient, 
                                               uint16 handle,
                                               uint8 *value,
                                               GattPacsClientStatus resultCode,
                                               uint16 valueLength,
                                               bool notification)
{
    uint32 location = 0;
    GattPacsClientType type = INVALID_PACS_CLIENT_TYPE;
    ServiceHandle clientHandle = pacsClient->srvcElem->service_handle;
    bool  isSink = (handle == pacsClient->pacsSinkAudioLocationHandle) ? TRUE : FALSE;

    type = isSink ? GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;
    location = pacsClientPackUint8ToUint32(value, resultCode, valueLength);
    GATT_PACS_CLIENT_DEBUG("\nPacs: pacsClientHandleAudioLocReadAndNoti: ");

    if (notification)
    {
        pacsClientSendAudioLocationNtfInd(clientHandle, pacsClient->app_task, location, type);
    }
    else
    {
        pacsClientSendAudioLocationReadCfm(clientHandle, pacsClient->app_task, location, 
                                                          pacsGetStatus(resultCode), type);
    }
}

static void pacsClientHandleAudioContextReadCfmAndNtfInd(GPacsC* pacsClient,
                                                   uint16 handle,
                                                   uint8* value,
                                                   GattPacsClientStatus resultCode,
                                                   uint16 valueLength,
                                                   bool notification)
{
    uint32 contextValue = 0;
    GattPacsClientAudioContext context = INVALID_PACS_CONTEXT_TYPE;
    ServiceHandle clientHandle = pacsClient->srvcElem->service_handle;
    bool available = (handle == pacsClient->pacsAvailableAudioContextHandle) ? TRUE : FALSE;

    context = available ? GATT_PACS_CLIENT_AVAILABLE : GATT_PACS_CLIENT_SUPPORTED;
    contextValue = pacsClientPackUint8ToUint32(value, resultCode, valueLength);
    GATT_PACS_CLIENT_DEBUG("\nPacs: pacsClientHandleAudioContextReadAndNoti: ");

    if (notification)
    {
        pacsClientSendAudioContextNtfInd(clientHandle, pacsClient->app_task, context, contextValue);

    }
    else
    {
        pacsClientSendAudioContextReadCfm(clientHandle, pacsClient->app_task,context, 
                                                         contextValue, pacsGetStatus(resultCode));
    }
}


static void pacsGattReadHandler(GPacsC *pacsClient, CsrBtGattReadCfm* msg)
{

    GATT_PACS_CLIENT_DEBUG("Pacs: Read cfm for PACS characteristic");

    if (msg->handle == pacsClient->pacsSinkAudioLocationHandle ||
        msg->handle == pacsClient->pacsSourceAudioLocationHandle)
    {
        pacsClientHandleAudioLocReadCfmAndNtfInd(pacsClient, msg->handle, msg->value, 
                                             msg->resultCode, msg->valueLength, FALSE);
    }
    else if (msg->handle == pacsClient->pacsAvailableAudioContextHandle ||
               msg->handle == pacsClient->pacsSupportedAudioContextHandle)
    {
        pacsClientHandleAudioContextReadCfmAndNtfInd(pacsClient, msg->handle, msg->value,
                                                   msg->resultCode, msg->valueLength, FALSE);
    }
    else
    {
        pacsClientHandlePacRecordReadCfmAndNtfInd(pacsClient, msg->handle, msg->value,
                                           msg->resultCode, msg->valueLength, FALSE);
    }
}

static void pacsGattNotificationHandler(GPacsC* const pacsClient,
                                       CsrBtGattClientNotificationInd* msg)
{
    GATT_PACS_CLIENT_DEBUG("Pacs: Gatt Notification recieved \n\n");

    if (msg->valueHandle == pacsClient->pacsSinkAudioLocationHandle ||
        msg->valueHandle == pacsClient->pacsSourceAudioLocationHandle)
    {
        pacsClientHandleAudioLocReadCfmAndNtfInd(pacsClient, msg->valueHandle, msg->value,
                                         CSR_BT_GATT_RESULT_SUCCESS, msg->valueLength, TRUE);
    }
    else if (msg->valueHandle == pacsClient->pacsAvailableAudioContextHandle ||
        msg->valueHandle == pacsClient->pacsSupportedAudioContextHandle)
    {
        pacsClientHandleAudioContextReadCfmAndNtfInd(pacsClient, msg->valueHandle, msg->value,
                                      CSR_BT_GATT_RESULT_SUCCESS, msg->valueLength, TRUE);

    }
    /* Allocate and fill indication message to be sent to app */
    else
    {
        pacsClientHandlePacRecordReadCfmAndNtfInd(pacsClient, msg->valueHandle, msg->value,
                                       CSR_BT_GATT_RESULT_SUCCESS, msg->valueLength, TRUE);

    }
}

static void handleNotificationCfm(GPacsC* const pacsClient, 
                                  GattPacsClientStatus status)
{
    if (pacsClient->notifyType == 0)
    {
        if (pacsClient->internalReq)
        {
            pacsClient->internalReq = FALSE;
            GATT_PACS_CLIENT_DEBUG("\n(PACS): Notification enable Status: 0x%x \n", status);
        }
        else 
        {
            pacsClientSendRegisterForNotificationCfm(pacsClient,
                                                     status,
                                                     pacsClient->srvcElem->service_handle,
                                                     pacsClient->app_task);
        }
    }
    else
    {
        pacsClientnotificationReqSend(pacsClient);
    }
}

static void pacsClientNotificationFlagUpdate(GPacsC* const pacsClient,
                                             uint16 type)
{
    if (pacsClient->notifyEnable)
        pacsClient->notifyTypeEnabled |= type;
    else
        pacsClient->notifyTypeEnabled &= ~type;
}

static void pacsGattWriteHandler(GPacsC *const pacsClient, CsrBtGattWriteCfm* msg)
{
    GATT_PACS_CLIENT_INFO("Pacs: Write cfm handle : 0x%02x, status: %d\n\n",
                           msg->handle, msg->resultCode);

    /* Allocate and fill indication message to be sent to app */

    if (msg->handle == pacsClient->pacsSinkAudioLocationHandle ||
            msg->handle == pacsClient->pacsSourceAudioLocationHandle)
    {
        pacsClientSendWriteCfm(pacsClient->app_task,
                             pacsGetStatus(msg->resultCode),
                             pacsClient->srvcElem->service_handle);
    }
    else
    {
        if (msg->resultCode != CSR_BT_GATT_RESULT_SUCCESS)
        {

            pacsClient->notifyType = 0;
            pacsClient->writeSinkCccdCount = 0;
            pacsClient->writeSourceCccdCount = 0;

            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
            return;
        }

        if (msg->handle == pacsClient->pacsAvailableAudioContextCcdHandle)
        {
            pacsClient->notifyType = (pacsClient->notifyType &
                              (~GATT_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT));

            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT);
            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));

        }
        else if (msg->handle == pacsClient->pacsSupportedAudioContextCcdHandle)
        {
            pacsClient->notifyType = (pacsClient->notifyType &
                             (~GATT_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT));

            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT);
            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
        }
        else if (msg->handle == pacsClient->pacsSinkAudioLocationCcdHandle)
        {
            pacsClient->notifyType = (pacsClient->notifyType &
                                 (~GATT_PACS_NOTIFICATION_SINK_AUDIO_LOCATION));

            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_SINK_AUDIO_LOCATION);
            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
        }
        else if (msg->handle == pacsClient->pacsSourceAudioLocationCcdHandle)
        {
            pacsClient->notifyType = (pacsClient->notifyType &
                                 (~GATT_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION));

            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION);
            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
        }
        else
        {
            if (pacsClient->sinkPacRecordCount)
            {

                CsrBtPacRecordCharacElement* pacElem = (CsrBtPacRecordCharacElement*)
                                                             pacsClient->sinkPacRecordList.first;

                for (; pacElem; pacElem = pacElem->next)
                {
                    if (pacElem->pacRecordCccdHandle == msg->handle)
                    {
                        pacsClient->writeSinkCccdCount--;

                        if (pacsClient->writeSinkCccdCount == 0)
                        {
                            pacsClient->notifyType = (pacsClient->notifyType &
                                      (~GATT_PACS_NOTIFICATION_SINK_PAC_RECORD));

                            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_SINK_PAC_RECORD);
                            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
                        }

                        return;
                    }
                }
            }

            if (pacsClient->sourcePacRecordCount)
            {
                CsrBtPacRecordCharacElement* pacElem = (CsrBtPacRecordCharacElement*)
                                                          pacsClient->sourcePacRecordList.first;

                for (; pacElem; pacElem = pacElem->next)
                {
                    if (pacElem->pacRecordCccdHandle == msg->handle)
                    {
                        pacsClient->writeSourceCccdCount--;

                        if (pacsClient->writeSourceCccdCount == 0)
                        {
                            pacsClient->notifyType = (pacsClient->notifyType &
                                     (~GATT_PACS_NOTIFICATION_SOURCE_PAC_RECORD));

                            pacsClientNotificationFlagUpdate(pacsClient, GATT_PACS_NOTIFICATION_SOURCE_PAC_RECORD);
                            handleNotificationCfm(pacsClient, pacsGetStatus(msg->resultCode));
                        }
                        return;
                    }
                }
            }
        }
    }
}

static void pacsClientTrackNotificationsEnabled(GPacsC* pacsClient, bool enable)
{
    uint16 notify = 0;

    if (pacsClient->sinkPacRecordCount == 0)
    {
        pacsClient->notifyType = pacsClient->notifyType & ~GATT_PACS_NOTIFICATION_SINK_PAC_RECORD;
        pacsClient->notifyType = pacsClient->notifyType & ~GATT_PACS_NOTIFICATION_SINK_AUDIO_LOCATION;
    }
    if (pacsClient->sourcePacRecordCount == 0)
    {
        pacsClient->notifyType = pacsClient->notifyType & ~GATT_PACS_NOTIFICATION_SOURCE_PAC_RECORD;
        pacsClient->notifyType = pacsClient->notifyType & ~GATT_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION;
    }

    if (enable)
    {
        notify = pacsClient->notifyTypeEnabled & pacsClient->notifyType & GATT_PACS_NOTIFICATION_ALL;
        pacsClient->notifyType &= ~notify;
    }

}


static void handlePacsClientInternalMsg(PacsC *mainInst, void *msg)
{
    GPacsC *pacsClient = NULL;
    GattPacsInternalMsgId* prim = (GattPacsInternalMsgId*)msg;
    switch (*prim)
    {
        case PACS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
        {
            PacsClientInternalMsgNotificationReq *message =
                              (PacsClientInternalMsgNotificationReq *)msg;

            pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if (pacsClient)
            {
                pacsClient->notifyType = message->type;
                pacsClient->notifyEnable = message->enable;

                pacsClientTrackNotificationsEnabled(pacsClient, message->enable);
                handleNotificationCfm(pacsClient, GATT_PACS_CLIENT_STATUS_SUCCESS);
            }
            break;
        }
        default:
            break;
    }

    CSR_UNUSED(mainInst);
}


static void pacsMessageFree(uint8** value, uint16 valueLen)
{
    if (*value && valueLen)
    {
        CsrPmemFree(*value);
        *value = NULL;
    }
}

static void handlePacsClientGattMsg(GPacsC *pacsClient, CsrBtGattPrim id,void *msg)
{

    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {

            pacsGattDiscoverCharacCfmHandler(pacsClient,
                (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T*)msg);
            break;
        }

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            pacsGattDiscoverCharacDescCfmHandler(pacsClient,
                    (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T*)msg);
            break;
        }
        case CSR_BT_GATT_WRITE_CFM:
        {
            pacsGattWriteHandler(pacsClient,
                           (CsrBtGattWriteCfm*)msg);
            break;
        }
        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm* message = (CsrBtGattReadCfm*)msg;
            pacsGattReadHandler(pacsClient,message);
            pacsMessageFree(&message->value, message->valueLength);
            break;
        }
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            CsrBtGattClientNotificationInd* message = (CsrBtGattClientNotificationInd*)msg;

            pacsGattNotificationHandler(pacsClient,message);
            pacsMessageFree(&message->value, message->valueLength);

            break;
        }
        default:
            break;
    }
}

static void pacsInitializePacRecordHandles(GattPacsClientDeviceData* deviceData, bool sinkRecord)
{
    if (sinkRecord)
    {
        deviceData->pacsSinkPacRecordHandle = (uint16*)CsrPmemZalloc(deviceData->sinkPacRecordCount * sizeof(uint16));
        deviceData->pacsSinkPacRecordCcdHandle = (uint16*)CsrPmemZalloc(deviceData->sinkPacRecordCount * sizeof(uint16));
    }
    else
    {
        deviceData->pacsSourcePacRecordHandle = (uint16*)CsrPmemZalloc(deviceData->sourcePacRecordCount * sizeof(uint16));
        deviceData->pacsSourcePacRecordCcdHandle = (uint16*)CsrPmemZalloc(deviceData->sourcePacRecordCount * sizeof(uint16));
    }
}


static GattPacsClientStatus pacsClientFreeInstanceData(GPacsC* const pacsClient)
{
    ServiceHandle clientHandle = pacsClient->srvcElem->service_handle;
    GattPacsClientStatus status = GATT_PACS_CLIENT_STATUS_FAILED;
    CsrCmnListDeinit(&pacsClient->sourcePacRecordList);
    CsrCmnListDeinit(&pacsClient->sinkPacRecordList);

    if (FREE_PACS_CLIENT_INST(clientHandle))
    {
        status = GATT_PACS_CLIENT_STATUS_SUCCESS;
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
    }
    else
    {
        GATT_PACS_CLIENT_INFO("Pacs: Unable to free PACS client instance \n\n");
    }

    return status;
}

/******************************************************************************/

void pacsClientSendInitCfm(GPacsC* const pacsClient, CsrBtResultCode status)
{
    GattPacsClientStatus result;
    GattPacsClientInitCfm* cfm = (GattPacsClientInitCfm *)CsrPmemZalloc(sizeof(GattPacsClientInitCfm));
    AppTask appTask = pacsClient->app_task;

    cfm->id = GATT_PACS_CLIENT_INIT_CFM;
    cfm->status = pacsGetStatus(status);
    cfm->cid = pacsClient->srvcElem->cid;
    cfm->clientHandle = pacsClient->srvcElem->service_handle;
    
    /* If Pacs client init is succesful, enable notifications, else free the instance data and *
     * Remove handle from list */

    if (cfm->status == GATT_PACS_CLIENT_STATUS_SUCCESS)
    {
        pacsClient->internalReq = TRUE;
        GattPacsClientRegisterForNotification(cfm->clientHandle, GATT_PACS_NOTIFICATION_ALL, TRUE);
    }
    else
    {
        result = pacsClientFreeInstanceData(pacsClient);
        cfm->clientHandle = INVALID_SERVICE_HANDLE;
        GATT_PACS_CLIENT_INFO("(Pacs): pacsClientSendInitCfm : Unable to free PACS client instance: %d \n\n", result);
        CSR_UNUSED(result);
    }

    CsrSchedMessagePut(appTask, PACS_CLIENT_PRIM, cfm);
}


void pacsClientSendTerminateCfm(GPacsC *const pacsClient)
{
    GattPacsClientTerminateCfm *cfm =
            (GattPacsClientTerminateCfm*)CsrPmemZalloc(
                                     sizeof(GattPacsClientTerminateCfm));
    /* Response Handle */
    AppTask app_task = pacsClient->app_task;
    /* Client service handle */
    cfm->id = GATT_PACS_CLIENT_TERMINATE_CFM;

    cfm->cid = pacsClient->srvcElem->cid;
    cfm->deviceData.startHandle = pacsClient->startHandle;
    cfm->deviceData.endHandle = pacsClient->endHandle;

    cfm->deviceData.pacsSinkAudioLocationHandle =
                           pacsClient->pacsSinkAudioLocationHandle;
    cfm->deviceData.pacsSinkAudioLocationCcdHandle =
                           pacsClient->pacsSinkAudioLocationCcdHandle;

    cfm->deviceData.pacsSourceAudioLocationHandle =
                           pacsClient->pacsSourceAudioLocationHandle;
    cfm->deviceData.pacsSourceAudioLocationCcdHandle =
                           pacsClient->pacsSourceAudioLocationCcdHandle;

    cfm->deviceData.pacsAvailableAudioContextHandle =
                           pacsClient->pacsAvailableAudioContextHandle;
    cfm->deviceData.pacsAvailableAudioContextCcdHandle =
                             pacsClient->pacsAvailableAudioContextCcdHandle;

    cfm->deviceData.pacsSupportedAudioContextHandle =
                           pacsClient->pacsSupportedAudioContextHandle;
    cfm->deviceData.pacsSupportedAudioContextCcdHandle =
                           pacsClient->pacsSupportedAudioContextCcdHandle;

    cfm->deviceData.sinkPacRecordCount = pacsClient->sinkPacRecordCount;
    cfm->deviceData.sourcePacRecordCount = pacsClient->sourcePacRecordCount;

    /* Copy source and sink pac records*/
    if (pacsClient->sinkPacRecordCount)
    {
        uint8 index;
        CsrCmnListElm_t* elem = (CsrCmnListElm_t*)(&pacsClient->sinkPacRecordList)->first;

        pacsInitializePacRecordHandles(&(cfm->deviceData), TRUE);

        for (index = 0;elem;elem = elem->next, index++)
        {
            CsrBtPacRecordCharacElement* recElem = (CsrBtPacRecordCharacElement*)elem;
            cfm->deviceData.pacsSinkPacRecordHandle[index] = recElem->valueHandle;
            cfm->deviceData.pacsSinkPacRecordCcdHandle[index] = recElem->pacRecordCccdHandle;
        }
    }

    if (pacsClient->sourcePacRecordCount)
    {
        uint8 index;
        CsrCmnListElm_t* elem = (CsrCmnListElm_t*)(&pacsClient->sourcePacRecordList)->first;

        pacsInitializePacRecordHandles(&(cfm->deviceData), FALSE);
        for (index = 0;elem;elem = elem->next, index++)
        {
            CsrBtPacRecordCharacElement* recElem = (CsrBtPacRecordCharacElement*)elem;
            cfm->deviceData.pacsSourcePacRecordHandle[index] = recElem->valueHandle;
            cfm->deviceData.pacsSourcePacRecordCcdHandle[index] = recElem->pacRecordCccdHandle;
        }
    }

    cfm->status = pacsClientFreeInstanceData(pacsClient);
    GATT_PACS_CLIENT_INFO("(Pacs): pacsClientSendTerminateCfm : cfm->status :%d \n\n", cfm->status);

    CsrSchedMessagePut(app_task, PACS_CLIENT_PRIM, cfm);
}

/* Register to recieve notifications when PAC record on server changes or is updated */

void pacsClientRegisterForPacRecordNotification(ServiceHandle clientHandle,
                                          GattPacsClientType type,
                                          bool enable)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    pacsClient->notifyEnable = enable;
    GATT_PACS_CLIENT_INFO("Pacs:  Register for PAC record notification\n\n");

    switch(type)
    {
        case GATT_PACS_CLIENT_SINK :
        {
            CsrCmnListIterate(&pacsClient->sinkPacRecordList, writeSinkPacRecordCccd, (void*)pacsClient);
            break;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {
            CsrCmnListIterate(&pacsClient->sourcePacRecordList, writeSourcePacRecordCccd, (void*)pacsClient);
            break;
        }
        default:
        {
            /* invalid  type*/
            GATT_PACS_CLIENT_ERROR("INVALID type \n\n");
        }
    }

}

void pacsClientRegisterForAudioLocationNotification(ServiceHandle clientHandle,
                                              GattPacsClientType type,
                                              bool enable)
{
    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    GATT_PACS_CLIENT_DEBUG("Pacs:  Register for audio location notification\n\n");

    switch (type)
    {
        case GATT_PACS_CLIENT_SINK:
        {

            pacsClientWriteRequestSend(pacsClient->srvcElem->gattId,
                                     pacsClient->srvcElem->cid,
                                     pacsClient->pacsSinkAudioLocationCcdHandle,
                                     enable);
            break;
        }
        case GATT_PACS_CLIENT_SOURCE:
        {

            pacsClientWriteRequestSend(pacsClient->srvcElem->gattId,
                                      pacsClient->srvcElem->cid,
                                      pacsClient->pacsSourceAudioLocationCcdHandle,
                                      enable);
            break;
        }
        default:
        {
            /* invalid  context*/
            GATT_PACS_CLIENT_ERROR("INVALID audio context \n\n");
        }
    }
}

void pacsClientRegisterForAudioContextNotification(ServiceHandle clientHandle,
                                           GattPacsClientAudioContext context,
                                           bool enable)
{

    GPacsC *pacsClient = FIND_PACS_INST_BY_SERVICE_HANDLE(clientHandle);

    if (pacsClient == NULL)
    {
        /* invalid pacs client*/
        PACS_REMOVE_SERVICE_HANDLE(mainInst->clientHandleList, clientHandle);
        GATT_PACS_CLIENT_ERROR("INVALID client instance \n\n");
        return;
    }

    GATT_PACS_CLIENT_INFO("Pacs:  Register for audio context notification\n\n");

    switch (context)
    {
        case GATT_PACS_CLIENT_AVAILABLE:
        {
            pacsClientWriteRequestSend(pacsClient->srvcElem->gattId,
                                       pacsClient->srvcElem->cid,
                                       pacsClient->pacsAvailableAudioContextCcdHandle,
                                       enable);
            break;
        }
        case GATT_PACS_CLIENT_SUPPORTED:
        {
            pacsClientWriteRequestSend(pacsClient->srvcElem->gattId,
                                      pacsClient->srvcElem->cid,
                                      pacsClient->pacsSupportedAudioContextCcdHandle,
                                      enable);
            break;
        }
        default:
        {
            /* invalid  context*/
            GATT_PACS_CLIENT_ERROR("INVALID audio context \n\n");
        }
    }
}


void PacsClientMsgHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    PacsC* inst = *((PacsC**)gash);
    GPacsC* pacsClient;

    pacsClient = NULL;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim* id = (CsrBtGattPrim*)msg;
                void* message = NULL;
                pacsClient = (GPacsC*)GetServiceClientByGattMsg(&inst->clientHandleList, msg);
                message = GetGattManagerMsgFromGattMsg(msg, id);

                if (pacsClient)
                    handlePacsClientGattMsg(pacsClient,*id, message);

                if (msg != message)
                {
                    CsrPmemFree(message);
                    message = NULL;
                }

                break;
            }
            case PACS_CLIENT_PRIM:
                handlePacsClientInternalMsg(inst, msg);
                break;
            default:
                break;
        }
        SynergyMessageFree(eventClass, msg);
    }
}
