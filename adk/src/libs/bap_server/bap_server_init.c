/****************************************************************************
* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include <gatt_ascs_server.h>
#include <gatt_pacs_server.h>
#include <gatt_bass_server.h>

#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_msg_handler.h"
#include "bap_server.h"
#include "connection_no_ble.h"
#include <panic.h>

static bapProfileHandle BapProfilHandle = BAP_INVALID_HANDLE;

bool bapServerAddConnectionIdToList(BAP *bapInst,
                                    ConnId connectionId)
{
    uint8 i;
    for(i=0; i< BAP_SERVER_MAX_CONNECTIONS ; i++)
    {
        if(bapInst->cid[i] == connectionId)
        {
            return TRUE;
        }
    }
    for(i=0; i< BAP_SERVER_MAX_CONNECTIONS ; i++)
    {
        if( bapInst->cid[i] == 0)
        {
            bapInst->cid[i] = connectionId;
            return TRUE;
        }
    }
    return FALSE;
}

void bapServerAddConfigToConnection(BAP *bapInst, ConnId connectionId)
{
    uint8 i;

    for(i=0; i< BAP_SERVER_MAX_CONNECTIONS ; i++)
    {
        if(bapInst->cid[i] == connectionId)
        {
            bapInst->numConfigInstance[i]++;
            return;
        }
    }
}

void bapServerRemoveConnectionIdFromList(BAP *bapInst, ConnId connectionId)
{
    uint8 i;

    for(i=0; i< BAP_SERVER_MAX_CONNECTIONS ; i++)
    {
        if( bapInst->cid[i] == connectionId)
        {
            if(bapInst->numConfigInstance[i] > 0)
            {
                bapInst->numConfigInstance[i]--;
                if(bapInst->numConfigInstance[i] == 0)
                {
                    bapInst->cid[i] = 0;
                }
            }
            return;
        }
    }
}

ServiceHandle bapServerPacsUtilitiesGetPacsInstance(void)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) BapProfilHandle);

    if((bapInst) && (bapInst->pacsHandle != 0))
    {
        return bapInst->pacsHandle;
    }
    return 0;
}

bapProfileHandle bapServerGetBapInstance(void)
{
    return BapProfilHandle;
}

uint8 BapServerUnicastGetMaxSupportedAses(void)
{
    return BAP_SERVER_MAX_NUM_ASES;
}

bapProfileHandle BapServerUnicastInit(Task appTask, uint8 numAses,
                                const BapServerHandleRange *pacsHandles,
                                const BapServerHandleRange *ascsHandles)
{
    UNUSED(appTask);
    UNUSED(numAses);
    UNUSED(pacsHandles);
    UNUSED(ascsHandles);
    Panic();
    return 0;
}

bapProfileHandle BapServerBroadcastInit(Task appTask, uint8 numberBroadcastSources,
                                        const BapServerHandleRange *pacsHandles,
                                        const BapServerHandleRange *bassHandles)
{
    BAP *bapInst = NULL;
    
    if (appTask == NULL)
    {
        BAP_DEBUG_PANIC(("PANIC: BapServerBroadcastInit Application Task NULL\n"));
    }

    if(BapProfilHandle == BAP_INVALID_HANDLE)
    {
        BapProfilHandle = (bapProfileHandle) ServiceHandleNewInstance((void **) &bapInst, sizeof(BAP));
        if(bapInst)
        {
            memset(bapInst, 0, sizeof(BAP));
            memset(bapInst->cid, 0, BAP_SERVER_MAX_CONNECTIONS);
            memset(bapInst->numConfigInstance, 0, BAP_SERVER_MAX_CONNECTIONS);
            bapInst->libTask.handler = BapServerMsgHandler;
        }
    }

    if (BapProfilHandle)
    {
       if(bapInst == NULL)
           bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(BapProfilHandle);

        /* Store the Task function parameter */
        bapInst->appBroadcastTask = appTask;
        bapInst->appSinkTask = appTask;

        if(bapInst->appUnicastTask == 0)
        {
            bapInst->appUnicastTask = NULL;
        }
        bapInst->profileHandle = BapProfilHandle;

        bapInst->bassHandle = GattBassServerInit(&bapInst->libTask,
                                                 bassHandles->startHandle,
                                                 bassHandles->endHandle,
                                                 numberBroadcastSources);
        if( bapInst->bassHandle == 0)
        {
            ServiceHandleFreeInstanceData(BapProfilHandle);
            return 0;
        }

        if (bapInst->pacsHandle == 0)
        {
            bapInst->pacsHandle = GattPacsServerInit(&bapInst->libTask,
                                                     pacsHandles->startHandle,
                                                     pacsHandles->endHandle);
            if( bapInst->pacsHandle == 0)
            {
                ServiceHandleFreeInstanceData(BapProfilHandle);
                return 0;
            }
        }
        /* Register for ISOC broadcast */
        ConnectionIsocRegister(&bapInst->libTask,
                                            BAP_SERVER_ISOC_TYPE_BROADCAST );
    }
    else
    {
        BAP_DEBUG_PANIC(("BapServer: BapServerBroadcastInit\n\n"));
    }
    
    BAP_DEBUG_INFO(("BapServer: BapServerBroadcastInit BapProfilHandle %d\n\n", BapProfilHandle));
    return BapProfilHandle;
}


void BapServerUnicastAseReceiveStartReadyResponse(bapProfileHandle profileHandle,
                                                  ConnId connectionId,
                                                  uint8 numAses,
                                                  const uint8 *aseIds)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(aseIds);
}

void BapServerUnicastAseReleased(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 uint8 numAses,
                                 const uint8 *aseIds,
                                 bool cacheCodecEnable)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(aseIds);
    UNUSED(cacheCodecEnable);
}

bool BapServerUnicastAseConfigureCodecReq(bapProfileHandle profileHandle, 
                                          const BapServerAseConfigCodecReq *aseCodecInfo)
{
    UNUSED(profileHandle);
    UNUSED(aseCodecInfo);
    return FALSE;
}

bool BapServerUnicastAseReceiveStartReadyReq(bapProfileHandle profileHandle,
                                             ConnId connectionId,
                                             uint8 numAses,
                                             const uint8 *aseIds)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(aseIds);

    return FALSE;
}

bool BapServerUnicastAseDisableReq(bapProfileHandle profileHandle,
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds,
                                   bool cisLoss)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(aseIds);
    UNUSED(cisLoss);

    return TRUE;
}

bool BapServerUnicastAseReleaseReq(bapProfileHandle profileHandle, 
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(aseIds);

    return TRUE;
}

bool BapServerUnicastAseEnableRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(numAses);
    UNUSED(bapServerAseResult);

    return TRUE;
}

BapServerAseCodecInfo * BapServerUnicastReadAseCodecConfiguration(bapProfileHandle profileHandle,
                                                                  ConnId connectionId,
                                                                  uint8 aseId)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(aseId);

    return NULL;
}

BapServerAseQosInfo * BapServerUnicastReadAseQosConfiguration(bapProfileHandle profileHandle,
                                                              ConnId connectionId,
                                                              uint8 aseId)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(aseId);

    return NULL;
}

AseDirectionType  BapServerUnicastReadAseDirection(bapProfileHandle profileHandle,
                                                   ConnId connectionId,
                                                   uint8 aseId)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(aseId);

    return ASE_DIRECTION_UNINITIALISED;
}

bapStatus BapServerUnicastAddAscsConfig(bapProfileHandle profileHandle,
                                       ConnId connectionId,
                                       const BapAscsConfig * config)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(config);
    return BAP_SERVER_STATUS_SUCCESS;
}

BapAscsConfig * BapServerUnicastRemoveAscsConfig(bapProfileHandle profileHandle,
                                                 ConnId connectionId)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    
    return NULL;
}

bapStatus BapServerAddBassConfig(bapProfileHandle profileHandle,
                                ConnId connectionId,
                                BapBassConfig * config)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    bapStatus status = BAP_SERVER_STATUS_SUCCESS;
    
    if ((bapInst) && bapServerAddConnectionIdToList(bapInst, connectionId))
    {
        if( GattBassServerAddConfig(bapInst->bassHandle,
                                         connectionId,
                                         (GattBassServerConfig*) config) != gatt_status_success)
        {
            status = BAP_SERVER_STATUS_FAILED;
            BAP_DEBUG_PANIC(("PANIC: BapServerAddBassConfig add PACS config failed status=%d", status));
            return status;

        }
        bapServerAddConfigToConnection(bapInst, connectionId);
    }
    return status;
}

BapBassConfig * BapServerRemoveBassConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapBassConfig * config = NULL;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        config = (BapBassConfig *)GattBassServerRemoveConfig(bapInst->bassHandle,
                                                             connectionId);
        BAP_DEBUG_INFO(("BapServerGattDisconnected cid=0x%x", connectionId));
        bapServerRemoveConnectionIdFromList(bapInst, connectionId);
    }
    return config;
}

bool BapServerValidateStreamingContext(bapProfileHandle profileHandle, 
                                      ConnId connectionId,uint8 aseId,
                                      uint8 * metadata, uint8 *metadataLength)
{
    UNUSED(profileHandle);
    UNUSED(connectionId);
    UNUSED(aseId);
    UNUSED(metadata);
    UNUSED(metadataLength);

    return FALSE;
}
