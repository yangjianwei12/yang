/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gatt_ascs_client_private.h"
#include "gatt_ascs_client_msg_handler.h"
#include "gatt_ascs_client_debug.h"
#include "gatt_ascs_client_util.h"
#include "gatt_ascs_client.h"
#include "csr_pmem.h"
#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "service_handle.h"
#include "csr_bt_core_stack_pmalloc.h"

/* Global ascs service instance */
extern AscsC *ascsClientMain;

CsrBool ascsFindCharacFromAseId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtAseCharacElement *conn = (CsrBtAseCharacElement *)elem;
    CsrBtAseId     aseId   = *(CsrBtAseId *) value;
    return ((conn->aseId == aseId) ? TRUE : FALSE);
}

CsrBool ascsFindCharacFromHandle(CsrCmnListElm_t *elem, void *value)
{
    CsrBtAseCharacElement *conn = (CsrBtAseCharacElement *)elem;
    CsrBtGattHandle valueHandle = *(CsrBtGattHandle *) value;
    return ((conn->valueHandle == valueHandle) ? TRUE : FALSE);
}

CsrBool ascsFindCharacFromCCCDHandle(CsrCmnListElm_t *elem, void *value)
{
    CsrBtAseCharacElement *conn = (CsrBtAseCharacElement *)elem;
    CsrBtGattHandle valueHandle = *(CsrBtGattHandle *) value;
    return ((conn->aseCccdHandle == valueHandle) ? TRUE : FALSE);
}

static void  ascsGattDiscoverRequest(GAscsC *const ascsClient)
{
    /* Start finding out the characteristics handles for pacs service */

    CsrBtGattDiscoverAllCharacOfAServiceReqSend(ascsClient->srvcElem->gattId,
                                                ascsClient->srvcElem->cid,
                                                ascsClient->startHandle,
                                                ascsClient->endHandle);

}

static void ascSetAseCharacElementValue(CsrBtAseCharacElement* elem, uint16 valueHandle, uint16 cccdHandle, uint8 aseId)
{
    elem->aseId = aseId;
    elem->valueHandle = valueHandle;
    elem->aseCccdHandle = cccdHandle;
    elem->declarationHandle = 0xFFFF;
    elem->endHandle = 0xFFFF;
}

bool GattAscsReadAseInfoReq(ServiceHandle clientHandle, AseCharType aseCharType)
{
    GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);

    if(ascsClient == NULL)
        return FALSE;

    if(((aseCharType == GATT_ASCS_CLIENT_ASE_SINK) && (ascsClient->aseSinkCharacCount == 0))||
        ((aseCharType == GATT_ASCS_CLIENT_ASE_SOURCE) && (ascsClient->aseSourceCharacCount == 0)))
    {
        return FALSE;
    }
    else
    {
        AscsClientInternalMsgReadAseInfoReq *req =
                CsrPmemZalloc(sizeof(AscsClientInternalMsgReadAseInfoReq));

        req->prim = ASCS_CLIENT_INTERNAL_MSG_READ_ASE_INFO_REQ;
        req->clientHandle = ascsClient->srvcElem->service_handle;

        if(aseCharType == GATT_ASCS_CLIENT_ASE_SINK)
            ascsClient->readSinkChar = TRUE;
        
        CsrSchedMessagePut(CSR_BT_ASCS_CLIENT_IFACEQUEUE, ASCS_CLIENT_PRIM, req);
    }
    return TRUE;
}

void GattAscsClientReadAseStateReq(ServiceHandle clientHandle, uint8 aseId,
        AseCharType aseCharType)
{
    GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);

    if(ascsClient == NULL)
    {
        GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
        return;
    }

    if(((aseCharType == GATT_ASCS_CLIENT_ASE_SINK) && (ascsClient->aseSinkCharacCount == 0))||
        ((aseCharType == GATT_ASCS_CLIENT_ASE_SOURCE) && (ascsClient->aseSourceCharacCount == 0)))
    {
        /* TODO send CFM failure */
        return;
    }
    else
    {
        AscsClientInternalMsgReadAseStateReq *req = CsrPmemZalloc
                    (sizeof(AscsClientInternalMsgReadAseStateReq));

        req->prim = ASCS_CLIENT_INTERNAL_MSG_READ_ASE_STATE_REQ;
        req->aseId = aseId;
        req->clientHandle = ascsClient->srvcElem->service_handle;

        CsrSchedMessagePut(CSR_BT_ASCS_CLIENT_IFACEQUEUE, ASCS_CLIENT_PRIM, req);
    }
}

void GattAscsRegisterForNotification(ServiceHandle clientHandle, CsrBtAseId aseId, CsrBool enable)
{
    GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);
    AscsClientInternalMsgNotificationReq *req = CsrPmemZalloc(sizeof(AscsClientInternalMsgNotificationReq));

    if(ascsClient == NULL)
    {
        GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
        return;
    }
    
    req->prim = ASCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ;
    req->clientHandle = ascsClient->srvcElem->service_handle;
    req->aseId = aseId;
    req->enable = enable;

    CsrSchedMessagePut(CSR_BT_ASCS_CLIENT_IFACEQUEUE, ASCS_CLIENT_PRIM, req);
}

void GattAscsWriteAsesControlPointReq(ServiceHandle clientHandle, uint16 sizeValue, uint8 *value)
{
    GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);
    AscsClientInternalMsgWriteAsesControlPointReq *req = CsrPmemZalloc(sizeof(AscsClientInternalMsgWriteAsesControlPointReq)+sizeValue);

    if(ascsClient == NULL)
    {
        GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
        return;
    }

    req->prim = ASCS_CLIENT_INTERNAL_MSG_WRITE_ASES_CONTROL_POINT_REQ;
    req->clientHandle = ascsClient->srvcElem->service_handle;
    req->sizeValue = sizeValue;
    if(value)
    {
        memmove(req->value, value, sizeValue);
        CsrPmemFree(value);
    }

    CsrSchedMessagePut(CSR_BT_ASCS_CLIENT_IFACEQUEUE, ASCS_CLIENT_PRIM, req);
}


/****************************************************************************/
void GattAscsClientInit(AppTask appTask ,
               const GattAscsClientInitParams *const initData,
               const GattAscsClientDeviceData *deviceData)
{
    ServiceHandle clientHandle = 0;
    GattAscsClientRegistrationParams regParams;
    GAscsC* ascsClient;
    uint8 index;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid task\n\n");
        return;
    }

    clientHandle = getAscsServiceHandle(&ascsClient, &(ascsClientMain->clientHandleList));
    CSR_UNUSED(clientHandle);

    GATT_ASCS_CLIENT_INFO("Ascs:  GattAscsClientInit\n\n");

    if(ascsClient == NULL)
    {
        GattAscsClientInitCfm* cfm = xzpnew(GattAscsClientInitCfm);
        cfm->cid = initData->cid;
        cfm->id = GATT_ASCS_CLIENT_INIT_CFM;
        cfm->clientHandle = 0;
        cfm->status = GATT_ASCS_CLIENT_STATUS_FAILED;

        GATT_ASCS_CLIENT_ERROR(" ASCS Client initialization failed \n\n");

        CsrSchedMessagePut(appTask, ASCS_CLIENT_PRIM, cfm);
        return;
    }

    /* Keep track on application task as all the notifications and indication need to be send to here */
    ascsClient->app_task = appTask;
    /* Store library task for external message reception */
    ascsClient->lib_task = CSR_BT_ASCS_CLIENT_IFACEQUEUE;  /* PACS client interface queue */

    ascsClient->startHandle = initData->startHandle;
    ascsClient->endHandle = initData->endHandle;

    ascsClient->srvcElem->cid = initData->cid;
    ascsClient->startHandle = initData->startHandle;
    ascsClient->endHandle = initData->endHandle;

    ascsClient->readCount = 0;
    ascsClient->readAseInfo = FALSE;
    ascsClient->writeCount = 0xFF;
    ascsClient->readSinkChar = FALSE;

    ascsClient->writeType = GATT_WRITE;

    CsrCmnListInit(&ascsClient->asesSinkCharacList, 0, InitAscsAseCharcList, NULL);
    ascsClient->aseSinkCharacCount = 0;

    CsrCmnListInit(&ascsClient->asesSourceCharacList, 0, InitAscsAseCharcList, NULL);
    ascsClient->aseSourceCharacCount = 0;

    regParams.cid = initData->cid;
    regParams.startHandle = initData->startHandle;
    regParams.endHandle = initData->endHandle;

    /* If the device is already known, get the persistent data */
    if (deviceData)
    {

         GATT_ASCS_CLIENT_ERROR("BAP: GattAscsClientInit failed conn_id %d", initData->cid);
        ascsClient->asesAseControlPointHandle = deviceData->asesAseControlPointHandle;
        ascsClient->asesAseControlPointCcdHandle = deviceData->asesAseControlPointCcdHandle;
        ascsClient->aseSinkCharacCount = deviceData->sinkAseCount;
        ascsClient->aseSourceCharacCount = deviceData->sourceAseCount;

      
        for (index = 0; index < deviceData->sinkAseCount; index++)
        {
            CsrBtAseCharacElement* elem = (CsrBtAseCharacElement*)ASE_ADD_PRIM_CHARAC(ascsClient->asesSinkCharacList);
            ascSetAseCharacElementValue(elem, deviceData->sinkAseHandle[index],
                              deviceData->sinkAseCcdHandle[index], deviceData->sinkAseId[index]);
        }


        for (index = 0; index < deviceData->sourceAseCount; index++)
        {
            CsrBtAseCharacElement* elem = (CsrBtAseCharacElement*)ASE_ADD_PRIM_CHARAC(ascsClient->asesSourceCharacList);
            ascSetAseCharacElementValue(elem, deviceData->sourceAseHandle[index],
                           deviceData->sourceAseCcdHandle[index], deviceData->sourceAseId[index]);
        }

    }

    if (GattRegisterAscsClient(&regParams, ascsClient))
    {
        if (!deviceData)
        {
            ascsGattDiscoverRequest(ascsClient);
        }
        else
        {
            ascsClientSendInitCfm(ascsClient, GATT_ASCS_CLIENT_STATUS_SUCCESS);
        }
    }
    else
    {
        GATT_ASCS_CLIENT_ERROR("Register with the GATT failed!\n");
        ascsClientSendInitCfm(ascsClient, GATT_ASCS_CLIENT_STATUS_FAILED);
    }

}

void GattAscsClientTerminate(ServiceHandle clientHandle)
{
    if(clientHandle)
    {
        GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);
        if (ascsClient == NULL)
        {
            GATT_ASCS_CLIENT_ERROR("ASCS:  invalid client Instance\n\n");
            ASCS_REMOVE_SERVICE_HANDLE(ascsClientMain->clientHandleList, clientHandle);
        }
        else
        {
            /* Unregister with the GATT Manager and verify the result */
            CsrBtGattUnregisterReqSend(ascsClient->srvcElem->gattId);

            ascsClientSendTerminateCfm(ascsClient, GATT_ASCS_CLIENT_STATUS_SUCCESS);
        }
    }
}

void GattAscsSetAsesControlPointReq(ServiceHandle clientHandle , bool response)
{
    GAscsC *ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(clientHandle);
    
    if(ascsClient)
    {
        if( response == TRUE )
            ascsClient->writeType = GATT_WRITE;
        else
            ascsClient->writeType = GATT_WRITE_WITHOUT_RESPONSE;
    }
}

