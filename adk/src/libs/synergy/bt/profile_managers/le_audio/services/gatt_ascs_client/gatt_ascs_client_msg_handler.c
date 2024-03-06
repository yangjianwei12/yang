/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csr_bt_gatt_lib.h"

#include "gatt_ascs_client_private.h"
#include "gatt_ascs_client_util.h"
#include "gatt_ascs_client_debug.h"
#include "gatt_ascs_client_msg_handler.h"

#include"csr_bt_core_stack_pmalloc.h"
#include "csr_list.h"

extern AscsC* ascsClientMain;

static void ascsClientDiscoverAllCharacteristicDescriptors(GAscsC *gattAscsClient);

static void readAseInfo(CsrCmnListElm_t *elem, void *data)
{
    CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *)elem;
    GAscsC *ascsClient =  (GAscsC *)data;

    ascsClient->readCount++;
    CsrBtGattReadReqSend(ascsClient->srvcElem->gattId,
                         ascsClient->srvcElem->cid,
                         aseCharac->valueHandle,
                         0);
}

static void ascsSetDeviceDataElement(CsrBtAseCharacElement* elem, uint16* valueHandle, uint16* ccdHandle, uint8* aseId)
{
    *valueHandle = elem->valueHandle;
    *ccdHandle = elem->aseCccdHandle;
    *aseId = elem->aseId;
}

static void ascsInitializeDeviceData(GattAscsClientDeviceData* deviceData)
{
    if (deviceData->sinkAseCount != 0)
    {
        deviceData->sinkAseId = (uint8*)CsrPmemZalloc(deviceData->sinkAseCount * sizeof(uint8));
        deviceData->sinkAseCcdHandle = (uint16*)CsrPmemZalloc(deviceData->sinkAseCount * sizeof(uint16));
        deviceData->sinkAseHandle = (uint16*)CsrPmemZalloc(deviceData->sinkAseCount * sizeof(uint16));
    }

    if (deviceData->sourceAseCount != 0)
    {
        deviceData->sourceAseId = (uint8*)CsrPmemZalloc(deviceData->sourceAseCount * sizeof(uint8));
        deviceData->sourceAseCcdHandle = (uint16*)CsrPmemZalloc(deviceData->sourceAseCount * sizeof(uint16));
        deviceData->sourceAseHandle = (uint16*)CsrPmemZalloc(deviceData->sourceAseCount * sizeof(uint16));
    }
}

static void ascsReadReqHandler(GAscsC *ascsClient, AscsClientInternalMsgReadAseInfoReq *req)
{
    if(ascsClient->readSinkChar == TRUE)
    {
        CsrCmnListIterate(&ascsClient->asesSinkCharacList,
                      readAseInfo,
                      ascsClient);
    }
    else
    {
        CsrCmnListIterate(&ascsClient->asesSourceCharacList,
                      readAseInfo,
                      ascsClient);
    }

    ascsClient->readAseInfo = TRUE;
    CSR_UNUSED(req);
}

static void ascsReadStateReqHandler(GAscsC *ascsClient, AscsClientInternalMsgReadAseStateReq *req)
{
    CsrCmnListElm_t *elem = (&ascsClient->asesSinkCharacList)->first;

    if (req->aseId == ASE_ID_ALL)
    {
        CsrBtGattHandle *handles = CsrPmemAlloc(sizeof(CsrBtGattHandle) * (ascsClient->aseSinkCharacCount));
        uint8 i;

        for (i = 0; elem; elem = elem->next, i++)
        {
            CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *) elem;
            handles[i] = aseCharac->valueHandle;
        }

        CsrBtGattReadMultiReqSend(ascsClient->srvcElem->gattId,
                                  ascsClient->srvcElem->cid,
                                  ascsClient->aseSinkCharacCount,
                                  handles);
    }
    else
    {
        CsrBtAseCharacElement *aseCharac = ASCS_FIND_PRIM_CHARAC_BY_ASEID(ascsClient->asesSinkCharacList,
                                                                                 &req->aseId);

        if((aseCharac == NULL) && (ascsClient->aseSourceCharacCount))
        {
            aseCharac = ASCS_FIND_PRIM_CHARAC_BY_ASEID(ascsClient->asesSourceCharacList,
                                                                   &req->aseId);
        }
        else
        {
            GATT_ASCS_CLIENT_ERROR("ascsReadStateReqHandler No ASE Handle\n");
            return;
        }

        CsrBtGattReadReqSend(ascsClient->srvcElem->gattId,
                             ascsClient->srvcElem->cid,
                             aseCharac->valueHandle,
                             0);
    }
}

static void  enableCccd(GAscsC *ascsClient, CsrBool enable)
{
    CsrCmnListElm_t *elem = NULL;
    uint8 *value = (uint8 *)CsrPmemAlloc(CSR_BT_WRITE_CCCD_SIZE*sizeof(CsrUint8));

    value[0] = enable ? GATT_NOTIFICATION_ENABLE_VALUE : 0;
    value[1] = 0;

    ascsClient->writeCount = 0;

    if(ascsClient->aseSinkCharacCount)
    {
        elem = (&ascsClient->asesSinkCharacList)->first;

        for (; elem; elem = elem->next)
        {
            CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *)elem;
            uint8 *val = (uint8 *)CsrPmemAlloc(CSR_BT_WRITE_CCCD_SIZE*sizeof(CsrUint8));

            val[0] = enable ? GATT_NOTIFICATION_ENABLE_VALUE : 0;
            val[1] = 0;

            if(aseCharac->aseCccdHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
            {
                ascsClient->writeCount++;
                CsrBtGattWriteReqSend(ascsClient->srvcElem->gattId,
                                      ascsClient->srvcElem->cid,
                                      aseCharac->aseCccdHandle,
                                      0,
                                      CSR_BT_WRITE_CCCD_SIZE,
                                      val);
            }
        }
    }
    
    if(ascsClient->aseSourceCharacCount)
    {
        elem = (&ascsClient->asesSourceCharacList)->first;

        for (; elem; elem = elem->next)
        {
            CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *)elem;
            uint8 *val = (uint8 *)CsrPmemAlloc(CSR_BT_WRITE_CCCD_SIZE*sizeof(CsrUint8));

            val[0] = enable ? GATT_NOTIFICATION_ENABLE_VALUE : 0;
            val[1] = 0;

            if(aseCharac->aseCccdHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
            {
                ascsClient->writeCount++;
                CsrBtGattWriteReqSend(ascsClient->srvcElem->gattId,
                                      ascsClient->srvcElem->cid,
                                      aseCharac->aseCccdHandle,
                                      0,
                                      CSR_BT_WRITE_CCCD_SIZE,
                                      val);
            }
        }
    }

    CsrBtGattWriteReqSend(ascsClient->srvcElem->gattId,
                          ascsClient->srvcElem->cid,
                          ascsClient->asesAseControlPointCcdHandle,
                          0,
                          CSR_BT_WRITE_CCCD_SIZE,
                          value);
}

static void  writeAseCccd(GAscsC *ascsClient,
                                CsrBtAseId aseId,
                                bool  enable)
{
    CsrBtAseCharacElement *aseCharac = NULL;
    ascsClient->writeCount = 0;

    if(ascsClient->aseSinkCharacCount)
    {
        aseCharac = ASCS_FIND_PRIM_CHARAC_BY_ASEID(ascsClient->asesSinkCharacList,
                                                                             &aseId);
        if(( aseCharac == NULL) && (ascsClient->aseSourceCharacCount))
        {
            aseCharac = ASCS_FIND_PRIM_CHARAC_BY_ASEID(ascsClient->asesSourceCharacList,
                                                                &aseId);
        }
    }
    else if( ascsClient->aseSourceCharacCount)
    {
        aseCharac = ASCS_FIND_PRIM_CHARAC_BY_ASEID(ascsClient->asesSourceCharacList,
                                                                &aseId);
    }

    if(aseCharac == NULL)
    {
        /* TODO send CFM */
        return;
    }

    if(aseCharac->aseCccdHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
    {
        uint8 *value = (uint8 *) CsrPmemAlloc(CSR_BT_WRITE_CCCD_SIZE*sizeof(uint8));

        value[0] = enable ? GATT_NOTIFICATION_ENABLE_VALUE : 0;
        value[1] = 0;

        ascsClient->writeCount++;

        CsrBtGattWriteReqSend(ascsClient->srvcElem->gattId,
                              ascsClient->srvcElem->cid,
                              aseCharac->aseCccdHandle,
                              0,
                              CSR_BT_WRITE_CCCD_SIZE,
                              value);
    }
}


/****************************************************************************/
static void ascsClientHandleDiscoverAllAscsCharacteristicsResp(GAscsC *gattAscsClient,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_ASCS_CLIENT_DEBUG("DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == CSR_BT_GATT_UUID_SINK_ASE)
            {
                CsrBtAseCharacElement *elem =
                                ASE_ADD_PRIM_CHARAC(gattAscsClient->asesSinkCharacList);
                elem->declarationHandle = cfm->declaration;
                elem->valueHandle = cfm->handle;
                elem->aseCccdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                gattAscsClient->aseSinkCharacCount++;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_SOURCE_ASE)
            {
                CsrBtAseCharacElement *elem =
                                ASE_ADD_PRIM_CHARAC(gattAscsClient->asesSourceCharacList);
                elem->declarationHandle = cfm->declaration;
                elem->valueHandle = cfm->handle;
                elem->aseCccdHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                gattAscsClient->aseSourceCharacCount++;
                
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_ASCS_CONTROL_POINT)
            {
                gattAscsClient->asesAseControlPointHandle = cfm->handle;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!(gattAscsClient->aseSinkCharacCount  || gattAscsClient->aseSourceCharacCount) ||
                !gattAscsClient->asesAseControlPointHandle)
            {
                /* One of the ASCS characteristic is not found, initialisation complete */
                ascsClientSendInitCfm(gattAscsClient, GATT_ASCS_CLIENT_STATUS_DISCOVERY_ERR);
            }
            else
            {
                /* All ASCS characteristics found, find the descriptors */
                ascsClientDiscoverAllCharacteristicDescriptors(gattAscsClient);
            }
        }
    }
    else
    {
        ascsClientSendInitCfm(gattAscsClient, GATT_ASCS_CLIENT_STATUS_DISCOVERY_ERR);
    }
}

/****************************************************************************/
static void ascsClientDiscoverAllCharacteristicDescriptors(GAscsC *gattAscsClient)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gattAscsClient->srvcElem->gattId,
                                                 gattAscsClient->srvcElem->cid,
                                                 gattAscsClient->startHandle + 1,
                                                 gattAscsClient->endHandle);
}

/****************************************************************************/
static void ascsClientHandleDiscoverAllCharacteristicDescriptorsResp(GAscsC *gattAscsClient,
                                     const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_ASCS_CLIENT_DEBUG("DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                           cfm->status,
                           cfm->handle,
                           cfm->uuid[0],
                           cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            if (cfm->uuid[0] == CSR_BT_GATT_UUID_SINK_ASE)
            {
                gattAscsClient->pendingHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_SOURCE_ASE)
            {
                gattAscsClient->pendingHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_ASCS_CONTROL_POINT)
            {
                gattAscsClient->pendingHandle = cfm->handle;
            }
            else if (cfm->uuid[0] == CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC)
            {
                if (gattAscsClient->pendingHandle == gattAscsClient->asesAseControlPointHandle)
                {
                    gattAscsClient->asesAseControlPointCcdHandle = cfm->handle;
                    gattAscsClient->pendingHandle = 0;
                }
                else 
                {
                    if(gattAscsClient->aseSinkCharacCount)
                    {
                        CsrCmnListElm_t *elem = (&gattAscsClient->asesSinkCharacList)->first;
                        for (; elem; elem = elem->next)
                        {
                            CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *)elem;

                            if(gattAscsClient->pendingHandle == aseCharac->valueHandle)
                            {
                                aseCharac->aseCccdHandle = cfm->handle;
                                break;
                            }
                        }
                    }
                    if(gattAscsClient->aseSourceCharacCount)
                    {
                        CsrCmnListElm_t *elem = (&gattAscsClient->asesSourceCharacList)->first;
                        for (; elem; elem = elem->next)
                        {
                            CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *)elem;

                            if(gattAscsClient->pendingHandle == aseCharac->valueHandle)
                            {
                                aseCharac->aseCccdHandle = cfm->handle;
                                break;
                            }
                        }
                    }
                    gattAscsClient->pendingHandle = 0;
                }
            }
        }
    }

    if (!cfm->more_to_come)
    {
        if (!gattAscsClient->asesAseControlPointCcdHandle)
        {
            ascsClientSendInitCfm(gattAscsClient, GATT_ASCS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            /* Enable all CCCD's for SINK and Source ASE's */
            enableCccd(gattAscsClient, TRUE);
            ascsClientSendInitCfm(gattAscsClient, GATT_ASCS_CLIENT_STATUS_SUCCESS);
        }
    }
}

static void  ascsGattReadHandler(GAscsC *ascsClient,
                                      CsrBtGattReadCfm *cfm)
{
    if(ascsClient->readAseInfo == TRUE)
    {
        CsrBtAseCharacElement *aseCharacElem = NULL;
        
        if( ascsClient->readSinkChar && ascsClient->aseSinkCharacCount)
        {
            aseCharacElem = ASCS_FIND_PRIM_CHARAC_BY_HANDLE(ascsClient->asesSinkCharacList,
                                                                                        &(cfm->handle));
        }
        else if(ascsClient->aseSourceCharacCount)
        {
            aseCharacElem = ASCS_FIND_PRIM_CHARAC_BY_HANDLE(ascsClient->asesSourceCharacList,
                                                                                        &(cfm->handle));
        }
        if (aseCharacElem && cfm->valueLength && cfm->value)
        {
            aseCharacElem->aseId = cfm->value[0];
            ascsClient->readCount--;
        }

        if (ascsClient->readCount == 0)
        {
            uint8 i;
            uint8 *data = NULL;
            GattAscsClientReadAseInfoCfm *readCfm = NULL;
            uint8 noOfAse = 0;
            CsrCmnListElm_t *elem = NULL;

            readCfm = CsrPmemZalloc(sizeof(GattAscsClientReadAseInfoCfm));

            if( ascsClient->readSinkChar && ascsClient->aseSinkCharacCount)
            {
                elem = (&ascsClient->asesSinkCharacList)->first;
                noOfAse = ascsClient->aseSinkCharacCount;
                readCfm->charType = GATT_ASCS_CLIENT_ASE_SINK;
            }
            else if( ascsClient->aseSourceCharacCount )
            {
                elem = (&ascsClient->asesSourceCharacList)->first;
                noOfAse = ascsClient->aseSourceCharacCount;
                readCfm->charType = GATT_ASCS_CLIENT_ASE_SOURCE;
            }

            data = (uint8 *) CsrPmemZalloc(noOfAse * sizeof(uint8));

            for (i = 0; elem && i < noOfAse; elem = elem->next, i++)
            {
                CsrBtAseCharacElement *aseCharac = (CsrBtAseCharacElement *) elem;
                data[i] = aseCharac->aseId;
            }

            readCfm->id = GATT_ASCS_CLIENT_READ_ASE_INFO_CFM;
            readCfm->clientHandle = ascsClient->srvcElem->service_handle;
            readCfm->status = cfm->resultCode;
            readCfm->noOfAse = noOfAse;

            readCfm->aseId = CsrPmemZalloc(sizeof(uint8_t)*noOfAse);
            CsrMemCpy(readCfm->aseId, data, noOfAse);

            if (data)
                CsrPmemFree(data);

            ascsClient->readAseInfo = FALSE;
            CsrSchedMessagePut(ascsClient->app_task,
                               ASCS_CLIENT_PRIM,
                               readCfm);
            if(ascsClient->readSinkChar == TRUE)
                ascsClient->readSinkChar = FALSE;
        }
    }
    else
    {
        GattAscsClientReadAseStateCfm *readCfm = CsrPmemZalloc(sizeof(GattAscsClientReadAseStateCfm));

        readCfm->id = GATT_ASCS_CLIENT_READ_ASE_STATE_CFM;
        readCfm->clientHandle = ascsClient->srvcElem->service_handle;
        readCfm->status = cfm->resultCode;
        /* If status is success, then fill in the data */
        if ((cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
            && (cfm->valueLength))
        {
            readCfm->sizeValue = cfm->valueLength;
            /* Copy the Published Audio Codec/location data */
            readCfm->value = CsrPmemZalloc(sizeof(uint8_t)*readCfm->sizeValue);
            CsrMemCpy(readCfm->value, cfm->value, cfm->valueLength);
        }

        CsrSchedMessagePut(ascsClient->app_task, ASCS_CLIENT_PRIM, readCfm);
    }

}

static void  ascsGattReadStateHandler(GAscsC *ascsClient,
                                           CsrBtGattReadMultiCfm *cfm)
{
    GattAscsClientReadAseStateCfm *readCfm = CsrPmemZalloc(sizeof(GattAscsClientReadAseStateCfm));

    readCfm->id = GATT_ASCS_CLIENT_READ_ASE_STATE_CFM;
    readCfm->clientHandle = ascsClient->srvcElem->service_handle;
    readCfm->status = cfm->resultCode;
    /* If status is success, then fill in the data */
    if ((cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
        && (cfm->valueLength))
    {
        readCfm->sizeValue = cfm->valueLength;
        /* Copy the Published Audio Codec/location data */
        CsrMemCpy(readCfm->value, cfm->value, cfm->valueLength);
    }

    CsrSchedMessagePut(ascsClient->app_task, ASCS_CLIENT_PRIM, readCfm);
}


static void  ascsGattWriteHandler(GAscsC *ascsClient,
                                       CsrBtGattWriteCfm *cfm)
{
    if(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        CsrBtAseCharacElement *aseCharac = ASCS_FIND_PRIM_CHARAC_BY_CCD_HANDLE(ascsClient->asesSinkCharacList,
                                                                             &cfm->handle);
        if(aseCharac)
        {
            if( ascsClient->writeCount != 0xFF)
                ascsClient->writeCount--;

            if(ascsClient->writeCount == 0)
            {
                GattAscsClientWriteAseCfm *writeCfm = CsrPmemZalloc(sizeof(GattAscsClientWriteAseCfm));

                writeCfm->id = GATT_ASCS_CLIENT_WRITE_ASE_CFM;
                writeCfm->clientHandle = ascsClient->srvcElem->service_handle;
                writeCfm->status = cfm->resultCode;
                writeCfm->aseId = aseCharac->aseId;

                CsrSchedMessagePut(ascsClient->app_task, ASCS_CLIENT_PRIM, writeCfm);
                ascsClient->writeCount = 0xFF;
            }
        }
    }
}

static void  ascsGattNotificationHandler(GAscsC *ascsClient,
                                              CsrBtGattClientIndicationInd *ind)
{
    /* Allocate and fill indication message to be sent to app */
    GattAscsClientIndicationInd *notiInd =  zpmalloc(sizeof(GattAscsClientIndicationInd)+ind->valueLength);

    notiInd->id = GATT_ASCS_CLIENT_INDICATION_IND;
    notiInd->clientHandle = ascsClient->srvcElem->service_handle;

    GATT_ASCS_CLIENT_DEBUG("(ASCS_CLIENT) ASE control point Notification handler\n\n");

    if(ascsClient->asesAseControlPointHandle == ind->valueHandle)
    {
        notiInd->aseId = 0;
    }
    else
    {
        CsrBtAseCharacElement *aseCharacElem = NULL;
        if( ascsClient->aseSinkCharacCount )
        {
            aseCharacElem = ASCS_FIND_PRIM_CHARAC_BY_HANDLE(ascsClient->asesSinkCharacList,
                                                          &(ind->valueHandle));
        }

        if((aseCharacElem == NULL) && ascsClient->aseSourceCharacCount)
        {
                aseCharacElem = ASCS_FIND_PRIM_CHARAC_BY_HANDLE(ascsClient->asesSourceCharacList,
                                                         &(ind->valueHandle));
        }
        if(aseCharacElem)
        notiInd->aseId = aseCharacElem->aseId;
    }

    /* If status is success, then fill in the data */
    if (ind->valueLength)
    {
        notiInd->sizeValue = ind->valueLength;
        notiInd->value = (uint8 *) CsrPmemZalloc(sizeof(uint8) * notiInd->sizeValue);
        /* Copy the Published Audio Codec/location data */
        memcpy(notiInd->value, ind->value, ind->valueLength);
    }

    CsrSchedMessagePut(ascsClient->app_task, ASCS_CLIENT_PRIM, notiInd);
}

static GattAscsClientStatus ascsClientFreeInstanceData(GAscsC* const ascsClient)
{
    GattAscsClientStatus status = GATT_ASCS_CLIENT_STATUS_FAILED;
    ServiceHandle clientHandle = ascsClient->srvcElem->service_handle;

    CsrCmnListDeinit(&ascsClient->asesSinkCharacList);
    ascsClient->aseSinkCharacCount = 0;
    CsrCmnListDeinit(&ascsClient->asesSourceCharacList);
    ascsClient->aseSourceCharacCount = 0;

    if (FREE_ASCS_CLIENT_INST(clientHandle))
    {
        status = GATT_ASCS_CLIENT_STATUS_SUCCESS;
        ASCS_REMOVE_SERVICE_HANDLE(ascsClientMain->clientHandleList, clientHandle);
    }
    else
    {
        GATT_ASCS_CLIENT_INFO("Pacs: Unable to free PACS client instance \n\n");
    }

    return status;
}


/***************************************************************************
NAME
    GattAscsClientStatus

DESCRIPTION
   Utility function to map gatt status to gatt ascs status
*/
static GattAscsClientStatus ascsGetStatus(CsrBtResultCode status)
{
    GattAscsClientStatus ascsStatus;
    
    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
        case CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE:
            ascsStatus = GATT_ASCS_CLIENT_STATUS_SUCCESS;
            break;

        case CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID:
            ascsStatus = GATT_ASCS_CLIENT_STATUS_NO_CONNECTION;
            break;

        case CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED:
            ascsStatus = GATT_ASCS_CLIENT_STATUS_NOT_ALLOWED;
            break;

        default:
            ascsStatus = GATT_ASCS_CLIENT_STATUS_FAILED;
            break;
    }
    
    return ascsStatus;
}


void ascsClientSendInitCfm(GAscsC *const ascsClient,
                                    CsrBtResultCode status)
{
    AppTask appTask = ascsClient->app_task;
    GattAscsClientStatus result;
    GattAscsClientInitCfm *cfm = CsrPmemZalloc(sizeof(GattAscsClientInitCfm));
    GATT_ASCS_CLIENT_INFO("(ASCS_CLIENT) ascsClientSendInitCfm status 0x%x\n\n",status);
    cfm->id = GATT_ASCS_CLIENT_INIT_CFM;
    cfm->cid = ascsClient->srvcElem->cid;
    cfm->clientHandle = ascsClient->srvcElem->service_handle;
    cfm->status = ascsGetStatus(status);

    if (cfm->status != GATT_ASCS_CLIENT_STATUS_SUCCESS)
    {
        result = ascsClientFreeInstanceData(ascsClient);
        cfm->clientHandle = INVALID_ASCS_SERVICE_HANDLE;
        GATT_ASCS_CLIENT_INFO("(ASCS_CLIENT) Ascs Instance Free status 0x%x\n\n", result);
        CSR_UNUSED(result);
    }

    CsrSchedMessagePut(appTask, ASCS_CLIENT_PRIM, cfm);
}

void ascsClientSendTerminateCfm(GAscsC *const ascsClient, GattAscsClientStatus status)
{
    uint8 index;
    CsrCmnListElm_t* elem = NULL;
    GattAscsClientTerminateCfm *cfm =
            (GattAscsClientTerminateCfm *) CsrPmemZalloc(
                                     sizeof(GattAscsClientTerminateCfm));
    AppTask appTask = ascsClient->app_task;

    cfm->id = GATT_ASCS_CLIENT_TERMINATE_CFM;
    cfm->status = status;
    cfm->clientHandle = ascsClient->srvcElem->service_handle;

    /* Update ASE count and control point handles */
    cfm->deviceData.sinkAseCount = ascsClient->aseSinkCharacCount;
    cfm->deviceData.sourceAseCount = ascsClient->aseSourceCharacCount;
    cfm->deviceData.asesAseControlPointHandle = ascsClient->asesAseControlPointHandle;
    cfm->deviceData.asesAseControlPointCcdHandle = ascsClient->asesAseControlPointCcdHandle;

    ascsInitializeDeviceData(&(cfm->deviceData));

    elem = (CsrCmnListElm_t*)(ascsClient->asesSinkCharacList.first);

    for (index = 0; (elem && cfm->deviceData.sinkAseCount); index++, elem = elem->next)
    {
        CsrBtAseCharacElement* aseElem = (CsrBtAseCharacElement*)elem;
        ascsSetDeviceDataElement(aseElem, &cfm->deviceData.sinkAseHandle[index],
                       &cfm->deviceData.sinkAseCcdHandle[index], &cfm->deviceData.sinkAseId[index]);
    }

    elem = (CsrCmnListElm_t*)(ascsClient->asesSourceCharacList.first);

    for (index = 0; (elem && cfm->deviceData.sourceAseCount); index++, elem = elem->next)
    {
        CsrBtAseCharacElement* aseElem = (CsrBtAseCharacElement*)elem;
        ascsSetDeviceDataElement(aseElem, &cfm->deviceData.sourceAseHandle[index],
            &cfm->deviceData.sourceAseCcdHandle[index], &cfm->deviceData.sourceAseId[index]);
    }


    /* Free CAP instance characteristics list */
    cfm->status = ascsClientFreeInstanceData(ascsClient);

    CsrSchedMessagePut(appTask, ASCS_CLIENT_PRIM, cfm);
}


static void ascsMessageFree(uint8 **value, uint16 valueLen )
{
    if (*value && valueLen)
    {
        CsrPmemFree(*value);
        *value = NULL;
    }
}

static void ascsClientHandleGattMsg(void *task, MsgId id, Msg msg)
{
    GAscsC *gattClient = (GAscsC *)task;

    switch(id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
            {
                ascsClientHandleDiscoverAllAscsCharacteristicsResp(gattClient,
                                                  (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
            }
            break;
        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
            {
                ascsClientHandleDiscoverAllCharacteristicDescriptorsResp(gattClient,
                                      (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
            }
            break;
        case CSR_BT_GATT_WRITE_CFM :
            {
                CsrBtGattWriteCfm* message = (CsrBtGattWriteCfm *) msg;

                ascsGattWriteHandler(gattClient, message);
            }
            break;
        case CSR_BT_GATT_READ_CFM:
            {
                CsrBtGattReadCfm* message = (CsrBtGattReadCfm *) msg;

                ascsGattReadHandler(gattClient, message);
                ascsMessageFree(&message->value, message->valueLength);
            }
            break;
        case CSR_BT_GATT_READ_MULTI_CFM:
            {
                CsrBtGattReadMultiCfm* message = (CsrBtGattReadMultiCfm *) msg;

                ascsGattReadStateHandler(gattClient, message);
                ascsMessageFree(&message->value, message->valueLength);

            }
            break;
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
            {
                CsrBtGattClientIndicationInd* message = (CsrBtGattClientIndicationInd *) msg;

                ascsGattNotificationHandler(gattClient, message);
                ascsMessageFree(&message->value, message->valueLength);

            }
            break;
        default:
            break;
    }

}

static void handleAscsClientMsg(AscsC *main_inst, void *msg)
{
    GAscsC *ascsClient = NULL;
    AscsClientInternalMsg *prim = (AscsClientInternalMsg *)msg;

    switch(*prim)
    {
        case ASCS_CLIENT_INTERNAL_MSG_DISCOVER:
        {
            /* Start by discovering Characteristic handles */
            AscsClientInternalMsgDiscover *message = (AscsClientInternalMsgDiscover *)msg;

            ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if(ascsClient == NULL)
            {
                GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
                return;
            }

            /* Start finding out the characteristics handles for ascs service */
            CsrBtGattDiscoverAllCharacOfAServiceReqSend(ascsClient->srvcElem->gattId,
                                                        message->clientHandle,
                                                        message->startHandle,
                                                        message->endHandle);
        }
        break;

        case ASCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
        {
            AscsClientInternalMsgNotificationReq *message = (AscsClientInternalMsgNotificationReq*)msg;

            ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if(ascsClient == NULL)
            {
                GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
                return;
            }

            ascsClient->writeCount = 0;

            if(message->aseId == ASE_ID_ALL)
            {
                ascsClient->writeCount = 0;
                enableCccd(ascsClient, message->enable);
            }
            else
            {
                writeAseCccd(ascsClient, message->aseId, message->enable);
            }
        }
        break;  

        case ASCS_CLIENT_INTERNAL_MSG_READ_ASE_INFO_REQ:
        {
            AscsClientInternalMsgReadAseInfoReq *message = (AscsClientInternalMsgReadAseInfoReq *)msg;

            ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if(ascsClient == NULL)
            {
                GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
                return;
            }

            ascsReadReqHandler(ascsClient, message);
        }
            break;

        case ASCS_CLIENT_INTERNAL_MSG_READ_ASE_STATE_REQ:
        {
            AscsClientInternalMsgReadAseStateReq *message = (AscsClientInternalMsgReadAseStateReq *)msg;

            ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if(ascsClient == NULL)
            {
                GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
                return;
            }

            ascsReadStateReqHandler(ascsClient, message);
        }
            break;
        case ASCS_CLIENT_INTERNAL_MSG_WRITE_ASES_CONTROL_POINT_REQ:
        {
            AscsClientInternalMsgWriteAsesControlPointReq *message = (AscsClientInternalMsgWriteAsesControlPointReq *)msg;
            uint8 *value = NULL;

            ascsClient = FIND_ASCS_INST_BY_SERVICE_HANDLE(message->clientHandle);

            if(ascsClient == NULL)
            {
                GATT_ASCS_CLIENT_PANIC("Ascs:  Invalid ascsClient\n\n");
                return;
            }

            value = (uint8 *) CsrPmemAlloc(sizeof(uint8)*(message->sizeValue));

            CsrMemCpy(value, &(message->value[0]), message->sizeValue);


            if( ascsClient->writeType == GATT_WRITE_WITHOUT_RESPONSE )
            {   /* Control point writeWithoutResponse */
                CsrBtGattWriteCmdReqSend(ascsClient->srvcElem->gattId,
                                     ascsClient->srvcElem->cid,
                                     ascsClient->asesAseControlPointHandle,
                                     message->sizeValue,
                                     value);
            }
            else
            {   /* Control point write */
                CsrBtGattWriteReqSend(ascsClient->srvcElem->gattId,
                         ascsClient->srvcElem->cid,
                         ascsClient->asesAseControlPointHandle,
                         0,
                         message->sizeValue,
                         value);
            }
        }
        break;

        default:
        break;
    }

    CSR_UNUSED(main_inst);
}

/****************************************************************************/
void AscsClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    AscsC *inst = *gash;

    if (CsrSchedMessageGet(&eventClass, &message))
    {

        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
                {
                    CsrBtGattPrim* id = message;
                    GAscsC *ascsClient = (GAscsC *) GetServiceClientByGattMsg(&inst->clientHandleList, message);
                    void *msg = GetGattManagerMsgFromGattMsg(message, id);

                    if(ascsClient)
                    {
                        ascsClientHandleGattMsg(ascsClient, *id, msg);
                    }

                    if (msg != message)
                    {
                        CsrPmemFree(msg);
                        msg = NULL;
                    }
                }
                break;
            case ASCS_CLIENT_PRIM:
                handleAscsClientMsg(inst, message);
                break;
        }
        SynergyMessageFree(eventClass, message);
    }
}

