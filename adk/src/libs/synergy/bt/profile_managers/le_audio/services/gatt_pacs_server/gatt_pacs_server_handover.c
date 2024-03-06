/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "gatt_pacs_server.h"
#include "gatt_pacs_server_handover.h"
#include "gatt_pacs_server_private.h"

#ifdef CSR_LOG_ENABLE
#include "csr_log_text_2.h"
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(gattPacsServerLto);
#endif

#ifdef CSR_LOG_ENABLE
#define GATT_PACS_SERVER_LTSO_HANDOVER             0
#define GATT_PACS_SERVER_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((gattPacsServerLto, GATT_PACS_SERVER_LTSO_HANDOVER, __VA_ARGS__))
#define GATT_PACS_SERVER_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((gattPacsServerLto, GATT_PACS_SERVER_LTSO_HANDOVER, __VA_ARGS__))
#define GATT_PACS_SERVER_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((gattPacsServerLto, GATT_PACS_SERVER_LTSO_HANDOVER, __VA_ARGS__))
#else
#define GATT_PACS_SERVER_HANDOVER_LOG_INFO(...)
#define GATT_PACS_SERVER_HANDOVER_LOG_WARNING(...)
#define GATT_PACS_SERVER_HANDOVER_LOG_ERROR(...)
#endif

static struct
{
    CsrBtMarshalUtilInst *conv;
    CsrUint32 available_audio_contexts;
} *pacsServerConverter;

static pacs_client_data *gattPacsServerFindConnectionByCid(GPACSS_T *pacsServer, CsrBtConnId cid)
{
    CsrUint8 i;

    for (i=0; i < GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacsServer->data.connected_clients[i].cid == cid)
            return &pacsServer->data.connected_clients[i];
    }

    return NULL;
}

static pacs_client_data *gattPacsServerAddHandoverConnection(GPACSS_T *pacsServer, CsrBtConnId cid)
{
    CsrUint8 i;

    for (i=0; i < GATT_PACS_MAX_CONNECTIONS; i++)
    {
        if (pacsServer->data.connected_clients[i].cid == 0)
        {
            pacsServer->data.connected_clients[i].cid = cid;
            return &pacsServer->data.connected_clients[i];
        }
    }

    return NULL;
}

static void serPacsServerData(CsrBtMarshalUtilInst *conv,
                              GPACSS_T *pacsServer,
                              CsrBtConnId cid)
{
    CsrBool present;

    pacs_client_data *pacsClientData = gattPacsServerFindConnectionByCid(pacsServer, cid);

    present = pacsClientData ? TRUE : FALSE;

    CsrBtMarshalUtilConvertObj(conv, pacsServer->data.available_audio_contexts);
    CsrBtMarshalUtilConvertObj(conv, pacsServer->data.audioContextAvailabiltyControlApp);

    /* Signifies whether connection instance is present or not */
    CsrBtMarshalUtilConvertObj(conv, present);

    if (present)
    {
        CsrBtMarshalUtilConvertObj(conv, pacsClientData->client_cfg);
        CsrBtMarshalUtilConvertObj(conv, pacsClientData->selectiveAudioContexts);
    }
}

static void deserPacsServerData(CsrBtMarshalUtilInst *conv,
                                GPACSS_T *pacsServer,
                                CsrBtConnId cid)
{
    CsrBool present;

    pacs_client_data *pacsClientData = gattPacsServerFindConnectionByCid(pacsServer, cid);

    present = pacsClientData ? TRUE : FALSE;

    pacsServerConverter->available_audio_contexts = pacsServer->data.available_audio_contexts;

    CsrBtMarshalUtilConvertObj(conv, pacsServer->data.available_audio_contexts);
    CsrBtMarshalUtilConvertObj(conv, pacsServer->data.audioContextAvailabiltyControlApp);

    /* Find if connection instance is present or not */
    CsrBtMarshalUtilConvertObj(conv, present);

    if (present)
    {
        if (!pacsClientData)
        { /* In case pacs client data is already on the secondary or unmarshal resume case then no need to add again */
            pacsClientData = gattPacsServerAddHandoverConnection(pacsServer, cid);
        }

        if (pacsClientData)
        { /* Above add function can return NULL as well */
            CsrBtMarshalUtilConvertObj(conv, pacsClientData->client_cfg);
            CsrBtMarshalUtilConvertObj(conv, pacsClientData->selectiveAudioContexts);
        }
    }
}

CsrBool gattPacsServerHandoverVeto(void)
{
    CsrBool veto = FALSE;

    /* There can be a case to check for pending msg queue before vetoing the handover */
    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerVeto %d", veto);

    return veto;
}

CsrBool gattPacsServerHandoverMarshal(ServiceHandle pacsServiceHandle,
                                      CsrBtConnId cid,
                                      CsrUint8 *buf,
                                      CsrUint16 length,
                                      CsrUint16 *written)
{
    GPACSS_T *pacsServer = (GPACSS_T *) ServiceHandleGetInstanceData(pacsServiceHandle);

    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerMarshal");

    if (pacsServer)
    {
        if (!pacsServerConverter)
        {
            pacsServerConverter = CsrPmemZalloc(sizeof(*pacsServerConverter));
            pacsServerConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(pacsServerConverter->conv, length, buf, TRUE);

        serPacsServerData(pacsServerConverter->conv, pacsServer, cid);

        *written = length - CsrBtMarshalUtilRemainingLengthGet(pacsServerConverter->conv);

        return CsrBtMarshalUtilStatus(pacsServerConverter->conv);
    }

    return TRUE;
}

CsrBool gattPacsServerHandoverUnmarshal(ServiceHandle pacsServiceHandle,
                                        CsrBtConnId cid,
                                        const CsrUint8 *buf,
                                        CsrUint16 length,
                                        CsrUint16 *written)
{
    GPACSS_T *pacsServer = (GPACSS_T *) ServiceHandleGetInstanceData(pacsServiceHandle);

    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerUnmarshal");

    if (pacsServer)
    {
        if (!pacsServerConverter)
        {
            pacsServerConverter = CsrPmemZalloc(sizeof(*pacsServerConverter));
            pacsServerConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(pacsServerConverter->conv, length, (void *) buf, TRUE);

        deserPacsServerData(pacsServerConverter->conv, pacsServer, cid);

        *written = length - CsrBtMarshalUtilRemainingLengthGet(pacsServerConverter->conv);

        return CsrBtMarshalUtilStatus(pacsServerConverter->conv);
    }

    return TRUE;
}

void gattPacsServerHandoverCommit(ServiceHandle pacsServiceHandle,
                                  CsrBtConnId cid,
                                  const bool newPrimary)
{
    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerHandoverCommit");

    CSR_UNUSED(pacsServiceHandle);
    CSR_UNUSED(cid);
    CSR_UNUSED(newPrimary);
}

void gattPacsServerHandoverComplete(ServiceHandle pacsServiceHandle, const bool newPrimary)
{
    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerHandoverComplete");

    if (pacsServerConverter)
    {
        CsrBtMarshalUtilDestroy(pacsServerConverter->conv);
        CsrPmemFree(pacsServerConverter);
        pacsServerConverter = NULL;
    }

    CSR_UNUSED(pacsServiceHandle);
    CSR_UNUSED(newPrimary);
}

void gattPacsServerHandoverAbort(ServiceHandle pacsServiceHandle)
{
    GPACSS_T *pacsServer = (GPACSS_T *) ServiceHandleGetInstanceData(pacsServiceHandle);

    GATT_PACS_SERVER_HANDOVER_LOG_INFO("PacsServerHandoverAbort");

    if (pacsServer)
    {
        if (pacsServerConverter && CsrBtMarshalUtilTypeGet(pacsServerConverter->conv) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
        {
            /* Resetting the info on secondary in case of handover abort */
            pacsServer->data.available_audio_contexts = pacsServerConverter->available_audio_contexts;

            CsrMemSet(pacsServer->data.connected_clients, 0, (sizeof(pacs_client_data) * GATT_PACS_MAX_CONNECTIONS));
        }
    }

    gattPacsServerHandoverComplete(pacsServiceHandle, FALSE);
}
