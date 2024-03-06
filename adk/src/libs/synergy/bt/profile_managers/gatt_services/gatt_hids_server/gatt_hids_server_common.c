/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#include "gatt_hids_server_common.h"

#include "gatt_hids_server_private.h"
#include "gatt_hids_server_debug.h"
#include "gatt_hids_server_access.h"

void hidsServerSendAccessRsp(
        CsrBtGattId task,
        ConnectionId cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 *const value)
{
    CsrBtGattDbReadAccessResSend(task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 value);
}


static status_t hidsServerSetCCC(GHIDS *hids_server,
                                ConnectionId cid,
                                uint16 handle,
                                uint8 *ccc)
{
    HidsClientDataElement *client = hidsFindClient(&hids_server->data.connectedClients, cid);
    if(client)
    {
        switch (handle)
        {
            case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_1:
                client->clientData.clientCfg.inputReport1ClientConfig = ccc[0];
                break;
        case HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_2:
            client->clientData.clientCfg.inputReport2ClientConfig = ccc[0];
            break;
        }
    }
     else
     {
        /* Invalid cid */
         GATT_HIDS_SERVER_ERROR("Invalid cid!\n");
         return CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID;
     }
     return CSR_BT_GATT_RESULT_SUCCESS;
}

void hidsServerHandleWriteClientConfigForInputReport(GHIDS *hids_server, CsrBtGattDbAccessWriteInd *const accessInd)
{
    uint8 i;
    uint16 sizeValue = 0;

    for(i = 0; i < accessInd->writeUnitCount; i++)
        sizeValue += accessInd->writeUnit[i].valueLength;

    if (sizeValue != GATT_HIDS_SERVER_CCC_VALUE_SIZE)
    {
        hidsServerSendAccessErrorRsp(
                    hids_server->gattId,
                    accessInd->btConnId,
                    accessInd->attrHandle,
                    CSR_BT_GATT_RESULT_INVALID_LENGTH);
    }
    else if (accessInd->writeUnit[0].value[0] == GATT_HIDS_SERVER_CCC_NOTIFY || accessInd->writeUnit[0].value[0] == 0)
    {
        /* Valid value of CCC */

        /* Save the new ccc in the library */
        status_t status = hidsServerSetCCC(
                                   hids_server,
                                   (connection_id_t) accessInd->btConnId,
                                   accessInd->attrHandle,
                                   accessInd->writeUnit[0].value);

        /* Send response to the client */
        gattHidsServerWriteGenericResponse(
                    hids_server->gattId,
                    accessInd->btConnId,
                    status,
                    accessInd->attrHandle);
    }
    else
    {
        /* Send response to the client but the value is ignored*/
        gattHidsServerWriteGenericResponse(
                    hids_server->gattId,
                    accessInd->btConnId,
                    CSR_BT_GATT_RESULT_SUCCESS,
                    accessInd->attrHandle);
    }
}


void hidsServerHidsInfoData(uint8 *hids_info, GHIDS *hids_server)
{
    hids_info[0] = (uint8)(hids_server->data.hidInformation.bcdHID & 0xFF);
    hids_info[1] = (uint8)(hids_server->data.hidInformation.bcdHID >>8);
    hids_info[2] = hids_server->data.hidInformation.bCountryCode;
    hids_info[3] = hids_server->data.hidInformation.flags;

}

void hidsServerReportMapData(uint8 *report, GHIDS *hids_server)
{
    for(uint8 i = 0 ; i <=hids_server->data.reportMap->mapDataLen ; i++ )
    {
        report[i] = hids_server->data.reportMap->mapData[i];
    }
}

void hidsServerReport(uint8 *report, GHIDS *hids_server, uint8 index)
{

    uint16 len = hids_server->data.inputReport[index].dataLen;
    for(uint8 i = 0 ; i < len ; i++ )
    {
        report[i] = hids_server->data.inputReport[index].data[i];
    }

}


void hidsServerFeatureReport(uint8 *report, GHIDS *hids_server, uint8 index)
{

    uint16 len = hids_server->data.inputReport[index].dataLen;
    for(uint8 i = 0 ; i <= len ; i++ )
    {
        report[i] = hids_server->data.inputReport[index].data[i];
    }
}

void hidsServerHidsClientConfigData(uint8 *hids_info, GHIDS *hids_server, const CsrBtGattAccessInd *access_ind)
{
    HidsClientDataElement *client = hidsFindClient(&hids_server->data.connectedClients, access_ind->cid);
    if(client)
    {
        if(access_ind->handle == HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_1)
        {
           hids_info[0] = (uint8)(client->clientData.clientCfg.inputReport1ClientConfig & 0xFF);
           hids_info[1] = (uint8)(client->clientData.clientCfg.inputReport1ClientConfig >> 8);
        }
        else if(access_ind->handle == HANDLE_HIDS_REPORT_INPUT_CLIENT_CONFIG_2)
        {
            hids_info[0] = (uint8)(client->clientData.clientCfg.inputReport2ClientConfig & 0xFF);
            hids_info[1] = (uint8)(client->clientData.clientCfg.inputReport2ClientConfig >> 8);
        }
    }
}

void hidsServerFeatureReportRefData(uint8 *hids_info, GHIDS *hids_server, uint8 index)
{
    hids_info[0] = (uint8)hids_server->data.featureReport[index].reportId;
    hids_info[1] = (uint8)hids_server->data.featureReport[index].reportType;
}

/***************************************************************************/
void hidsServerSendCharacteristicChangedNotification(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 sizeValue,
        uint8 const *value
        )
{
    uint8* data;
    data = (uint8*)CsrPmemAlloc(sizeValue);

    if (data == NULL)
    {
        GATT_HIDS_SERVER_PANIC(
            "HIDS: insufficient resources!\n"
            );
        return;
    }

    memcpy(data, value, sizeValue);

    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_HIDS_SERVER_PANIC(
                    "HIDS: No GattId!\n"
                    );
    }
    else if ( cid == 0 )
    {
        GATT_HIDS_SERVER_PANIC(
                    "GHIDS: No Cid!\n"
                    );
    }
    else
    {
        CsrBtGattNotificationEventReqSend(task,
                                         cid,
                                         handle,
                                         sizeValue,
                                         data);
    }
}


/******************************************************************************/
void sendHidsServerAccessRsp(
        CsrBtGattId task,
        connection_id_t cid,
        uint16 handle,
        uint16 result,
        uint16 sizeValue,
        uint8 *value
        )
{

    CsrBtGattDbReadAccessResSend(
                                 task,
                                 cid,
                                 handle,
                                 result,
                                 sizeValue,
                                 value);
}

void gattHidsServerWriteGenericResponse(
        CsrBtGattId task,
        connection_id_t cid,
        uint16      result,
        uint16      handle
        )
{
    if (task == CSR_BT_GATT_INVALID_GATT_ID)
    {
        GATT_HIDS_SERVER_PANIC(
                    "GTBS: Null instance!\n"
                    );
    }
    else if (cid == 0)
    {
        GATT_HIDS_SERVER_PANIC("GTBS: No Cid!\n");
    }
    else
    {
        CsrBtGattDbWriteAccessResSend(
                                      task,
                                      cid,
                                      handle,
                                      result);
    }
}


