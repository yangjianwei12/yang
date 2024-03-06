/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "csr_bt_gatt_lib.h"

#include "gatt_hids_server_private.h"
#include "gatt_hids_server_access.h"
#include "gatt_hids_server_db.h"
#include "gatt_hids_server_debug.h"
#include "gatt_hids_server_msg_handler.h"
#include "gatt_hids_server_common.h"


#include <stdlib.h>

void hidsServerRespondToCharacteristicRead(CsrBtGattDbAccessReadInd *readInd, uint8 *value, uint16 valueLength, CsrBtGattId gattId)
{
    if (value != NULL)
    {
        hidsServerSendAccessRsp(gattId,
                                readInd->btConnId,
                                readInd->attrHandle,
                                CSR_BT_GATT_RESULT_SUCCESS,
                                valueLength,
                                value);
    }
    else
    {
        hidsServerSendAccessRsp(gattId,
                                readInd->btConnId,
                                readInd->attrHandle,
                                CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR,
                                valueLength,
                                value);
    }
}


static void hidsServerHandleControlPoint(GHIDS *hids_server, const CsrBtGattDbAccessWriteInd *access_ind)
{
    if ((access_ind->type & ATT_ACCESS_WRITE) &&  (*access_ind->writeUnit->value == 0 ||*access_ind->writeUnit->value == 1))
    {
        gattHidsServerWriteGenericResponse(hids_server->gattId,
                                           access_ind->btConnId,
                                           CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                           HANDLE_HIDS_CONTROL_POINT);

        GattHidsServerControlPointWriteInd* message = (GattHidsServerControlPointWriteInd*)
                                                    CsrPmemZalloc(sizeof(GattHidsServerControlPointWriteInd));

        message->srvcHndl = hids_server->srvcHandle;
        message->gattId = hids_server->gattId;
        message->cid = access_ind->btConnId;
        message->OldControlPoint = hids_server->data.controlPoint;
        message->NewControlPoint =  *access_ind->writeUnit->value;
        hids_server->data.controlPoint = message->NewControlPoint;

        HidsMessageSend(hids_server->appTask, GATT_HIDS_SERVER_CONTROL_POINT_WRITE_IND, message );

    }
}

static void hidsServerHandleFeatureReportInd(GHIDS *hids_server, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 status;
    if (access_ind->type & ATT_ACCESS_WRITE)
    {

        if (access_ind->attrHandle == HANDLE_HIDS_REPORT_FEATURE_1)
        {
            gattHidsServerWriteGenericResponse(hids_server->gattId,
                                           access_ind->btConnId,
                                           CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                           HANDLE_HIDS_REPORT_FEATURE_1);
        }
        else if (access_ind->attrHandle == HANDLE_HIDS_REPORT_FEATURE_2)
        {
            gattHidsServerWriteGenericResponse(hids_server->gattId,
                                           access_ind->btConnId,
                                           CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                           HANDLE_HIDS_REPORT_FEATURE_2);
        }
        else if (access_ind->attrHandle == HANDLE_HIDS_REPORT_FEATURE_3)
        {
            gattHidsServerWriteGenericResponse(hids_server->gattId,
                                           access_ind->btConnId,
                                           CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                           HANDLE_HIDS_REPORT_FEATURE_3);
        }        
        status = HIDS_SUCCESS;
    }
    else
    {
        status = HIDS_ERROR;
    }
    GattHidsServerFeatureReportWriteInd* message = (GattHidsServerFeatureReportWriteInd*)
                                                CsrPmemZalloc(sizeof(GattHidsServerFeatureReportWriteInd));

    message->srvcHndl = hids_server->srvcHandle;
    message->gattId = hids_server->gattId;
    message->cid = access_ind->btConnId;
    message->dataLen = access_ind->writeUnit->valueLength;
    message->data =  access_ind->writeUnit->value;
    message->status = status;
    HidsMessageSend(hids_server->appTask, GATT_HIDS_SERVER_FEATURE_REPORT_WRITE_IND, message);
}


static void hidsServerHandleHidInfo(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {

        uint8 *value = NULL;
        value = (uint8*) CsrPmemAlloc(HID_INFORMATION_SIZE);
        hidsServerHidsInfoData(value, hids_server);

        sendHidsServerAccessRsp(
                hids_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                HID_INFORMATION_SIZE,
                value
                );


    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}


static void hidsServerHandleFeatureReportRef(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind, uint8 index)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {

        uint8 *value = NULL;
        value = (uint8*) CsrPmemAlloc(2);
        hidsServerFeatureReportRefData(value, hids_server, index);

        sendHidsServerAccessRsp(
                hids_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                2,
                value
                );

    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

static void hidsServerHandleClientConfigForInputReport(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {

        uint8 *value = NULL;
        value = (uint8*) CsrPmemAlloc(2);
        hidsServerHidsClientConfigData(value, hids_server, access_ind);

        sendHidsServerAccessRsp(
                hids_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                2,
                value
                );

    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

static void hidsServerHandleReportMap(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {

        if(hids_server->data.reportMap == NULL)
        {
            sendHidsServerAccessRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    0,
                    NULL
                    );
        }
        else
        {
            uint8 *value = NULL;
            value = (uint8*) CsrPmemAlloc(hids_server->data.reportMap->mapDataLen);
            hidsServerReportMapData(value, hids_server);

            sendHidsServerAccessRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    hids_server->data.reportMap->mapDataLen,
                    value
                    );

        }

    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }


}

void hidsServerHandleInputReport(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind, uint8 index)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        if(hids_server->data.inputReport[index].data == NULL)
        {
            sendHidsServerAccessRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    0,
                    NULL
                    );
        }
        else
        {
             uint8 *value = NULL;
             value = (uint8*) CsrPmemAlloc(sizeof(uint8) * hids_server->data.inputReport[index].dataLen);
             hidsServerReport(value, hids_server, index);
             sendHidsServerAccessRsp(
                hids_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(uint8) * hids_server->data.inputReport[index].dataLen,
                value
                );

        }

    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }


}

void hidsServerHandleFeatureReport(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind, uint8 index)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {


        if(hids_server->data.featureReport[index].data == NULL)
        {
            sendHidsServerAccessRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    0,
                    NULL
                    );
        }
        else
        {
             uint8 *value = NULL;
             value = (uint8*) CsrPmemAlloc(sizeof(uint8) * hids_server->data.featureReport[index].dataLen);
             hidsServerFeatureReport(value, hids_server, index);

             sendHidsServerAccessRsp(
                hids_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(uint8) * hids_server->data.featureReport[index].dataLen,
                value
                );

        }

    }
    else
    {
        /* Reject access requests that aren't read, which shouldn't happen. */
        sendHidsServerAccessErrorRsp(
                    hids_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }

}

HidsClientDataElement *hidsFindClient(CsrCmnList_t *connectedClients, ConnectionId cid)
{
    HidsClientDataElement *client;

    for(client = (HidsClientDataElement *)connectedClients->first; client; client = client->next)
    {
        if(client->clientData.cid == cid)
        {
            return client;
        }
    }
    GATT_HIDS_SERVER_ERROR("HIDS Server: Client not found");
    return NULL;
}
/***************************************************************************
NAME
    hidsServerGetFeatureReportCharacteristicHandleFromAccessInd

DESCRIPTION
    Get the characteristic handle to use in handleHidsServerAccess from the
    real handle in the received access indication for Feature reports.
*/
static uint8 hidsServerGetFeatureReportCharacteristicHandleFromAccessInd(
                                             const CsrBtGattAccessInd *access_ind,
                                             uint16 *handle)
{
    uint8 i;

    /* Offset is the offset between the handles of the report feature characteristics in the ATT database */
    uint8 offset = GATT_HIDS_FEATURE_REPORT_DB_SIZE;

    for(i=0; i< MAX_NO_FEATURE_REPORT; i++)
    {
        if(access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_1 + (i * offset)))
        {
            (*handle) = HANDLE_HIDS_REPORT_FEATURE_1;
            break;
        }
        else if (access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_2 + (i * offset)))
        {
            (*handle) =  HANDLE_HIDS_REPORT_FEATURE_2;
            break;
        }
        else if (access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_3 + (i * offset)))
        {
            (*handle) =  HANDLE_HIDS_REPORT_FEATURE_3;
            break;
        }
    }
    return i;
}

/***************************************************************************
NAME
    hidsServerGetInputReportCharacteristicHandleFromAccessInd

DESCRIPTION
    Get the characteristic handle to use in handleHidsServerAccess from the
    real handle in the received access indication for Input reports.
*/
static uint8 hidsServerGetInputReportCharacteristicHandleFromAccessInd(
                                             const CsrBtGattAccessInd *access_ind,
                                             uint16 *handle)
{
    uint8 i;

    /* Offset is the offset between the handles of the report Input characteristics in the ATT database */
    uint8 offset = GATT_HIDS_INPUT_REPORT_DB_SIZE;

    for(i=0; i< MAX_NO_INPUT_REPORT; i++)
    {
        if(access_ind->handle == (HANDLE_HIDS_REPORT_INPUT_1 + (i * offset)))
        {
            (*handle) = HANDLE_HIDS_REPORT_INPUT_1;
            break;
        }
        else if (access_ind->handle == (HANDLE_HIDS_REPORT_INPUT_2 + (i * offset)))
        {
            (*handle) =  HANDLE_HIDS_REPORT_INPUT_2;
            break;
        }
    }
    return i;
}

/***************************************************************************
NAME
    hidsServerGetCharacteristicRefHandleFromAccessInd

DESCRIPTION
   Get the characteristic handle to use in handleHidsServerAccess from the
   real handle in the received access indication for Feature reports.
    */
static uint8 hidsServerGetCharacteristicRefHandleFromAccessInd(
                                                 const CsrBtGattAccessInd *access_ind,
                                                 uint16 *handle)
{
        uint8 i;

        /* Offset is the offset between the handles of the report feature characteristics in the ATT database */
        uint8 offset = GATT_HIDS_FEATURE_REPORT_DB_SIZE;

        for(i=0; i< MAX_NO_FEATURE_REPORT; i++)
        {
            if(access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_1_REF + (i * offset)))
            {
                (*handle) = HANDLE_HIDS_REPORT_FEATURE_1_REF;
                break;
            }
            else if (access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_2_REF + (i * offset)))
            {
                (*handle) =  HANDLE_HIDS_REPORT_FEATURE_2_REF;
                break;
            }
            else if (access_ind->handle == (HANDLE_HIDS_REPORT_FEATURE_3_REF + (i * offset)))
            {
                (*handle) =  HANDLE_HIDS_REPORT_FEATURE_3_REF;
                break;
            }
        }
    return i;
}

void hidsServerHandleAccessWriteIndication(GHIDS *hids_server, CsrBtGattDbAccessWriteInd *const accessInd)
{
    switch (accessInd->attrHandle)
    {
        case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_1:
        case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_2:
        {
            GATT_HIDS_SERVER_INFO("hidsServerHandleAccessWriteIndication: Handle (0x%04X) is being written to\n.", accessInd->attrHandle);
            hidsServerHandleWriteClientConfigForInputReport(hids_server,accessInd);
            break;
        }

        case HANDLE_HIDS_REPORT_FEATURE_1:
        case HANDLE_HIDS_REPORT_FEATURE_2:
        case HANDLE_HIDS_REPORT_FEATURE_3:
        {
            hidsServerHandleFeatureReportInd(hids_server,accessInd);
            break;
        }
        case HANDLE_HIDS_CONTROL_POINT:
        {
            hidsServerHandleControlPoint(hids_server,accessInd );
            break;
        }

     }
}

void handleHidsServerAccess(GHIDS *hids_server, const CsrBtGattAccessInd *access_ind)
{
    uint16 handle = access_ind->handle;

    switch (handle)
    {
        case HANDLE_HIDS_REPORT_INPUT_1:
        case HANDLE_HIDS_REPORT_INPUT_2:
        {
            uint8 index = hidsServerGetInputReportCharacteristicHandleFromAccessInd(access_ind, &handle);
            if(index < MAX_NO_INPUT_REPORT)
            {
                hidsServerHandleInputReport(hids_server,access_ind, index);
            }
            break;
        }
        case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_1:
        case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_2:
        {
            hidsServerHandleClientConfigForInputReport(hids_server,access_ind);
            break;
        }
        case HANDLE_HIDS_REPORT_FEATURE_1:
        case HANDLE_HIDS_REPORT_FEATURE_2:
        case HANDLE_HIDS_REPORT_FEATURE_3:
        {
            uint8 index = hidsServerGetFeatureReportCharacteristicHandleFromAccessInd(access_ind, &handle);
            if(index < MAX_NO_FEATURE_REPORT)
            {
                hidsServerHandleFeatureReport(hids_server,access_ind, index);
            }
            break;
        }

        case HANDLE_REPORT_MAP:
        {
            hidsServerHandleReportMap(hids_server,access_ind);
            break;
        }

        case HANDLE_HIDS_INFORMATION:
        {
            hidsServerHandleHidInfo(hids_server,access_ind );
            break;
        }
        case HANDLE_HIDS_REPORT_FEATURE_1_REF:
        case HANDLE_HIDS_REPORT_FEATURE_2_REF:
        case HANDLE_HIDS_REPORT_FEATURE_3_REF:       
        {
            uint8 index = hidsServerGetCharacteristicRefHandleFromAccessInd(access_ind, &handle);
            if(index < MAX_NO_FEATURE_REPORT)
            {
                hidsServerHandleFeatureReportRef(hids_server,access_ind, index);
            }
            break;
        }

        case HANDLE_EXT_REPORT_MAP:
        {   /* No data available for extended report map */
            sendHidsServerAccessRsp(hids_server->gattId,
                                    access_ind->cid,
                                    access_ind->handle,
                                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                    0,
                                    NULL);
            break;
        }
    }
}
