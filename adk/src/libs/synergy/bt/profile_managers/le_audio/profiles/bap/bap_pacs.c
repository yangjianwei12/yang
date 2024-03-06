/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bap_client_lib.h"
#include "tbdaddr.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"
#include "bap_connection.h"

void BapInitReq(phandle_t appHandle,
                BapInitData initData)
{
    BapInternalInitReq *pPrim = CsrPmemZalloc(sizeof(BapInternalInitReq));

    pPrim->type = BAP_INTERNAL_INIT_REQ;
    pPrim->phandle = appHandle;
    pPrim->initData.cid = initData.cid;
    pPrim->initData.role = initData.role;
    pPrim->initData.handles = initData.handles;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapDeinitReq(BapProfileHandle handle,
                  BapRole role)
{
    BapInternalDeinitReq *pPrim = CsrPmemZalloc(sizeof(BapInternalDeinitReq));

    pPrim->type = BAP_INTERNAL_DEINIT_REQ;
    pPrim->handle = handle;
    pPrim->role = role;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
void BapAddCodecRecordReq(phandle_t appHandle,
                          BapPacLocalRecord *pacRecord)
{
    BapInternalAddPacRecordReq *pPrim = CsrPmemZalloc(sizeof(BapInternalAddPacRecordReq));

    pPrim->type = BAP_INTERNAL_ADD_PAC_RECORD_REQ;
    pPrim->phandle = appHandle;
    pPrim->pacRecord = pacRecord;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapRemoveCodecRecordReq(phandle_t appHandle,
                             uint16 pacRecordId)
{
    BapInternalRemovePacRecordReq *pPrim = CsrPmemZalloc(sizeof(BapInternalRemovePacRecordReq));

    pPrim->type = BAP_INTERNAL_REMOVE_PAC_RECORD_REQ;
    pPrim->phandle = appHandle;
    pPrim->pacRecordId = pacRecordId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapDiscoverAudioRoleReq(BapProfileHandle handle,
                             BapPacRecordType recordType)
{
    BapInternalDiscoverAudioRoleReq *pPrim = CsrPmemZalloc(sizeof(BapInternalDiscoverAudioRoleReq));

    pPrim->type = BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ;
    pPrim->handle = handle;
    pPrim->recordType = recordType;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapDiscoverRemoteAudioCapabilityReq(BapProfileHandle handle,
                                         BapPacRecordType recordType)
{
    BapInternalDiscoverRemoteAudioCapabilityReq *pPrim = CsrPmemZalloc(sizeof(BapInternalDiscoverRemoteAudioCapabilityReq));

    pPrim->type = BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ;
    pPrim->handle = handle;
    pPrim->recordType = recordType;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapRegisterPacsNotificationReq(BapProfileHandle handle,
                                    BapPacsNotificationType notifyType,
                                    bool notifyValue)
{
    BapInternalRegisterPacsNotificationReq* pPrim = CsrPmemZalloc(sizeof(BapInternalRegisterPacsNotificationReq));

    pPrim->type = BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ;
    pPrim->handle = handle;
    pPrim->notifyType = notifyType;
    pPrim->notifyEnable = notifyValue;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapGetRemoteAudioLocationReq(BapProfileHandle handle,
                                  BapPacRecordType recordType)
{
    BapInternalGetRemoteAudioLocationReq *pPrim = CsrPmemZalloc(sizeof(BapInternalGetRemoteAudioLocationReq));

    pPrim->type = BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ;
    pPrim->handle = handle;
    pPrim->recordType = recordType;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapSetRemoteAudioLocationReq(BapProfileHandle handle,
                                  BapPacRecordType recordType,
                                  BapAudioLocation location)
{
    BapInternalSetRemoteAudioLocationReq *pPrim = CsrPmemZalloc(sizeof(BapInternalSetRemoteAudioLocationReq));

    pPrim->type = BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ;
    pPrim->handle = handle;
    pPrim->recordType = recordType;
    pPrim->location = location;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapDiscoverAudioContextReq(BapProfileHandle handle,
                                BapPacAudioContext contextType)
{
    BapInternalDiscoverAudioContextReq *pPrim = CsrPmemZalloc(sizeof(BapInternalDiscoverAudioContextReq));

    pPrim->type = BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ;
	pPrim->handle = handle;
    pPrim->context = contextType;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

bool BapDiscoverPacsAudioRoleReq(BapProfileHandle handle,
                             BapPacRecordType recordType)
{

    BAP* bap = bapGetInstance();
    BapConnection* connection = NULL;
    bool pacsRole = FALSE;

    if (bapClientFindConnectionByCid(bap, handle, &connection))
    {
        GattPacsClientType direction = (recordType == BAP_AUDIO_SINK_RECORD) ?
            GATT_PACS_CLIENT_SINK : GATT_PACS_CLIENT_SOURCE;

        pacsRole = GattPacsClientFindAudioRoleReq(connection->pacs.srvcHndl, direction);
    }

    return pacsRole;
}
#endif
