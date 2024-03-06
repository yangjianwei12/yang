/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"
#include "gatt_hids_server_private.h"
#include "gatt_hids_server_common.h"
#include "gatt_hids_server_access.h"

ServiceHandle hidsServerServiceHandle;

static void hidsListInit(CsrCmnListElm_t *elem)
{
    HidsClientDataElement *inst = (HidsClientDataElement *) elem;

    inst->clientData.cid = 0;
}

uint8 HidsServerInputReportNumber(uint8 totalReportNumber, ReportMapData *mapData)
{
    uint8 inputCount = 0;
	uint8 i;

    for (i = 0 ; i <= totalReportNumber ; i++)
    {
        if (mapData[i].reportType == INPUT_REPORT)
        {
            inputCount++;
        }
    }
    /* only one Input report is allowed */
    if (inputCount > 1 )
    {
        return HIDS_INPUT_REPORT_NUMBER_EXCEEDS_ONE;

    }
    return HIDS_SUCCESS;
}

uint8 HidsServerFeatureReportNumber(uint8 totalReportNumber, ReportMapData *mapData)
{
    uint8 featureCount = 0;
	uint8 i;

    for (i = 0 ; i <= totalReportNumber ; i++)
    {
        if (mapData[i].reportType == FEATURE_REPORT)
        {
            featureCount++;
        }
    }
    /* only three or less Feature report is allowed */
    if (featureCount > 3 )
    {
        return HIDS_FEATURE_REPORT_NUMBER_EXCEEDS_THREE;
    }
    return HIDS_SUCCESS;

}

ServiceHandle GattHidsServerInit(AppTask  theAppTask,
                                 uint16  startHandle,
                                 uint16  endHandle)
{
    GHIDS *gattHidsServerInst = NULL;

    if (theAppTask == CSR_SCHED_QID_INVALID)
    {
        GATT_HIDS_SERVER_PANIC("Application Task NULL\n");
        return 0;
    }

    /* New Service Handle */
    hidsServerServiceHandle = ServiceHandleNewInstance((void **) &gattHidsServerInst, sizeof(GHIDS));

    if (gattHidsServerInst != 0)
    {
        /* Reset all the service library memory */
        memset(gattHidsServerInst, 0, sizeof(GHIDS));

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gattHidsServerInst->appTask = theAppTask;

        /* Set up library handler for external messages */
        gattHidsServerInst->libTask = CSR_BT_HIDS_SERVER_IFACEQUEUE;

        gattHidsServerInst->srvcHandle = hidsServerServiceHandle;
        gattHidsServerInst->startHandle = startHandle;
        gattHidsServerInst->endHandle = endHandle;

        /*Input report struct*/
        for(uint8 i=0 ; i< MAX_NO_INPUT_REPORT; i++)
        {
            gattHidsServerInst->data.inputReport[i].data = NULL;
            gattHidsServerInst->data.inputReport[i].dataLen = 0;
            gattHidsServerInst->data.inputReport[i].reportId = 0;
            gattHidsServerInst->data.inputReport[i].reportType = 0x01;
        }

        /*Feature report struct*/
        for(uint8 i=0 ; i< MAX_NO_FEATURE_REPORT; i++)
        {
            gattHidsServerInst->data.featureReport[i].data = NULL;
            gattHidsServerInst->data.featureReport[i].dataLen = 0;
            gattHidsServerInst->data.featureReport[i].reportId = 0;
            gattHidsServerInst->data.featureReport[i].reportType = 0x03;
        }

        /*report map struct*/
        gattHidsServerInst->data.reportMap= NULL;

        gattHidsServerInst->data.hidInformation.bcdHID = 0;
        gattHidsServerInst->data.hidInformation.bCountryCode = 0;
        gattHidsServerInst->data.hidInformation.flags = 0;

        gattHidsServerInst->data.controlPoint = 0;

        /* Reset the client data memory */
        CsrCmnListInit(&(gattHidsServerInst->data.connectedClients), 0, hidsListInit, NULL);


        /* Register with the GATT */
        CsrBtGattRegisterReqSend(CSR_BT_HIDS_SERVER_IFACEQUEUE, 0);

        GattHidsServerInitCfm* message =  (GattHidsServerInitCfm*)
                                              CsrPmemZalloc(sizeof(GattHidsServerInitCfm));
        message->srvcHndl = gattHidsServerInst->srvcHandle;
        HidsMessageSend(gattHidsServerInst->appTask, GATT_HIDS_SERVER_INIT_CFM, message);

        return gattHidsServerInst->srvcHandle;

    }
    else
    {
        GATT_HIDS_SERVER_PANIC("Memory alllocation of HIDS Server instance failed!\n");
        return 0;
    }
}

/******************************************************************************/
/* HIDS server init Synergy Scheduler Task */

void gattHidsServerInit(void** gash)
{
   *gash = &hidsServerServiceHandle;
    GATT_HIDS_SERVER_INFO("HIDS: gattHidsServerInit\n\n");
}

/******************************************************************************/
status_t GattHidsServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t  cid,
                                 GattHidsServerConfigType *const config)
{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);

    if(hids)
    {
        if(config)
        {
            if(config->inputReport1ClientConfig == GATT_HIDS_SERVER_CCC_INDICATE ||
                    config->inputReport2ClientConfig == GATT_HIDS_SERVER_CCC_INDICATE)
            {
                /* HIDS characteristics can be only notified */
                GATT_HIDS_SERVER_ERROR("Invalid Client Configuration Characteristic!\n");
                return CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED;
            }
            else
            {
                /* Adding new client to front of list */
                HidsClientDataElement *client = HIDS_ADD_CLIENT(hids->data.connectedClients);

                /* Store clients configuration in Server struct */
                client->clientData.clientCfg.inputReport1ClientConfig = config->inputReport1ClientConfig;
                client->clientData.clientCfg.inputReport2ClientConfig = config->inputReport2ClientConfig;
                client->clientData.cid = cid;

                if(hids->data.connectedClients.last == NULL)
                {
                    hids->data.connectedClients.last = hids->data.connectedClients.first;
                }

                return CSR_BT_GATT_RESULT_SUCCESS;

            }
        }
        else
        {
            HidsClientDataElement *client = HIDS_ADD_CLIENT(hids->data.connectedClients);
            client->clientData.cid = cid;
            CsrMemSet(&client->clientData.clientCfg, 0, sizeof(GattHidsServerConfigType));
            return CSR_BT_GATT_RESULT_SUCCESS;
        }
    }

    return CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;

}

/*******************************************************************************
 * GattHidsRemoveConfig
 *
 *  1. Find the connection instance (identified by the cid)
 *  2. Store all the connection instance data in the GATT_HIDS_CLIENT_CONFIG_T (so that the connection can be re-instantiated later)
 *  3. Remove the connection instance from the HIDS library
 */
GattHidsServerConfigType* GattHidsServerRemoveConfig(ServiceHandle srvcHndl,
                                                connection_id_t connectionId)
{
    GHIDS *hidsInst = (GHIDS *) ServiceHandleGetInstanceData(srvcHndl);
    GattHidsServerConfigType *config = NULL;

    if(hidsInst)
    {
        HidsClientDataElement *client = hidsFindClient(&(hidsInst->data.connectedClients), connectionId);
        if (client)
        {
            config = (GattHidsServerConfigType *)CsrPmemZalloc(sizeof(GattHidsServerConfigType));
            SynMemCpyS(config, sizeof(GattHidsServerConfigType), &(client->clientData.clientCfg), sizeof(GattHidsServerConfigType));
            HIDS_REMOVE_CLIENT(hidsInst->data.connectedClients, client);
        }
    }

    return config;
}

/******************************************************************************/
uint8 GattHidsServerSetHidsInfo(ServiceHandle srvcHndl,
                                uint16      bcdHID,
                                uint8       bCountryCode,
                                uint8       flags)
{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);
    if (hids == NULL)
    {
        GATT_HIDS_SERVER_ERROR("\n HIDS: NULL instance \n");
        return HIDS_ERROR;
    }
    else
    {
        hids->srvcHandle = srvcHndl;
        hids->data.hidInformation.bcdHID = (uint16)bcdHID;
        hids->data.hidInformation.bCountryCode = (uint8)bCountryCode;
        hids->data.hidInformation.flags = (uint8)flags;

        GattHidsServerSetHidsInfoCfm* message =  (GattHidsServerSetHidsInfoCfm*)
                                              CsrPmemZalloc(sizeof(GattHidsServerSetHidsInfoCfm));
        message->srvcHndl = hids->srvcHandle;
        message->hidsInfo= hids->data.hidInformation;

        HidsMessageSend(hids->appTask, GATT_HIDS_SERVER_SET_HIDS_INFO_CFM, message);

        return HIDS_SUCCESS;
    }
}
/******************************************************************************/
uint8 GattHidsServerSetControlPoint(ServiceHandle srvcHndl,
                                uint8       controlPoint)
{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);
    if (hids == NULL)
    {
        GATT_HIDS_SERVER_ERROR("\n HIDS: NULL instance \n");
        return HIDS_ERROR;
    }
    if (controlPoint!= GATT_HIDS_SERVER_SUSPEND || controlPoint != GATT_HIDS_SERVER_EXIT_SUSPEND)
    {
         hids->data.controlPoint = controlPoint;
         GattHidsServerSetControlPointCfm* message =  (GattHidsServerSetControlPointCfm*)
                                               CsrPmemZalloc(sizeof(GattHidsServerSetControlPointCfm));
         message->srvcHndl = hids->srvcHandle;
         message->controlPoint = hids->data.controlPoint;
         HidsMessageSend(hids->appTask, GATT_HIDS_SERVER_SET_CONTROL_POINT_CFM, message);
    }
    else
    {
        return HIDS_ERROR;
    }

    return HIDS_SUCCESS;
}

/******************************************************************************/
GattHidsServerStatus GattHidsServerSetInputReport(ServiceHandle srvcHndl,
                                   uint8         index,
                                   uint8         reportId,
                                   uint16        length,
                                   uint8         *InputData)

{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);
    HidsClientDataElement *client = NULL;
    if (hids == NULL)
    {
        GATT_HIDS_SERVER_ERROR("\n HIDS: NULL instance \n");
        return HIDS_ERROR;
    }

    /* index can only be 1 or 2 */
    if (hids == NULL || index > MAX_NO_INPUT_REPORT || index == 0
            || hids->data.totalNumberOfReport == 0 || hids->data.totalNumberOfReport > MAX_NO_REPORT)
    {
        GATT_HIDS_SERVER_ERROR("\n HIDS: NULL instance \n");
        return HIDS_ERROR;
    }
    else
    {
        uint8 i;

        /* Does the report match the one that Report Map told us?
           Go through report map and check if we are told about this report.
           If yes, then populate inputReport.
           i is the index for report_map.
           index-1 is the index for inputReport.
           i and index-1 do need to be equal.
        */
        for (i = 0; i < hids->data.totalNumberOfReport; i++)
        {
            if (hids->data.RMapData[i].reportType ==  INPUT_REPORT &&
                hids->data.RMapData[i].reportLen == length &&
                hids->data.RMapData[i].reportId == reportId)
            {
                pfree(hids->data.inputReport[index-1].data);

                hids->data.inputReport[index-1].dataLen = length;
                hids->data.inputReport[index-1].data = InputData;
                hids->data.inputReport[index-1].reportId = reportId;
                hids->data.inputReport[index-1].reportType = INPUT_REPORT;

                /* notify the change*/
                for (client = (HidsClientDataElement *)hids->data.connectedClients.first; client; client = client->next)
                {
                    /* If the Client Config is 0x01 (Notify is TRUE), a notification will
                    * be sent to the client */
                    if(index == 1)
                    {
                        if(client->clientData.clientCfg.inputReport1ClientConfig == GATT_HIDS_SERVER_CCC_NOTIFY)
                        {
                            hidsServerSendCharacteristicChangedNotification(hids->gattId,
                                                                        client->clientData.cid,
                                                                        HANDLE_HIDS_REPORT_INPUT_1,
                                                                        hids->data.inputReport[index-1].dataLen,
                                                                        (uint8*)(hids->data.inputReport[index-1].data));
                        }
                    }
                    else if (index == 2)
                    {
                        if(client->clientData.clientCfg.inputReport2ClientConfig == GATT_HIDS_SERVER_CCC_NOTIFY)
                        {
                            hidsServerSendCharacteristicChangedNotification(hids->gattId,
                                                                        client->clientData.cid,
                                                                        HANDLE_HIDS_REPORT_INPUT_2,
                                                                        hids->data.inputReport[index-1].dataLen,
                                                                        (uint8*)(hids->data.inputReport[index-1].data));
                        }
                    }
                }

                break;
            }
        }

        GattHidsServerSetInputReportCfm* message = (GattHidsServerSetInputReportCfm*)
                        CsrPmemZalloc(sizeof(GattHidsServerSetInputReportCfm));

        if (i == hids->data.totalNumberOfReport)
        {
            /* Unable to find the report from the report map, send error to application.*/
            message->status = HIDS_ERROR;
        }
        else
        {
            message->srvcHndl = hids->srvcHandle;
            message->reportId = hids->data.inputReport[index-1].reportId;
            message->dataLen = hids->data.inputReport[index-1].dataLen;
            message->status = HIDS_SUCCESS;
        }

        HidsMessageSend(hids->appTask, GATT_HIDS_SERVER_SET_INPUT_REPORT_CFM, message);

        return message->status;
    }
}
/******************************************************************************/
uint8 GattHidsServerSetFeatureReport(ServiceHandle srvcHndl,
                                   uint8           index,
                                   uint8           reportId,
                                   uint16          length,
                                   uint8           *featureData)

{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);

    /* index can only be 1 or 2 or 3 */
    if (hids == NULL || index > MAX_NO_FEATURE_REPORT || index == 0)
    {
        GATT_HIDS_SERVER_ERROR("\n HIDS: NULL instance \n");
        return HIDS_ERROR;
    }
    else
    {
        uint8 i;
        /* Does the report matches the one that Report Map told us? */
        for (i = 0; i < MAX_NO_FEATURE_REPORT; i++)
        {
            if (hids->data.RMapData[i].reportType ==  FEATURE_REPORT &&
                hids->data.RMapData[i].reportLen == length &&
                hids->data.RMapData[i].reportId == reportId)
            {
                pfree(hids->data.featureReport[index-1].data);

                hids->data.featureReport[index-1].dataLen = length;
                hids->data.featureReport[index-1].data = featureData;
                hids->data.featureReport[index-1].reportId = reportId;

                break;
            }
        }

        return (i == MAX_NO_FEATURE_REPORT ? HIDS_ERROR : HIDS_SUCCESS);
    }
}

/******************************************************************************/
/* From reportMap data, we need to extract
 * * number of Input Report which should be maximum two
 * * number Feature reports which should be maximum three
 * * usage page
 * * report ID for each report
 * * reaport size and report count which tell us the length of the data we will
 * * be receiving.

 * Below table shows how the needed data, is being extracted
 * Of course, this table is much bigger but I only added the rows we are
 * interested in it

 |-----------------------------------------------------------------------------|
 |  b39...b8 |b7| b6| b5| b4| b3| b2| b1| b0|   HID CLASS SHORT ITEM FORMAT    |
 |------------------------------------------|                                  |
 |           |  bTag        | bType | bSize |                                  |
 |-----------|--------------|-------|-------|----------------------------------|
 |           |  |   |   |   |   |   | 0 | 0 | [Data] Length: 0 Oct             |
 |-----------|--------------|-------|-------|----------------------------------|
 |           |  |   |   |   |   |   | 0 | 1 | [Data] Length: 1 Oct             |
 |-----------|--------------|-------|-------|----------------------------------|
 |           |  |   |   |   |   |   | 1 | 0 | [Data] Length: 2 Oct             |
 |-----------|--------------|-------|-------|----------------------------------|
 |           |  |   |   |   |   |   | 1 | 1 | [Data] Length: 4 Oct             |
 |-----------|--------------|-------|-------|----------------------------------|
 |           |  |   |   |   | 0 | 0 | 'Main'tem                                |
 |-----------|--------------|-------|------------------------------------------|
 |           |  |   |   |   | 0 | 1 | 'Global'item                             |
 |-----------|--------------|-------|------------------------------------------|
 |           |  |   |   |   | 1 | 0 | 'Local'item                              |
 |-----------|--------------|-------|------------------------------------------|
 |           |  |   |   |   | 1 | 1 | Reserve                                  |
 |-----------|--------------|-------|------------------------------------------|
 |           |Depend  bType | 0xbTag[0|1|2|4] = 'Main' item[0|1|2|4] octects   |
 |-----------|--------------|--------------------------------------------------|
 |           |1 |0  |0  | 0 | 0x8[0|1|2|4] = Input                             |
 |-----------|--------------|--------------------------------------------------|
 |           |1 |0  |1  | 1 | 0xB[0|1|2|4] = Feature                           |
 |-----------|--------------|--------------------------------------------------|
 |           |1 |1  |0  | 0 | 0xC[0|1|2|4] = End Collection                    |
 |-----------|--------------|--------------------------------------------------|
 |           |x |x  |x  | x | 0x0, 0x7,0xD and 0xF[0|1|2|4] = Reserved         |
 |-----------|--------------|--------------------------------------------------|
 |           |              | 0xbTag[4|5|6|7] = 'Global' item[0|1|2|4] octects |
 |-----------|--------------|--------------------------------------------------|
 |           |0 |0  |0  | 0 | 0x0[4|5|6|7] = Usage Page                        |
 |-----------|--------------|--------------------------------------------------|
 |           |0 |1  |1 | 1  | 0x7[4|5|6|7] = Report Size                       |
 |-----------|--------------|--------------------------------------------------|
 |           |1 |0  |0  | 0 | 0x8[4|5|6|7] = Report ID                         |
 |-----------|--------------|--------------------------------------------------|
 |           |1 |0  |0  | 1 | 0x9[4|5|6|7] = Report Count                      |
 |-----------|--------------|--------------------------------------------------|
 |           |x |x  |x  | x | 0xC - 0xF[4|5|6|7] = Reserved                    |
 |-----------|--------------|--------------------------------------------------|
 |           |              | 0xbTag[8|9|A|B] = 'Local' item[0|1|2|4] octects  |
 |-----------|--------------|--------------------------------------------------|
 |           |0 |0  |0  | 0 | 0x0[8|9|A|B] = Usage                             |
 |-----------|--------------|--------------------------------------------------|
 |           |x |x  |x  | x | 0xB - 0xF[4|5|6|7] = Reserved                    |
 |-----------|--------------|--------------------------------------------------|
*/

uint8 GattHidsServerSetReportMap(ServiceHandle srvcHndl,
                                  uint16       len,
                                  const uint8  *mapData)
{
     GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);
     uint8 bSize = 0, bType = 0, bTag = 0, reportSize = 0 , reportCount = 0 ;

     if(hids == NULL || len == 0 || mapData == NULL)
     {
         return HIDS_ERROR;  /*cannot progress any further*/
     }
     else
     {
         /*Let's do  fresh start */
         hids->data.totalNumberOfReport = 0;
         for (uint8 count = 0 ; count < len; count++)
        {
            /*bit 0 and 1 determine the data size of octets followed*/
            bSize = mapData[count] & 3;
            bType = (mapData[count] >> 2) & 3;
            bTag = (mapData[count] >> 4) & 15;

            if(bType == HIDS_TYPE_GLOBAL)
            {
                /* Get the Usage Page */
                if(bTag == HIDS_TAG_USAGE_PAGE)
                {
                    hids->data.usagePage = mapData[count+bSize];
                }
                /* Get the report id */
                else if(bTag == HIDS_TAG_REPORT_ID)
                {
                    hids->data.RMapData[hids->data.totalNumberOfReport].reportId = mapData[count+bSize];
                    hids->data.totalNumberOfReport++;
                }
                /* Get the report type*/
                else if(bTag == HIDS_TAG_FEATURE || bTag == HIDS_TAG_INPUT )
                {
                    hids->data.RMapData[hids->data.totalNumberOfReport-1].reportType = mapData[count+bSize];
                }
                /* Get the report length */
                else if(bTag == HIDS_TAG_REPORT_SIZE)
                    reportSize = mapData[count+bSize];

                else if(bTag == HIDS_TAG_REPORT_COUNT)
                {
                    reportCount = mapData[count+bSize];
                    hids->data.RMapData[hids->data.totalNumberOfReport-1].reportLen += reportSize * reportCount;
                    reportSize = 0 , reportCount = 0 ;
                }
            }
            /* Get the report type*/
            else if(bType == HIDS_TYPE_MAIN && (bTag == HIDS_TAG_FEATURE || bTag == HIDS_TAG_INPUT) )
            {
                hids->data.RMapData[hids->data.totalNumberOfReport-1].reportType = mapData[count+bSize];
            }
        }
        /*UsagePage should always be present*/
        if (hids->data.usagePage == 0)  return HIDS_ERROR;
     }

     GattHidsServerSetReportMapCfm* message =  (GattHidsServerSetReportMapCfm*)
                                           CsrPmemZalloc(sizeof(GattHidsServerSetReportMapCfm));
     message->srvcHndl = hids->srvcHandle;
     message->usagePage = hids->data.usagePage;
     message->totalReportNo = hids->data.totalNumberOfReport;

     for (uint8 i=0; i<hids->data.totalNumberOfReport ; i++)
     {
         message->reportId[i] = hids->data.RMapData[i].reportId;
         message->reportType[i] = hids->data.RMapData[i].reportType;
         message->reportLen[i] = hids->data.RMapData[i].reportLen;
     }
     HidsMessageSend(hids->appTask, GATT_HIDS_SERVER_SET_REPORT_MAP_CFM, message);

     return HIDS_SUCCESS;

}

/******************************************************************************/
uint8 GattHidsServerTerminate(ServiceHandle srvcHndl)
{
    GHIDS *hids = (GHIDS*)ServiceHandleGetInstanceData(srvcHndl);
    if(hids == NULL)
    {
        return HIDS_ERROR;  /*cannot progress any further*/
    }
    else
    {
        uint8 index;

        for (index=0; index < MAX_NO_INPUT_REPORT ; index++)
        {
            if (hids->data.inputReport[index].data)
            {
                pfree(hids->data.inputReport[index].data);
                hids->data.inputReport[index].data = NULL;
            }
        }

        for (index=0; index < MAX_NO_FEATURE_REPORT ; index++)
        {
            if (hids->data.featureReport[index].data)
            {
                pfree(hids->data.featureReport[index].data);
                hids->data.featureReport[index].data = NULL;
            }
        }

        CsrCmnListDeinit(&hids->data.connectedClients);

        CsrBtGattUnregisterReqSend(hids->gattId);
        if(ServiceHandleFreeInstanceData(srvcHndl))
        {
            GATT_HIDS_SERVER_INFO("HIDS: gatt_hids_deinit\n\n");
        }
        else
        {
            GATT_HIDS_SERVER_PANIC("Unable to free the HIDS server instance\n");
            return HIDS_ERROR;
        }
        return HIDS_SUCCESS;
    }
}

/******************************************************************************/
