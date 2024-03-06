/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#include <gatt_ascs_server.h>
#include <gatt_pacs_server.h>
#include <gatt_bass_server.h>

#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_msg_handler.h"
#include "csr_bt_cm_lib.h"

#include "bap_server_lib.h"

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

void bapServerAddConfigToConnection(BAP *bapInst,
                                    ConnId connectionId)
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

void bapServerRemoveConnectionIdFromList(BAP *bapInst,
                                         ConnId connectionId)
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

void BapServerInit(void** gash)
{
    *gash = &BapProfilHandle;
}

#ifdef ENABLE_SHUTDOWN
void BapServerDeinit(void** gash)
{
    bapProfileHandle profileHandle = *((bapProfileHandle*)*gash);

    if(!ServiceHandleFreeInstanceData(profileHandle))
    {
        BAP_SERVER_DEBUG("Unable to free the BAP server instance\n");
    }
}
#endif

uint8 BapServerUnicastGetMaxSupportedAses(void)
{
    return BAP_SERVER_MAX_NUM_ASES;
}

bapProfileHandle BapServerUnicastInit(AppTask appTask, uint8 numAses,
                                      const BapServerHandleRange *pacsHandles,
                                      const BapServerHandleRange *ascsHandles)
{
    BAP *bapInst = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        BAP_SERVER_PANIC("PANIC: Application Task NULL\n");
    }

    if( numAses > BapServerUnicastGetMaxSupportedAses())
    {
        return BapProfilHandle;
    }

    if(BapProfilHandle == BAP_INVALID_HANDLE)
    {
        BapProfilHandle = (bapProfileHandle) ServiceHandleNewInstance((void **) &bapInst, sizeof(BAP));

        if(bapInst)
        {
            memset(bapInst, 0, sizeof(BAP));
            memset(bapInst->cid, 0, BAP_SERVER_MAX_CONNECTIONS);
            memset(bapInst->numConfigInstance, 0, BAP_SERVER_MAX_CONNECTIONS);
            bapInst->libTask = CSR_BT_BAP_SERVER_IFACEQUEUE;
        }
    }

    if (BapProfilHandle)
    {
        if(bapInst == NULL)
            bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(BapProfilHandle);

        if (bapInst != NULL)
        {
            /* Store the Task function parametera */
            bapInst->appUnicastTask = appTask;
            if (bapInst->appBroadcastTask == 0)
            {
                bapInst->appBroadcastTask = CSR_SCHED_QID_INVALID;
                bapInst->appSinkTask = CSR_SCHED_QID_INVALID;
            }
            bapInst->profileHandle = BapProfilHandle;
            bapServerQosParamInit();

            bapInst->ascsHandle = GattAscsServerInit(bapInst->libTask,
                ascsHandles->startHandle,
                ascsHandles->endHandle);
            if (bapInst->pacsHandle == 0)
            {
                bapInst->pacsHandle = GattPacsServerInit(bapInst->libTask,
                    pacsHandles->startHandle,
                    pacsHandles->endHandle);
            }
            CmIsocRegisterReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
                BAP_SERVER_ISOC_TYPE_UNICAST);
        }
        else
        {
            BAP_SERVER_PANIC("BapServer: BapServerUnicastInit bapInst NULL\n");
        }
    }

    return BapProfilHandle;
}

bapProfileHandle BapServerBroadcastInit(AppTask appTask, uint8 numberBroadcastSources,
                                        const BapServerHandleRange *pacsHandles,
                                        const BapServerHandleRange *bassHandles)
{
    BAP *bapInst = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        BAP_SERVER_PANIC("PANIC: BapServerBroadcastInit Application Task NULL\n");
    }

    if(BapProfilHandle == BAP_INVALID_HANDLE)
    {
        BapProfilHandle = (bapProfileHandle) ServiceHandleNewInstance((void **) &bapInst, sizeof(BAP));
        if(bapInst)
        {
            memset(bapInst, 0, sizeof(BAP));
            memset(bapInst->cid, 0, BAP_SERVER_MAX_CONNECTIONS);
            memset(bapInst->numConfigInstance, 0, BAP_SERVER_MAX_CONNECTIONS);
            bapInst->libTask = CSR_BT_BAP_SERVER_IFACEQUEUE;
        }
    }

    if (BapProfilHandle)
    {
       if(bapInst == NULL)
           bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(BapProfilHandle);

       if (bapInst != NULL)
       {
           /* Store the Task function parameter */
           bapInst->appBroadcastTask = appTask;
           bapInst->appSinkTask = appTask;

           if (bapInst->appUnicastTask == 0)
           {
               bapInst->appUnicastTask = CSR_SCHED_QID_INVALID;
           }
           bapInst->profileHandle = BapProfilHandle;

           bapInst->bassHandle = GattBassServerInit(bapInst->libTask,
               bassHandles->startHandle,
               bassHandles->endHandle,
               numberBroadcastSources);

           if (bapInst->pacsHandle == 0)
           {
               bapInst->pacsHandle = GattPacsServerInit(bapInst->libTask,
                   pacsHandles->startHandle,
                   pacsHandles->endHandle);
           }
           CmIsocRegisterReqSend(CSR_BT_BAP_SERVER_IFACEQUEUE,
               BAP_SERVER_ISOC_TYPE_BROADCAST);
       }
       else
       {
           BAP_SERVER_PANIC("BapServer: BapServerBroadcastInit bapInst NULL\n");
       }
    }

    return BapProfilHandle;
}


void BapServerUnicastAseReceiveStartReadyResponse(bapProfileHandle profileHandle,
                                                  ConnId connectionId,
                                                  uint8 numAses,
                                                  const uint8 *aseIds)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(GattAscsServerReceiverReadyRsp, numAses);

        /*validate connectionId */
        response->cid = connectionId;
        response->numAses = numAses;

        if(connectionId && numAses)
        {
            uint8 i;
            for(i = 0; i < numAses; i++)
            {
                response->gattAscsAseResult[i].aseId = aseIds[i];
                response->gattAscsAseResult[i].value = GATT_ASCS_ASE_RESULT_SUCCESS;
                BAP_SERVER_DEBUG("BapServerUnicastAseReceiveStartReadyResponse ase=0x%x result=0x%x",
                    aseIds[i], response->gattAscsAseResult[i].value);
            }
        }
        GattAscsReceiverReadyResponse(bapInst->ascsHandle,
                                      response);
        FREE_RESPONSE_ASE_RESULT(response);
    }
}

void BapServerUnicastAseReleased(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 uint8 numAses,
                                 const uint8 *aseIds,
                                 bool cacheCodecEnable)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        MAKE_ASCS_SERVER_RESPONSE_ASE(GattAscsServerReleaseComplete, numAses);

        response->cid = connectionId;
        response->numAses = numAses;

        if(connectionId && numAses)
        {
            uint8 i;
            for(i = 0; i < numAses; i++)
            {
                response->ase[i].aseId = aseIds[i];
                response->ase[i].cacheCodecConfiguration = cacheCodecEnable;
                response->ase[i].gattAscsServerConfigureCodecServerInfo = NULL;

                BAP_SERVER_DEBUG("BapServerUnicastAseReleased ase=0x%x", aseIds[i]);
            }
            GattAscsServerReleaseCompleteRequest(bapInst->ascsHandle,
                                                 response);
        }

        FREE_RESPONSE_ASE(response);
    }
}

bool BapServerUnicastAseConfigureCodecReq(bapProfileHandle profileHandle,
                                        BapServerAseConfigCodecReq *aseCodecInfo)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, aseCodecInfo->cid))
    {
        GattAscsServerConfigureCodecRequest(bapInst->ascsHandle, aseCodecInfo);
        return TRUE;
    }
    return FALSE;
}

bool BapServerUnicastAseUpdateMetadataRequest(bapProfileHandle profileHandle,
                                              BapServerAseUpdateMetadataReq *updateMetadataReq)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, updateMetadataReq->cid))
    {
        GattAscsServerUpdateMetadataRequest(bapInst->ascsHandle, updateMetadataReq);
        return TRUE;
    }
    return FALSE;
}

bool BapServerUnicastAseReceiveStartReadyReq(bapProfileHandle profileHandle,
                                                 ConnId connectionId,
                                                 uint8 numAses,
                                                 const uint8 *aseIds)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        MAKE_ASCS_SERVER_REQUEST_ASE_ID(GattAscsServerReceiverReadyReq, numAses);

        request->cid = connectionId;
        request->numAses = numAses;

        if(connectionId && numAses)
        {
            uint8 i;
            for(i = 0; i < numAses; i++)
            {
                request->aseId[i] = aseIds[i];
            }
        }
        GattAscsReceiverReadyRequest(bapInst->ascsHandle,
                                     request);
        FREE_REQUEST_ASE_ID(request);

        return TRUE;
    }
    return FALSE;
}

bool BapServerUnicastAseDisableReq(bapProfileHandle profileHandle,
                                       ConnId connectionId,
                                       uint8 numAses,
                                       const uint8 *aseIds,
                                       bool cisLoss)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        uint8 i;
        MAKE_ASCS_SERVER_REQUEST_ASE_ID(GattAscsServerDisableReq, numAses);

        request->cid = connectionId;
        request->numAses = numAses;

        for( i = 0; i < numAses; i++)
        {
            request->aseId[i] = aseIds[i];
        }
        GattAscsServerDisableRequest(bapInst->ascsHandle,
                                     request, cisLoss);
        FREE_REQUEST_ASE_ID(request);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

bool BapServerUnicastAseReleaseReq(bapProfileHandle profileHandle,
                                       ConnId connectionId,
                                       uint8 numAses,
                                       const uint8 *aseIds)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        uint8 i;
        MAKE_ASCS_SERVER_REQUEST_ASE_ID(GattAscsServerReleaseReq, numAses);

        request->cid = connectionId;
        request->numAses = numAses;

        for(i = 0; i < numAses; i++)
        {
            request->aseId[i] = aseIds[i];
        }
        GattAscsServerReleaseRequest(bapInst->ascsHandle,
                                     request);

        FREE_REQUEST_ASE_ID(request);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BapServerAseCodecInfo * BapServerUnicastReadAseCodecConfiguration(bapProfileHandle profileHandle,
                                                                  ConnId connectionId,
                                                                  uint8 aseId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapServerAseCodecInfo * Codecconfig = NULL;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        Codecconfig = (BapServerAseCodecInfo *)GattAscsReadCodecConfiguration(bapInst->ascsHandle,
                                                                              connectionId,
                                                                              aseId);
    }
    return Codecconfig;
}

BapServerAseQosInfo * BapServerUnicastReadAseQosConfiguration(bapProfileHandle profileHandle,
                                                              ConnId connectionId,
                                                              uint8 aseId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapServerAseQosInfo * Qosconfig = NULL;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        Qosconfig = (BapServerAseQosInfo *)GattAscsReadQosConfiguration(bapInst->ascsHandle,
                                                                        connectionId,
                                                                        aseId);
    }
    return Qosconfig;
}

AseDirectionType  BapServerUnicastReadAseDirection(bapProfileHandle profileHandle,
                                                   ConnId connectionId,
                                                   uint8 aseId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    AseDirectionType aseDirection = ASE_DIRECTION_UNINITIALISED;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        aseDirection = GattAscsReadAseDirection(bapInst->ascsHandle,
                                                connectionId,
                                                aseId);
    }
    return aseDirection;
}

bapStatus BapServerUnicastAddAscsConfig(bapProfileHandle profileHandle,
                                        ConnId connectionId,
                                        const BapAscsConfig * config)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    bapStatus status = BAP_SERVER_STATUS_SUCCESS;

    if ((bapInst) && bapServerAddConnectionIdToList(bapInst, connectionId))
    {
        if (GattAscsAddConfig(bapInst->ascsHandle,
                              connectionId,
                              (GattAscsClientConfig*) config) != CSR_BT_GATT_ACCESS_RES_SUCCESS)
        {
            status = BAP_SERVER_STATUS_FAILED;
            return status;
        }
        bapServerAddConfigToConnection(bapInst, connectionId);
    }
    return status;
}

BapAscsConfig * BapServerUnicastRemoveAscsConfig(bapProfileHandle profileHandle,
                                                 ConnId connectionId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapAscsConfig * config = NULL;
    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        config = (BapAscsConfig *)GattAscsRemoveConfig(bapInst->ascsHandle,
                                                       connectionId);
        bapServerRemoveConnectionIdFromList(bapInst, connectionId);
    }
    return config;
}

bool BapServerUnicastAseEnableRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(GattAscsServerEnableRsp, numAses);

        response->cid = connectionId;
        response->numAses = numAses;
        if(numAses && bapServerAseResult)
        {
            CsrMemCpy(response->gattAscsAseResult, bapServerAseResult,
                                        (numAses *sizeof(BapServerAseResult)));
        }
        GattAscsServerEnableResponse(bapInst->ascsHandle, response);
        FREE_RESPONSE_ASE_RESULT(response);
        return TRUE;
    }
    return FALSE;
}

bapStatus BapServerAddBassConfig(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 BapBassConfig * config)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    bapStatus status = BAP_SERVER_STATUS_SUCCESS;

    if ((bapInst) && bapServerAddConnectionIdToList(bapInst, connectionId))
    {
        if (GattBassServerAddConfig(bapInst->bassHandle,
                                    connectionId,
                                    (GattBassServerConfig*) config) != GATT_BASS_SERVER_STATUS_SUCCESS)
        {
            status = BAP_SERVER_STATUS_FAILED;
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
        bapServerRemoveConnectionIdFromList(bapInst, connectionId);
    }
    return config;
}

void* BapServerGetServiceConfig(bapProfileHandle profileHandle,
    ConnId connectionId,
    BapServerConfigType configType)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        if (configType == BAP_SERVER_CONFIG_BASS)
        {
            return (void *)GattBassServerGetConfig(bapInst->bassHandle, connectionId);
        }
        else if (configType == BAP_SERVER_CONFIG_PACS)
        {
            return (void *)GattPacsServerGetConfig(bapInst->pacsHandle, connectionId);
        }
        else if (configType == BAP_SERVER_CONFIG_ASCS)
        {
            return (void *)GattAscsServerGetConfig(bapInst->ascsHandle, connectionId);
        }
    }

    return NULL;
}

BapServerReleasingAseInfo* BapServerUnicastReadReleasingAseIdsByCisId(bapProfileHandle profileHandle,
                                 ConnId connectionId, uint8 cisId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        return (BapServerReleasingAseInfo*)GattAscsReadReleasingAseIdsByCisId(bapInst->ascsHandle,
                                    connectionId, cisId);
    }
    return NULL;
}

bool BapServerUnicastAseUpdateMetadataRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId, uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(GattAscsServerUpdateMetadataRsp, numAses);

        response->cid = connectionId;
        response->numAses = numAses;
        if(numAses && bapServerAseResult)
        {
            CsrMemCpy(response->gattAscsAseResult, bapServerAseResult,
                                        (numAses *sizeof(BapServerAseResult)));
        }
        GattAscsServerUpdateMetadataResponse(bapInst->ascsHandle, response);
        FREE_RESPONSE_ASE_RESULT(response);
        return TRUE;
    }
    return FALSE;
}

BapServerAseData* BapServerUnicastGetAseData(bapProfileHandle profileHandle,
                                             ConnId connectionId,
                                             uint8 aseId)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);
    BapServerAseData *bapAseData = NULL;

    if ((bapInst) && bapServerIsValidConectionId(bapInst, connectionId))
    {
        bapAseData = (BapServerAseData*) CsrPmemZalloc(sizeof(BapServerAseData));

        if (bapAseData != NULL)
            bapAseData->aseData = GattAscsServerGetAseData(bapInst->ascsHandle, connectionId, aseId, &bapAseData->aseDataLength);
    }

    return bapAseData;
}

