/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#include "scan_delegator_role.h"
#include "scan_delegator_role_sync.h"
#include "scan_delegator_role_advertising.h"

#include <connection.h>
#include <gatt_connect.h>
#include <gatt_handler_db_if.h>
#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <stdio.h>
#include "pacs_utilities.h"

#ifdef USE_SYNERGY
#include "cm_lib.h"
#include "gatt_lib.h"
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

#define SCAN_DELEGATOR_ROLE_LOG     DEBUG_LOG

#define MS_TO_SYNC_TIMEOUT(ms)                              ((ms)/10)

#define SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SKIP             0x0000
#define SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SYNC_TIMEOUT_MS  4000
#define SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SYNC_MODE        0x02
#define SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_CTE_TYPE         0x00

#ifdef USE_SYNERGY
extern void LeBapBroadcastSink_MessageHandleCmPrim(void* message);
#endif
extern void LeBapBroadcastSink_MessageHandleBapPrim(BapServerBigInfoAdvReportInd *message);

static ServiceHandle bap_handle = 0;
const LeBapScanDelegator_callback_interface_t * scan_delegator_registered_callbacks = NULL;

static void leBapScanDelegator_BassMessageHandler(Task task, MessageId id, Message message);
static const TaskData bass_task = { .handler = leBapScanDelegator_BassMessageHandler };

static void leBapScanDelegator_HandlePeriodicSyncTransferInd(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_IND_T * ind);
static void leBapScanDelegator_OnGattConnect(gatt_cid_t cid);
static void leBapScanDelegator_OnGattDisconnect(gatt_cid_t cid);
static void leBapScanDelegator_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);
static void leBapScanDelegator_VerifyCallbacksArePresent(const LeBapScanDelegator_callback_interface_t * callbacks_to_verify);
static void leBapScanDelegator_FreeBroadcastSourceSubgroupsState(scan_delegator_server_get_broadcast_source_state_t *source_state);

static const gatt_connect_observer_callback_t le_connect_callbacks =
{
    .OnConnection = leBapScanDelegator_OnGattConnect,
    .OnDisconnection = leBapScanDelegator_OnGattDisconnect,
    .OnEncryptionChanged = leBapScanDelegator_OnEncryptionChanged
};


ServiceHandle leBapScanDelegator_GetBapProfileHandle(void)
{
    PanicZero(bap_handle);
    return bap_handle;
}

#ifdef USE_SYNERGY
static bool leBapScanDelegator_HandleCmPrim(void* message)
{
    bool status = FALSE;
    CsrBtCmPrim *primType = (CsrBtCmPrim *) message;

    switch (*primType)
    {
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND:
        {
            CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_IND_T ind;
            ind.status = ((CmPeriodicScanSyncTransferInd *)message)->resultCode;
            ind.adv_sid = ((CmPeriodicScanSyncTransferInd *)message)->advSid;
            ind.sync_handle = ((CmPeriodicScanSyncTransferInd *)message)->syncHandle;
            ind.service_data = ((CmPeriodicScanSyncTransferInd *)message)->serviceData;
            ind.adv_addr.type = ((CmPeriodicScanSyncTransferInd *)message)->addrt.type;
            ind.adv_addr.addr.uap = ((CmPeriodicScanSyncTransferInd *)message)->addrt.addr.uap;
            ind.adv_addr.addr.nap = ((CmPeriodicScanSyncTransferInd *)message)->addrt.addr.nap;
            ind.adv_addr.addr.lap = ((CmPeriodicScanSyncTransferInd *)message)->addrt.addr.lap;
            leBapScanDelegator_HandlePeriodicSyncTransferInd(&ind);
            status = TRUE;
        }
            break;
        default:
            break;
    }

    return status;
}

static void leBapScanDelegator_BassMessageCmPrimHandler(void* message)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *) message;

    switch (*primType)
    {
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND:
            LeBapBroadcastSink_MessageHandleCmPrim(message);
            break;
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND:
            leBapScanDelegator_HandleCmPrim(message);
            break;
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM:
            {
                /* There are scenarios where this SYNC_TRANSFER_PARAMS_REQ will fail
                 * due to no connection with the remote device. This happend due to
                 * race condition between the SYNC_TRANSFER_PARAMS_REQ command getting triggered
                 * and received disconnect indication for the remote device */
                CmPeriodicScanSyncTransferParamsCfm *cfm = (CmPeriodicScanSyncTransferParamsCfm *)message;

                SCAN_DELEGATOR_ROLE_LOG("CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM resultCode 0x%x addr[%x %x:%x:%lx]",
                                        cfm->resultCode, cfm->addrt.type,
                                        cfm->addrt.addr.nap, cfm->addrt.addr.uap, cfm->addrt.addr.lap);

                PanicFalse((cfm->resultCode == success) || (cfm->resultCode == hci_error_no_connection));
            }
            break;
        default:
            Panic();
            break;
    }
}

static void leBapScanDelegator_HandleConfigChangeInd(BapServerConfigChangeInd* ind)
{
    BapBassConfig stored_config = {0};
    BapBassConfig *service_config = NULL;
    uint8 *data = NULL;
    uint8 size;
    bool needs_update = FALSE;
    int scan;

    if (scan_delegator_registered_callbacks)
    {
        service_config = (BapBassConfig *)BapServerGetServiceConfig(bap_handle, ind->connectionId, ind->configType);
        data = (uint8 *) scan_delegator_registered_callbacks->LeBapScanDelegator_RetrieveClientConfig(ind->connectionId);

        if (data != NULL)
        {
            stored_config = *(BapBassConfig *) data;
            stored_config.receiveStateCcc = (uint16 *) &data[offsetof(BapBassConfig, receiveStateCcc)];
        }

        if (service_config != NULL && service_config->receiveStateCcc != NULL)
        {
            if (service_config->receiveStateCccSize == stored_config.receiveStateCccSize)
            {
                for (scan = 0; scan < service_config->receiveStateCccSize; scan++)
                {
                    if (service_config->receiveStateCcc[scan] != stored_config.receiveStateCcc[scan])
                    {
                        needs_update = TRUE;
                        break;
                    }
                }
            }
            else
            {
                needs_update = TRUE;
            }

            if (needs_update)
            {
                size = sizeof(scan_delegator_config_t) + (sizeof(uint16) * service_config->receiveStateCccSize);
                scan_delegator_config_t * delegator_config = PanicUnlessMalloc(size);
                delegator_config->receive_state_ccc_size = service_config->receiveStateCccSize;

                if (service_config->receiveStateCccSize)
                {
                    for (scan = 0; scan < service_config->receiveStateCccSize; scan++)
                    {
                        delegator_config->receive_state_ccc[scan] = service_config->receiveStateCcc[scan];
                    }
                }

                scan_delegator_registered_callbacks->LeBapScanDelegator_StoreClientConfig(ind->connectionId, (void *)delegator_config, size);

                DEBUG_LOG_INFO("leBapScanDelegator_BassMessageHandler BAP_SERVER_CONFIG_CHANGE_IND storing configType %d, size %d", ind->configType, size);
                pfree(delegator_config);
            }
            pfree(service_config->receiveStateCcc);
        }
        pfree(service_config);
    }
}
#endif

static void leBapScanDelegator_BassMessageHandler(Task task, MessageId id, Message message)
{
    uint16 msgId = 0;

    UNUSED(task);

#ifdef USE_SYNERGY

    if(!message)
    {
        SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_BassMessageHandler : It is null message");
        Panic();
    }

    if ((id == CM_PRIM) || (id == BAP_SRVR_PRIM))
    {
        msgId = *((CsrBtCmPrim *) message);
    }
    else
    {
        msgId = *((uint8*) message);
    }

    SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_BassMessageHandler : ID 0x%04X, MSG ID 0x%04X", id, msgId);

    switch(msgId)

#else /* USE_SYNERGY */

    if(!message)
    {
        SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_BassMessageHandler : It is null message");
        Panic();
    }

    SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_BassMessageHandler : MSG:0x%X", id);

    switch(id)

#endif /* USE_SYNERGY */
    {
        case BAP_SERVER_BASS_SCANNING_STATE_IND:
        {
            if(scan_delegator_registered_callbacks)
            {
                BapServerBassScanningStateInd *ind = (BapServerBassScanningStateInd *)message;
                if (ind->clientScanningState)
                {
                    scan_delegator_registered_callbacks->LeBapScanDelegator_RemoteScanningStart();
                }
                else
                {
                    scan_delegator_registered_callbacks->LeBapScanDelegator_RemoteScanningStop();
                }
            }
        }
        break;
        case BAP_SERVER_BASS_ADD_SOURCE_IND:
        {
            if(scan_delegator_registered_callbacks)
            {
                BapServerBassAddSourceInd *ind = (BapServerBassAddSourceInd *)message;
                scan_delegator_client_add_broadcast_source_t broadcastSource;
                broadcastSource.pa_sync = ind->source.paSync;
                broadcastSource.advertiser_address.type = ind->source.advertiserAddress.type;
                broadcastSource.advertiser_address.addr.nap = ind->source.advertiserAddress.addr.nap;
                broadcastSource.advertiser_address.addr.uap = ind->source.advertiserAddress.addr.uap;
                broadcastSource.advertiser_address.addr.lap = ind->source.advertiserAddress.addr.lap;
                broadcastSource.broadcast_id = ind->source.broadcastId;
                broadcastSource.source_adv_sid = ind->source.sourceAdvSid;
                broadcastSource.pa_interval = ind->source.paInterval;
                broadcastSource.num_subgroups = ind->source.numSubGroups;
                broadcastSource.subgroups = (le_bm_source_subgroup_t *)ind->source.subGroupsData;
                BdaddrTypedSetEmpty(&broadcastSource.assistant_address);
                tp_bdaddr tp_addr = {0};
                if (GattConnect_GetTpaddrFromConnectionId(ind->source.cid, &tp_addr))
                {
                    broadcastSource.assistant_address = tp_addr.taddr;
                }
                scan_delegator_registered_callbacks->LeBapScanDelegator_AddSource(&broadcastSource);

                LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(ind->source.numSubGroups, (le_bm_source_subgroup_t *)ind->source.subGroupsData);
            }
        }
        break;
        case BAP_SERVER_BASS_MODIFY_SOURCE_IND:
        {
            if(scan_delegator_registered_callbacks)
            {
                BapServerBassModifySourceInd *ind = (BapServerBassModifySourceInd *)message;
                scan_delegator_client_modify_broadcast_source_t broadcastSource;
                broadcastSource.source_id = ind->source.sourceId;
                broadcastSource.pa_sync = ind->source.paSyncState;
                broadcastSource.pa_interval = ind->source.paInterval;
                broadcastSource.num_subgroups = ind->source.numSubGroups;
                broadcastSource.subgroups = (le_bm_source_subgroup_t *)ind->source.subGroupsData;
                BdaddrTypedSetEmpty(&broadcastSource.assistant_address);
                tp_bdaddr tp_addr = {0};
                if (GattConnect_GetTpaddrFromConnectionId(ind->source.cid, &tp_addr))
                {
                    broadcastSource.assistant_address = tp_addr.taddr;
                }

                scan_delegator_registered_callbacks->LeBapScanDelegator_ModifySource(&broadcastSource);

                LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(ind->source.numSubGroups, (le_bm_source_subgroup_t *)ind->source.subGroupsData);
            }
        }
        break;
        case BAP_SERVER_BASS_BROADCAST_CODE_IND:
        {
            if(scan_delegator_registered_callbacks)
            {
                BapServerBassBroadcastCodeInd *ind = (BapServerBassBroadcastCodeInd *)message;
                scan_delegator_client_broadcast_code_t broadcastCode;
                broadcastCode.source_id = ind->code.sourceId;
                broadcastCode.broadcast_code= ind->code.broadcastCode;
                scan_delegator_registered_callbacks->LeBapScanDelegator_BroadcastCode(&broadcastCode);
                free(ind->code.broadcastCode);
            }
        }
        break;
        case BAP_SERVER_BASS_REMOVE_SOURCE_IND:
        {
            if(scan_delegator_registered_callbacks)
            {
                BapServerBassRemoveSourceInd *ind = (BapServerBassRemoveSourceInd *)message;
                scan_delegator_client_remove_broadcast_source_t broadcastSource;
                broadcastSource.source_id = ind->source.sourceId;
                scan_delegator_registered_callbacks->LeBapScanDelegator_RemoveSource(&broadcastSource);
            }
        }
        break;
        case BAP_SERVER_BIGINFO_ADV_REPORT_IND:
        {
            LeBapBroadcastSink_MessageHandleBapPrim((BapServerBigInfoAdvReportInd *) message);
        }
        break;

#ifdef USE_SYNERGY
        case BAP_SERVER_CONFIG_CHANGE_IND:
            {
                leBapScanDelegator_HandleConfigChangeInd((BapServerConfigChangeInd *) message);
            }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND:
        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM:
            leBapScanDelegator_BassMessageCmPrimHandler((void *) message);
        break;
#else
        case CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM:
            {
                CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM_T *cfm = (CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM_T *)message;

                SCAN_DELEGATOR_ROLE_LOG("CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM status 0x%x addr[%x %x:%x:%lx]",
                                        cfm->status, cfm->taddr.type,
                                        cfm->taddr.addr.uap, cfm->taddr.addr.nap, cfm->taddr.addr.lap);
                /*PanicFalse(((CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM_T *)message)->status == success);*/
            }
        break;
#endif

        default:
            SCAN_DELEGATOR_ROLE_LOG("Unhandled message ID 0x%04X, MSG ID 0x%04X", id, msgId);
        break;
    }
}

static void leBapScanDelegator_HandlePeriodicSyncTransferInd(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_IND_T * ind)
{
    if (ind)
    {
        SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_HandlePeriodicSyncTransferInd status=0x%x sync_handle=0x%x source_adv_sid=0x%x addr=[%x %x:%x:%lx]",
            ind->status,
            ind->sync_handle,
            ind->adv_sid,
            ind->adv_addr.type,
            ind->adv_addr.addr.nap,
            ind->adv_addr.addr.uap,
            ind->adv_addr.addr.lap
            );

        if(scan_delegator_registered_callbacks)
        {
            scan_delegator_periodic_sync_t sync;
            sync.status = (ind->status == success) ? scan_delegator_status_success : scan_delegator_status_failed;
            sync.sync_handle = ind->sync_handle;
            sync.taddr_source = ind->adv_addr;
            sync.source_adv_sid = ind->adv_sid;
            sync.service_data = ind->service_data;
            scan_delegator_registered_callbacks->LeBapScanDelegator_PeriodicSync(&sync);
        }
    }
    else
    {
        SCAN_DELEGATOR_ROLE_LOG("leBapScanDelegator_HandlePeriodicSyncTransferInd. NULL ind");
        Panic();
    }
}

static void leBapScanDelegator_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void leBapScanDelegator_OnGattDisconnect(gatt_cid_t cid)
{
    BapBassConfig * bass_config = BapServerRemoveBassConfig(bap_handle, cid);

    if (bass_config)
    {
        uint8 config_size = sizeof(scan_delegator_config_t) + (sizeof(uint16) * bass_config->receiveStateCccSize);
        scan_delegator_config_t * delegator_config = PanicUnlessMalloc(config_size);
        delegator_config->receive_state_ccc_size = bass_config->receiveStateCccSize;

        if (bass_config->receiveStateCccSize)
        {
            for (int i=0;i<bass_config->receiveStateCccSize;i++)
            {
                delegator_config->receive_state_ccc[i] = bass_config->receiveStateCcc[i];
            }
            free(bass_config->receiveStateCcc);
        }
        free(bass_config);

        if(scan_delegator_registered_callbacks)
        {
            scan_delegator_registered_callbacks->LeBapScanDelegator_StoreClientConfig(cid, (void *)delegator_config, config_size);
        }
        free(delegator_config);
    }
}

static void leBapScanDelegator_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        DEBUG_LOG("leBapScanDelegator_OnEncryptionChanged cid 0x%4x encrypted %d", cid, encrypted);
        scan_delegator_config_t * delegator_config = NULL;
        GattBassServerConfig * bass_config = NULL;

        if (scan_delegator_registered_callbacks)
        {
            delegator_config = (scan_delegator_config_t *)scan_delegator_registered_callbacks->LeBapScanDelegator_RetrieveClientConfig(cid);
            if (delegator_config)
            {
                bass_config = PanicUnlessMalloc(sizeof(GattBassServerConfig));
                bass_config->receiveStateCccSize = delegator_config->receive_state_ccc_size;
                if (delegator_config->receive_state_ccc_size)
                {
                    bass_config->receiveStateCcc = delegator_config->receive_state_ccc;
                }
            }
        }

        if (BapServerAddBassConfig(bap_handle, cid, (BapBassConfig *) bass_config) != BAP_SERVER_STATUS_SUCCESS)
        {
            Panic();
        }

        if (bass_config)
        {
            free(bass_config);
        }
    }
}

static void leBapScanDelegator_VerifyCallbacksArePresent(const LeBapScanDelegator_callback_interface_t * callbacks_to_verify)
{
    PanicNull((void *)callbacks_to_verify);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_RemoteScanningStart);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_RemoteScanningStop);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_AddSource);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_ModifySource);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_BroadcastCode);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_RemoveSource);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_PeriodicSync);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_RetrieveClientConfig);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_StoreClientConfig);
    PanicNull((void *)callbacks_to_verify->LeBapScanDelegator_GetTargetSyncState);
}

static void leBapScanDelegator_FreeBroadcastSourceSubgroupsState(scan_delegator_server_get_broadcast_source_state_t *source_state)
{
    uint8 i = 0;

    for (i=0; i<source_state->num_subgroups; i++)
    {
        if (source_state->subgroups[i].metadata_length)
        {
            free(source_state->subgroups[i].metadata);
        }
    }
    if (source_state->num_subgroups)
    {
        free(source_state->subgroups);
    }
}

void LeBapScanDelegator_Init(uint8 number_broadcast_sources, const LeBapScanDelegator_callback_interface_t * callbacks_to_register)
{
    BapServerHandleRange pacsServiceHandle = {HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE,
                                              HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE_END};
    BapServerHandleRange bassServiceHandle = {HANDLE_BASS_SERVICE, HANDLE_BASS_SERVICE_END};

    leBapScanDelegator_VerifyCallbacksArePresent(callbacks_to_register);

#ifdef USE_SYNERGY
    bap_handle = BapServerBroadcastInit(TrapToOxygenTask((Task)&bass_task),
                                         number_broadcast_sources,
                                         &pacsServiceHandle,
                                         &bassServiceHandle);
    DEBUG_LOG("LeBapScanDelegator_Init bap_handle=0x%x", bap_handle);
#else
    bap_handle = BapServerBroadcastInit((Task)&bass_task,
                                     number_broadcast_sources,
                                     &pacsServiceHandle,
                                     &bassServiceHandle);
#endif
    LeBapPacsSetBapHandle(bap_handle);

    LeBapScanDelegator_SetupLeAdvertisingData();
    LeBapScanDelegatorSync_Init();

    GattConnect_RegisterObserver(&le_connect_callbacks);

    scan_delegator_registered_callbacks = callbacks_to_register;
}

const LeBapScanDelegator_callback_interface_t* LeBapScanDelegator_RegisterCallbacks(const LeBapScanDelegator_callback_interface_t *callbacks_to_register)
{
    const LeBapScanDelegator_callback_interface_t *registered = scan_delegator_registered_callbacks;
    leBapScanDelegator_VerifyCallbacksArePresent(callbacks_to_register);
    scan_delegator_registered_callbacks = callbacks_to_register;
    return registered;
}

scan_delegator_status_t LeBapScanDelegator_AddBroadcastSourceState(uint8 *source_id, const scan_delegator_server_add_broadcast_source_state_t * source_state)
{
    scan_delegator_status_t result;
    BapServerBassReceiveState new_receive_state;
    
    if (source_state == NULL)
    {
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_AddBroadcastSourceState. NULL param");
        result = scan_delegator_status_failed;
    }
    else
    {
        memset(&new_receive_state, 0, sizeof(BapServerBassReceiveState));
        new_receive_state.paSyncState = source_state->pa_sync_state;
        new_receive_state.bigEncryption = source_state->big_encryption;
        new_receive_state.sourceAddress.type = source_state->source_address.type;
        new_receive_state.sourceAddress.addr.nap = source_state->source_address.addr.nap;
        new_receive_state.sourceAddress.addr.uap = source_state->source_address.addr.uap;
        new_receive_state.sourceAddress.addr.lap = source_state->source_address.addr.lap;
        new_receive_state.broadcastId = source_state->broadcast_id;
        new_receive_state.sourceAdvSid = source_state->source_adv_sid;
        new_receive_state.badCode = source_state->bad_code;
        new_receive_state.numSubGroups = source_state->num_subgroups;
        new_receive_state.subGroupsData = (GattBassServerSubGroupsData *)source_state->subgroups;

        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_AddBroadcastSourceState enum:scan_delegator_client_pa_sync_t:%d enum:scan_delegator_server_big_encryption_t:%d enum:scan_delegator_advertiser_address_type_t:%d addr[%x:%x:%lx] source_adv_sid[0x%x] broadcast_id[0x%x] num_subgroups[0x%x]",
                new_receive_state.paSyncState,
                new_receive_state.bigEncryption,
                new_receive_state.sourceAddress.type,
                new_receive_state.sourceAddress.addr.nap,
                new_receive_state.sourceAddress.addr.uap,
                new_receive_state.sourceAddress.addr.lap,
                new_receive_state.sourceAdvSid,
                new_receive_state.broadcastId,
                new_receive_state.numSubGroups
                );

        for (uint8 i=0; i<new_receive_state.numSubGroups; i++)
        {
            SCAN_DELEGATOR_ROLE_LOG("  index[%d] bis_sync[0x%x] meta_len[0x%x]",
                i,
                new_receive_state.subGroupsData[i].bisSync,
                new_receive_state.subGroupsData[i].metadataLen);
        }

        result = BapServerAddBroadcastSourceReq(bap_handle,
                                                     source_id,
                                                     &new_receive_state);
    }

    SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_AddBroadcastSourceState: result=enum:scan_delegator_status_t:%d", result);
        
    return result;
}

scan_delegator_status_t LeBapScanDelegator_ModifyBroadcastSourceState(uint8 source_id, const scan_delegator_server_modify_broadcast_source_state_t * source_state)
{
    scan_delegator_status_t result = scan_delegator_status_failed;
    BapServerBassReceiveState new_receive_state;
    
    memset(&new_receive_state, 0, sizeof(BapServerBassReceiveState));
    
    if (source_state == NULL)
    {
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_ModifyBroadcastSourceState. NULL param");
    }
    else if (BapServerGetBroadcastReceiveStateReq(bap_handle,
                                             source_id,
                                             &new_receive_state) == BAP_SERVER_STATUS_SUCCESS)
    {
        if (new_receive_state.badCode)
        {
            free(new_receive_state.badCode);
        }
        LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(new_receive_state.numSubGroups, (le_bm_source_subgroup_t *)new_receive_state.subGroupsData);
        
        new_receive_state.paSyncState = source_state->pa_sync_state;
        new_receive_state.bigEncryption = source_state->big_encryption;
        new_receive_state.badCode = source_state->bad_code;
        new_receive_state.numSubGroups = source_state->num_subgroups;
        new_receive_state.subGroupsData = (GattBassServerSubGroupsData *)source_state->subgroups;
        
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_ModifyBroadcastSourceState enum:scan_delegator_client_pa_sync_t:%d enum:scan_delegator_server_big_encryption_t:%d num_subgroups[0x%x]",
            new_receive_state.paSyncState,
            new_receive_state.bigEncryption,
            new_receive_state.numSubGroups
            );
            
        for (uint8 i=0; i<new_receive_state.numSubGroups; i++)
        {
            SCAN_DELEGATOR_ROLE_LOG("  index[%d] bis_sync[0x%x] meta_len[0x%x]",
                i,
                new_receive_state.subGroupsData[i].bisSync,
                new_receive_state.subGroupsData[i].metadataLen);
        }

        result = BapServerModifyBroadcastSourceReq(bap_handle,
                                                        source_id,
                                                        &new_receive_state);
                                                            
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_ModifyBroadcastSourceState. Modify BASS result=enum:scan_delegator_status_t:%d", result);

    }
    else
    {
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_ModifyBroadcastSourceState. Unknown Receive State with source_id %d", source_id);
        result = scan_delegator_status_invalid_source_id;
    }

    return result;
}

scan_delegator_status_t LeBapScanDelegator_RemoveBroadcastSourceState(uint8 source_id)
{
    scan_delegator_status_t result = BapServerRemoveBroadcastSourceReq(bap_handle, source_id);
                                                        
    SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_RemoveBroadcastSourceState. Remove BASS result=enum:scan_delegator_status_t:%d", result);
    
    return result;
}

scan_delegator_status_t LeBapScanDelegator_GetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    scan_delegator_status_t result = scan_delegator_status_failed;;
    BapServerBassReceiveState new_receive_state;
    
    memset(&new_receive_state, 0, sizeof(BapServerBassReceiveState));
    
    if (source_state == NULL)
    {
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_GetBroadcastSourceState. NULL param");
    }
    else if (BapServerGetBroadcastReceiveStateReq(bap_handle,
                                             source_id,
                                             &new_receive_state) == BAP_SERVER_STATUS_SUCCESS)
    {
        source_state->pa_sync_state = new_receive_state.paSyncState;
        source_state->big_encryption = new_receive_state.bigEncryption;
        source_state->source_address.type = new_receive_state.sourceAddress.type;
        source_state->source_address.addr.nap = new_receive_state.sourceAddress.addr.nap;
        source_state->source_address.addr.uap = new_receive_state.sourceAddress.addr.uap;
        source_state->source_address.addr.lap = new_receive_state.sourceAddress.addr.lap;
        source_state->broadcast_id = new_receive_state.broadcastId;
        source_state->source_adv_sid = new_receive_state.sourceAdvSid;
        source_state->bad_code = new_receive_state.badCode;
        source_state->num_subgroups = new_receive_state.numSubGroups;
        source_state->subgroups = (le_bm_source_subgroup_t *)new_receive_state.subGroupsData;
        
        result = scan_delegator_status_success;
    }
    else
    {
        SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_GetBroadcastSourceState. Unknown Receive State with source_id %d", source_id);
        result = scan_delegator_status_invalid_source_id;
    }
        
    return result;
}

void LeBapScanDelegator_FreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state)
{
    if (source_state->bad_code)
    {
        free(source_state->bad_code);
    }
    leBapScanDelegator_FreeBroadcastSourceSubgroupsState(source_state);
}

void LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(uint8 num_subgroups, le_bm_source_subgroup_t *subgroups_data)
{
    for (uint8 i=0; i<num_subgroups; i++)
    {
        if (subgroups_data[i].metadata_length)
        {
            free(subgroups_data[i].metadata);
        }
    }
    if (num_subgroups)
    {
        free(subgroups_data);
    }
}

uint8 * LeBapScanDelegator_GetBroadcastCode(uint8 source_id)
{
    return BapServerGetBroadcastCodeReq(bap_handle, source_id);
}

scan_delegator_status_t LeBapScanDelegator_SetBroadcastCode(uint8 source_id, uint8 * broadcast_code)
{
    scan_delegator_status_t result = BapServerSetBroadcastCodeReq(bap_handle, source_id, broadcast_code);
    
    SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_SetBroadcastCode: result=enum:scan_delegator_status_t:%d", result);
        
    return result;
}

uint8 * LeBapScanDelegator_GetBroadcastSourceIds(uint16 * number_source_id)
{
    PanicNull(number_source_id);
    uint8 * source_ids = BapServerGetSourceIdsReq(bap_handle, number_source_id);
    if(*number_source_id && source_ids == NULL)
    {
        Panic();
    }
    return source_ids;
}

bool LeBapScanDelegator_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    UNUSED(already_handled);

    bool handled = FALSE;

    switch(id)
    {
        case CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_IND:
            leBapScanDelegator_HandlePeriodicSyncTransferInd((CL_DM_BLE_PERIODIC_SCAN_SYNC_TRANSFER_IND_T *)message);
            handled = TRUE;
            break;
        default:
            break;
    }

    return handled;
}

bool LeBapScanDelegator_IsAnyClientConnected(void)
{
    return BapServerIsAnyClientConnected(bap_handle);
}

void LeBapScanDelegator_ConfigurePastForAddr(typed_bdaddr *taddr, bool enable)
{
    uint8 mode = 0x00;

    SCAN_DELEGATOR_ROLE_LOG("LeBapScanDelegator_ConfigurePastForAddr enable %d addr[%x %x:%x:%lx]",
                            enable, taddr->type, taddr->addr.nap, taddr->addr.uap, taddr->addr.lap);

    if (enable)
    {
        mode = SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SYNC_MODE;
    }

    ConnectionDmBlePeriodicScanSyncTransferParamsReq((Task)&bass_task,
                                                    *taddr,
                                                    SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SKIP,
                                                    MS_TO_SYNC_TIMEOUT(SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_SYNC_TIMEOUT_MS),
                                                    mode,
                                                    SCAN_DELEGATOR_SYNC_TRANSFER_PARAM_CTE_TYPE);
}

#ifdef HOSTED_TEST_ENVIRONMENT

const TaskData * LeBapScanDelegator_GetBassMessageHandler(void)
{
    return &bass_task;
}

#endif
