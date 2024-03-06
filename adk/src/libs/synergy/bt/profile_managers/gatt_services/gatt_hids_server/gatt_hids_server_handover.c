/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "service_handle.h"
#include "gatt_hids_server.h"
#include "gatt_hids_server_handover.h"
#include "gatt_hids_server_private.h"
#include "gatt_hids_server_debug.h"
#include "gatt_hids_server_access.h"

static struct hidsServerConverter_tag
{
    CsrBtMarshalUtilInst *conv;
    HidsData *hidsHandoverData; /* Used during unmarshal */
    CsrBtConnId cid; /* Used during unmarshal */
} *hidsServerConverter;

static void gattHidsServerConvReports(CsrBtMarshalUtilInst *conv, HidsData* hidsData)
{
    uint8 index;

    for (index =0; index < MAX_NO_INPUT_REPORT; index++)
    {
        CsrBtMarshalUtilConvertObj(conv, hidsData->inputReport[index].dataLen);
        if (hidsData->inputReport[index].dataLen)
        {
            if (hidsData->inputReport[index].data == NULL)
            {   /* Allocate memory during unmarshal */
                hidsData->inputReport[index].data = CsrPmemZalloc(hidsData->inputReport[index].dataLen);
            }
            CsrBtMarshalUtilConvert(conv, hidsData->inputReport[index].data, hidsData->inputReport[index].dataLen);
        }
        CsrBtMarshalUtilConvertObj(conv, hidsData->inputReport[index].reportId);
    }

    for (index =0; index < MAX_NO_FEATURE_REPORT; index++)
    {
        CsrBtMarshalUtilConvertObj(conv, hidsData->featureReport[index].dataLen);
        if (hidsData->featureReport[index].dataLen)
        {
            if (hidsData->featureReport[index].data == NULL)
            {   /* Allocate memory during unmarshal */
                hidsData->featureReport[index].data = CsrPmemZalloc(hidsData->featureReport[index].dataLen);
            }
            CsrBtMarshalUtilConvert(conv, hidsData->featureReport[index].data, hidsData->featureReport[index].dataLen);
        }
        CsrBtMarshalUtilConvertObj(conv, hidsData->featureReport[index].reportId);
    }
}

static void gattHidsServerConvReportMapData(CsrBtMarshalUtilInst *conv, HidsData* hidsData)
{
    uint8 index;

    for (index =0; index < hidsData->totalNumberOfReport; index++)
    {
        CsrBtMarshalUtilConvertObj(conv, hidsData->RMapData[index].reportId);
        CsrBtMarshalUtilConvertObj(conv, hidsData->RMapData[index].reportType);
        CsrBtMarshalUtilConvertObj(conv, hidsData->RMapData[index].reportLen);
    }
}

static void gattHidsServerConvData(CsrBtMarshalUtilInst *conv,
                                   HidsData* hidsData,
                                   GattHidsServerClientData* clientData)
{
        gattHidsServerConvReports(conv, hidsData);
        CsrBtMarshalUtilConvertObj(conv, hidsData->totalNumberOfReport);
        gattHidsServerConvReportMapData(conv, hidsData);
        CsrBtMarshalUtilConvertObj(conv, hidsData->controlPoint);
        CsrBtMarshalUtilConvertObj(conv, hidsData->usagePage);
        CsrBtMarshalUtilConvertObj(conv, clientData->clientCfg);
}

static void gattHidsServerCommitHandoverData(HidsData* hidsData, HidsData* handoverHidsData)
{
    uint8 index;

    for (index =0; index < MAX_NO_INPUT_REPORT ; index++)
    {
            hidsData->inputReport[index] = handoverHidsData->inputReport[index];
            hidsData->inputReport[index].reportType = INPUT_REPORT;
    }

    for (index =0; index < MAX_NO_FEATURE_REPORT ; index++)
    {
            hidsData->featureReport[index] = handoverHidsData->featureReport[index];
            hidsData->featureReport[index].reportType = FEATURE_REPORT;
    }

    for (index =0; index < handoverHidsData->totalNumberOfReport ; index++)
    {
            hidsData->RMapData[index] = handoverHidsData->RMapData[index];
    }

    hidsData->controlPoint = handoverHidsData->controlPoint;
    hidsData->usagePage = handoverHidsData->usagePage;
    hidsData->totalNumberOfReport = handoverHidsData->totalNumberOfReport;
}

static void gattHidsServerHandoverCleanup(HidsData* hidsData, CsrBtConnId cid)
{
    HidsClientDataElement * clientDataElement;

    clientDataElement = hidsFindClient(&hidsData->connectedClients, cid);
    
    if (clientDataElement)
    {
        HIDS_REMOVE_CLIENT(hidsData->connectedClients, clientDataElement);
    }
}

CsrBool gattHidsServerHandoverVeto(ServiceHandle hidsServiceHandle)
{
    CsrBool veto = FALSE;
    GHIDS *hidsServerInstance = (GHIDS*)ServiceHandleGetInstanceData(hidsServiceHandle);

    if (hidsServerInstance)
    {
        if (SynergySchedMessagesPendingForTask(CSR_BT_HIDS_SERVER_IFACEQUEUE, NULL) != 0)
        {
            /* If there are pending messages in the HIDS server queue, veto handover. */
            veto = TRUE;
        }
        
    }
    else
    {
         /* HIDS Server Instance not found */
         GATT_HIDS_SERVER_WARNING("gattHidsServerHandoverVeto, HIDS Server Instance not found");
    }

    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverVeto %d", veto);

    return veto;
}

CsrBool gattHidsServerHandoverMarshal(ServiceHandle hidsServiceHandle,
                                      CsrBtConnId cid,
                                      CsrUint8 *buf,
                                      CsrUint16 length,
                                      CsrUint16 *written)
{
    HidsClientDataElement *hidsClient;
    GHIDS *hidsServerInstance = (GHIDS*)ServiceHandleGetInstanceData(hidsServiceHandle);
    *written = 0;

    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverMarshal cid=0x%04X", cid);

    if (hidsServerInstance)
    {
        hidsClient = hidsFindClient(&hidsServerInstance->data.connectedClients, cid);

        /* Marshal only if client exists */
        if (hidsClient)
        {   
            if (!hidsServerConverter)
            {
                hidsServerConverter = CsrPmemZalloc(sizeof(*hidsServerConverter));
                hidsServerConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
            }

            CsrBtMarshalUtilResetBuffer(hidsServerConverter->conv, length, buf, TRUE);

            gattHidsServerConvData(hidsServerConverter->conv, &hidsServerInstance->data, &hidsClient->clientData);

            *written = length - CsrBtMarshalUtilRemainingLengthGet(hidsServerConverter->conv);

            return CsrBtMarshalUtilStatus(hidsServerConverter->conv);
        }
        else
        {   /* HIDS Server Client not found" */
            GATT_HIDS_SERVER_WARNING("gattHidsServerHandoverMarshal, Client with cid(0x%04X) not found", cid);
        }
    }
    else
    {   /* HIDS Server Instance not found" */
        GATT_HIDS_SERVER_WARNING("gattHidsServerHandoverMarshal, HIDS Server Instance not found");
    }

    return TRUE;
}

CsrBool gattHidsServerHandoverUnmarshal(ServiceHandle hidsServiceHandle,
                                        CsrBtConnId cid,
                                        const CsrUint8 *buf,
                                        CsrUint16 length,
                                        CsrUint16 *written)
{
    HidsClientDataElement *hidsClient;
    GHIDS *hidsServerInstance = (GHIDS*)ServiceHandleGetInstanceData(hidsServiceHandle);

    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverUnmarshal cid=0x%04X", cid);

    if (hidsServerInstance)
    {
        if (!hidsServerConverter)
        {
            hidsServerConverter = CsrPmemZalloc(sizeof(*hidsServerConverter));
            hidsServerConverter->hidsHandoverData = CsrPmemZalloc(sizeof(*(hidsServerConverter->hidsHandoverData)));
            hidsServerConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(hidsServerConverter->conv, length, (void *) buf, TRUE);

        hidsClient = HIDS_ADD_CLIENT(hidsServerInstance->data.connectedClients);
        hidsClient->clientData.cid = hidsServerConverter->cid = cid;

        gattHidsServerConvData(hidsServerConverter->conv, hidsServerConverter->hidsHandoverData, &hidsClient->clientData);

        *written = length - CsrBtMarshalUtilRemainingLengthGet(hidsServerConverter->conv);

        return CsrBtMarshalUtilStatus(hidsServerConverter->conv);
    }
    else
    {   /* HIDS Server Instance not found" */
        GATT_HIDS_SERVER_WARNING("gattHidsServerHandoverUnmarshal, HIDS Server Instance not found");
    }

    return TRUE;
}


void gattHidsServerHandoverCommit(ServiceHandle hidsServiceHandle,
                                  CsrBtConnId cid,
                                  const bool newPrimary)
{
    GHIDS *hidsServerInstance = (GHIDS*)ServiceHandleGetInstanceData(hidsServiceHandle);

    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverCommit cid=0x%04X", cid);

    if (hidsServerInstance)
    {
        if (newPrimary)
        {
            if (hidsServerConverter->cid == cid)
            {
                gattHidsServerCommitHandoverData(&hidsServerInstance->data,
                                                 hidsServerConverter->hidsHandoverData);
            }
        }
        else
        {   /* This is the new secondary EB */
            gattHidsServerHandoverCleanup(&hidsServerInstance->data, cid);
        }
    }
    else
    {   /* HIDS Server Instance not found" */
        GATT_HIDS_SERVER_WARNING("gattHidsServerHandoverCommit, HIDS Server Instance not found");
    }
}

void gattHidsServerHandoverComplete(ServiceHandle hidsServiceHandle, const bool newPrimary)
{
    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverComplete, newPrimary=%d",newPrimary);

    if (hidsServerConverter)
    {
        CsrBtMarshalUtilDestroy(hidsServerConverter->conv);
        if (hidsServerConverter->hidsHandoverData)
        {
            CsrPmemFree(hidsServerConverter->hidsHandoverData);
        }
        CsrPmemFree(hidsServerConverter);
        hidsServerConverter = NULL;
    }

    CSR_UNUSED(hidsServiceHandle);
    CSR_UNUSED(newPrimary);
}

void gattHidsServerHandoverAbort(ServiceHandle hidsServiceHandle)
{
    GHIDS *hidsServerInstance = (GHIDS*)ServiceHandleGetInstanceData(hidsServiceHandle);

    GATT_HIDS_SERVER_INFO("gattHidsServerHandoverAbort");

    if (hidsServerInstance)
    {
        if (hidsServerConverter && CsrBtMarshalUtilTypeGet(hidsServerConverter->conv) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
        {
            /* Resetting the info on secondary in case of handover abort */
            gattHidsServerHandoverCleanup(&hidsServerInstance->data, hidsServerConverter->cid);
        }
    }

    gattHidsServerHandoverComplete(hidsServiceHandle, FALSE);
}

