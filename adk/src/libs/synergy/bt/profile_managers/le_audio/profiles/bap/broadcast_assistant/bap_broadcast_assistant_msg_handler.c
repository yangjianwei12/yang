/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant handler for upstream and internal downstream primitives.
 */

/**
 * \defgroup BAP_BROADCAST_ASSISTANT_HANDLER BAP
 * @{
 */

#include <stdio.h>
#include "bap_client_lib.h"
#include "bap_client_list_container_cast.h"
#include "tbdaddr.h"
#include "csr_bt_common.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_tasks.h"

#include "bap_utils.h"
#include "bap_client_debug.h"

#include "bap_broadcast_assistant_utils.h"
#include "bap_broadcast_assistant.h"
#include "bap_broadcast_assistant_msg_handler.h"

#include "bap_broadcast_src.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

/*! Service Data (16-bit default). */
#define BLE_AD_TYPE_SERVICE_DATA            0x16

#define UUID_BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE    0x1852
#define UUID_BASIC_AUDIO_ANNOUNCMENT_SERVICE         0x1851

#define BAP_PRESENTATION_DELAY_SIZE        0x3
#define BAP_NUM_SUBGROUP_OFFSET            0x3
#define BAP_SUBGROUP_LEN                   0x1

#define BAP_NUM_OF_BIS_OFFSET_L2   (BAP_PRESENTATION_DELAY_SIZE + BAP_SUBGROUP_LEN)

#define BAP_NUM_BIS_LEN                    0x1
#define BAP_CODEC_ID_SIZE                  0x5
#define BAP_CODEC_SPECIFIC_CONFIG_LEN      0x1
#define BAP_METADATA_LEN                   0x1
#define BAP_NUM_BIS_INDEX_LEN              0x1

static Bool findConnectionByBroadcastAssistantState(BAP* const bap,
                                                    bapAssistantState assistantState,
                                                    struct BapConnection** const connection)
{
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&bap->connectionList);
        listElement != NULL;
        listElement = bapClientListElementGetNext(listElement))
    {
        *connection = CONTAINER_CAST(listElement, BapConnection, listElement);

        if ((*connection)->broadcastAssistant.assistantState == assistantState)
        {
            return TRUE;
        }
    }
    return FALSE;
}


static BapResult bapBroadcastAssistantGetSourceReq(BAP * const bap,
                                                   BapProfileHandle handle,
                                                   uint16 filterContext)
{
    BroadcastSrcList *broadcastSrcs = (BroadcastSrcList*) CsrPmemZalloc(sizeof(BroadcastSrcList));
    BroadcastSrc *tempSrcs;

    /* Get the source list */
    if (bapBroadcastSrcGetSrcList(bap, filterContext, broadcastSrcs))
    {
        BapBroadcastAssistantSrcReportInd *indPrim;
        BroadcastSrc  *sources = broadcastSrcs->sources;

        while (sources)
        {
            uint16 i = 0, j= 0;

            indPrim = CsrPmemZalloc(sizeof(BapBroadcastAssistantSrcReportInd));
            indPrim->type = BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND;
            indPrim->handle = handle;
            indPrim->advSid = sources->advSid;
            indPrim->collocated = TRUE;
            indPrim->advHandle = sources->advHandle;
            indPrim->broadcastId = sources->broadcastId;
            tbdaddr_copy(&(indPrim->sourceAddrt), &(broadcastSrcs->sourceAddress));
            indPrim->bigNameLen = sources->bigNameLen;

            if(indPrim->bigNameLen)
            {
                indPrim->bigName = CsrPmemZalloc(indPrim->bigNameLen * sizeof(char));
                SynMemCpyS(indPrim->bigName, indPrim->bigNameLen, 
                        sources->bigName, sources->bigNameLen);
            }

            indPrim->numSubgroup = sources->numSubgroup;

            indPrim->subgroupInfo = CsrPmemZalloc(indPrim->numSubgroup * sizeof(BapBigSubgroup));

            /* Level 2 and level3 info */
            for(i = 0; i < sources->numSubgroup; i++)
            {
                uint16 metadataLen = sources->subgroupInfo[i].metadata.metadataLen;

                CsrMemCpy(&(indPrim->subgroupInfo[i].codecId),
                       &(sources->subgroupInfo[i].codecId),
                       sizeof(BapCodecId));

                CsrMemCpy(&(indPrim->subgroupInfo[i].codecConfigurations),
                       &(sources->subgroupInfo[i].codecConfigurations),
                       sizeof(BapCodecConfiguration));

                indPrim->subgroupInfo[i].metadata.streamingAudioContext = sources->subgroupInfo[i].metadata.streamingAudioContext;
                indPrim->subgroupInfo[i].metadata.metadataLen = metadataLen;

                if (metadataLen != 0)
                {
                    indPrim->subgroupInfo[i].metadata.metadata = CsrPmemZalloc(metadataLen);

                    CsrMemCpy(indPrim->subgroupInfo[i].metadata.metadata,
                                   sources->subgroupInfo[i].metadata.metadata, metadataLen);

                    CsrPmemFree(sources->subgroupInfo[i].metadata.metadata);
                }

                indPrim->subgroupInfo[i].numBis = sources->subgroupInfo[i].numBis;

                if(sources->subgroupInfo[i].numBis > 0)
                {
                    indPrim->subgroupInfo[i].bisInfo =  CsrPmemZalloc(sources->subgroupInfo[i].numBis*sizeof(BapBisInfo));

                    for(j = 0; j < sources->subgroupInfo[i].numBis; j++)
                    {
                       indPrim->subgroupInfo[i].bisInfo[j].bisIndex = sources->subgroupInfo[i].bisInfo[j].bisIndex;
                       indPrim->subgroupInfo[i].bisInfo[j].bisHandle = sources->subgroupInfo[i].bisInfo[j].bisHandle;
                       CsrMemCpy(&(indPrim->subgroupInfo[i].bisInfo[j].codecConfigurations),
                            &(sources->subgroupInfo[i].bisInfo[j].codecConfigurations), sizeof(BapCodecConfiguration));
                    }
                    CsrPmemFree(sources->subgroupInfo[i].bisInfo);
                }
            }

            CsrPmemFree(sources->subgroupInfo);
            putMessageSynergy(bap->appPhandle, BAP_PRIM, indPrim);

            sources = sources->next;

        }

        /* Free the Broadcast Src List*/
        while(broadcastSrcs->sources)
        {
            tempSrcs = broadcastSrcs->sources;
            broadcastSrcs->sources = tempSrcs->next;
            tempSrcs->next = NULL;
            CsrPmemFree(tempSrcs);
        }
        tempSrcs = broadcastSrcs->sources;

        CsrPmemFree(broadcastSrcs);

        return BAP_RESULT_SUCCESS;
    }

    return BAP_RESULT_ERROR;
}

static void bapBroadcastAssistantExtScanSetGlobalParams(BAP * const bap,
                                                        BapProfileHandle handle,
                                                        uint8 scanFlags,
                                                        uint8 ownAddressType,
                                                        uint8 scanningFilterPolicy)
{
    uint8 filterDuplicated = 0x01;
    uint16 scanningPhys = 1;
    CmScanningPhy phys[2] = {0};
    phys[0].scan_type = 0x1;
    phys[0].scan_interval = 0x0064;
    phys[0].scan_window = 0x005F;

    CmExtScanSetGlobalParamsReqSend(CSR_BT_BAP_IFACEQUEUE, scanFlags, ownAddressType,
        scanningFilterPolicy, filterDuplicated, scanningPhys, phys);

    CSR_UNUSED(bap);
    CSR_UNUSED(handle);
}


static void handleBapBroadcastAssistantStartScanReq(BAP * const bap,
                                                    BapInternalBroadcastAssistantStartScanReq * const req)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_INVALID_OPERATION;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        if (req->flags == BROADCAST_SRC_COLLOCATED)
        {
            /* Collocated Assistant is interested in local Broadcast SRC only */
            result = bapBroadcastAssistantGetSourceReq(bap, req->handle, req->filterContext);

            bapBroadcastAssistantUtilsSendStartScanCfm(bap->appPhandle,
                                                       req->handle,
                                                       result,
                                                       0);
        }
        else
        {
            if (req->flags == BROADCAST_SRC_ALL)
            {
                result = bapBroadcastAssistantGetSourceReq(bap, req->handle, req->filterContext);
            }

            bapBroadcastAssistantExtScanSetGlobalParams(bap,
                                                        req->handle,
                                                        req->scanFlags,
                                                        req->ownAddressType,
                                                        req->scanningFilterPolicy);

            /* Update the assistant state */
            bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_START_SCAN);
        }
    }
    else
    {
        bapBroadcastAssistantUtilsSendStartScanCfm(bap->appPhandle,
                                                   req->handle,
                                                   result,
                                                   0);
    }
}

static void handleBapBroadcastAssistantStopScanReq(BAP * const bap,
                                                   BapInternalBroadcastAssistantStopScanReq * const req)
{
    BapConnection* connection = NULL;

    BAP_CLIENT_DEBUG("(BAP) : handleBapBroadcastAssistantStopScanReq scanHandle =%x\n\n",req->scanHandle);

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        if (req->scanHandle != 0)
        {
            /* We enabled start scan for periodic Adv, we need to stop it */
            CmPeriodicScanStopFindTrainsReqSend(CSR_BT_BAP_IFACEQUEUE, (uint8)req->scanHandle);

            /* Update the assistant state */
            bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_STOP_SCAN);
        }
        else
        {
            /*  return cfm success whenever called for collocated */
            bapBroadcastAssistantUtilsSendStopScanCfm(bap->appPhandle,
                                                      req->handle,
                                                      BAP_RESULT_SUCCESS);
        }
    }
    else
    {
        bapBroadcastAssistantUtilsSendStopScanCfm(bap->appPhandle,
                                                  req->handle,
                                                  BAP_RESULT_INVALID_OPERATION);
    }
}

static void handleBapBroadcastAssistantSyncToSrcStartReq(BAP * const bap,
                                                         BapInternalBroadcastAssistantSyncToSrcStartReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        CmPeriodicScanTrains periodicTrains;
        uint8 reportPeriodic = 1;
        uint16 skip = 0;
        uint16 syncTimeout = 500;
        uint8 syncCteType = 0;
        uint16 attemptSyncForXSeconds = 0;
        uint8 numberOfPeriodicTrains = 1;

        periodicTrains.advSid = req->advSid;

        CsrMemCpy(&(periodicTrains.addrt), &(req->addrt), sizeof(TYPED_BD_ADDR_T));

        CmPeriodicScanSyncToTrainReqSend(CSR_BT_BAP_IFACEQUEUE,
                                        reportPeriodic,
                                        skip,
                                        syncTimeout,
                                        syncCteType,
                                        attemptSyncForXSeconds,
                                        numberOfPeriodicTrains,
                                        &periodicTrains);

        /* Update sync params */
        bapBroadcastAssistantAddSyncParam(connection, req->addrt, req->advSid);

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_SYNC_TO_SRC);
    }
    else
    {
        bapBroadcastAssistantUtilsSendSyncToSrcStartCfm(bap->appPhandle,
                                                        req->handle,
                                                        0,
                                                        req->advSid,
                                                        0,
                                                        0,
                                                        0,
                                                        req->addrt,
                                                        BAP_RESULT_INVALID_OPERATION);
    }
}

static void handleBapBroadcastAssistantSyncToSrcCancelReq(BAP * const bap,
                                                          BapInternalBroadcastAssistantSyncToSrcCancelReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_SYNC_TO_SRC)
    {
        CmPeriodicScanSyncToTrainCancelReqSend(CSR_BT_BAP_IFACEQUEUE);
    }
    else
    {
        bapBroadcastAssistantUtilsSendSyncToSrcCancelCfm(bap->appPhandle,
                                                         req->handle,
                                                         BAP_RESULT_INVALID_OPERATION);
    }

}

static void handleBapBroadcastAssistantSyncToSrcTerminateReq(BAP * const bap,
                                                             BapInternalBroadcastAssistantSyncToSrcTerminateReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        CmPeriodicScanSyncTerminateReqSend(CSR_BT_BAP_IFACEQUEUE,
                                           req->syncHandle);
    }
    else
    {
        bapBroadcastAssistantUtilsSendSyncToSrcTerminateCfm(bap->appPhandle,
                                                            req->handle,
                                                            req->syncHandle,
                                                            BAP_RESULT_INVALID_OPERATION);
    }
}


static void handleBapBroadcastAssistantBrsRegisterForNotificationReq(BAP * const bap,
                                                                     BapInternalBroadcastAssistantBrsRegisterForNotifcationReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        GattBassClientBroadcastReceiveStateRegisterForNotificationReq(
                        connection->bass.srvcHndl,
                        req->sourceId,
                        req->allSources,
                        TRUE);
    }
    else
    {
        bapBroadcastAssistantUtilsSendBroadcastReceiveStateSetNtfCfm(bap->appPhandle,
                                                                     req->handle,
                                                                     BAP_RESULT_INVALID_OPERATION,
                                                                     req->sourceId);
    }
}

static void handleBapBroadcastAssistantReadBrsCccReq(BAP * const bap,
                                                     BapInternalBroadcastAssistantReadBrsCccReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        GattBassClientReadBroadcastReceiveStateCccRequest(
                                        connection->bass.srvcHndl,
                                        req->sourceId,
                                        req->allSources);
    }
    else
    {
        bapBroadcastAssistantUtilsSendReadBroadcastReceiveStateCccCfm(bap->appPhandle,
                                                                      req->handle,
                                                                      BAP_RESULT_INVALID_OPERATION,
                                                                      req->sourceId,
                                                                      0,
                                                                      0);
    }
}

static void handleBapBroadcastAssistantReadBrsReq(BAP * const bap,
                                                  BapInternalBroadcastAssistantReadBrsReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        GattBassClientReadBroadcastReceiveStateRequest(connection->bass.srvcHndl,
                                                       req->sourceId,
                                                       req->allSources);
    }
    else
    {
        BD_ADDR_T sourceAddress = {0, 0, 0};
        bapBroadcastAssistantUtilsSendBroadcastReceiveStateCfm(bap->appPhandle,
                                                               req->handle,
                                                               BAP_RESULT_INVALID_OPERATION,
                                                               req->sourceId,
                                                               sourceAddress,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0,
                                                               0);
    }

}

static void handleBapBroadcastAssistantAddSrcReq(BAP * const bap,
                                                 BapInternalBroadcastAssistantAddSrcReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        GattBassClientAddSourceParam params;
        uint8 i;
        bool responseOp, longWrite;

        for (i = 0; i < req->numbSubGroups; i++)
        {
            if ((req->subgroupInfo[i]->bisSyncState != 0xFFFFFFFF) &&
                (!((req->paSyncState == GATT_BASS_CLIENT_PA_SYNC_SYNCHRONIZE_PAST) || (req->paSyncState == GATT_BASS_CLIENT_PA_SYNC_SYNCHRONIZE_NO_PAST))))
            {
                bapBroadcastAssistantUtilsSendAddSourceCfm(bap->appPhandle,
                                                           req->handle,
                                                           BAP_RESULT_INVALID_PARAMETER);
                return;
            }
        }

        bapBroadcastAssistantGetControlPointOp(connection, &responseOp, &longWrite);

        params.advSid = req->sourceAdvSid;
        params.paSyncState = req->paSyncState;

        bd_addr_copy(&params.sourceAddress.addr, &req->sourceAddrt);
        params.sourceAddress.type = req->advertiserAddressType;

        params.paInterval = req->paInterval;
        params.numSubGroups = req->numbSubGroups;
        params.broadcastId = req->broadcastId;

        params.subGroupsData =
            (GattBassClientSubGroupsData *) CsrPmemZalloc(params.numSubGroups * sizeof(GattBassClientSubGroupsData));

        BAP_CLIENT_DEBUG("Num of subgroup %x \n", req->numbSubGroups);

        for (i = 0; i < req->numbSubGroups; i++)
        {
            params.subGroupsData[i].bisSync = req->subgroupInfo[i]->bisSyncState;
            params.subGroupsData[i].metadataLen = req->subgroupInfo[i]->metadataLen;

            BAP_CLIENT_DEBUG("BisSyncState %x\n", params.subGroupsData[i].bisSync);

            if (req->subgroupInfo[i]->metadataLen != 0)
            {
                params.subGroupsData[i].metadata = CsrPmemZalloc(req->subgroupInfo[i]->metadataLen *sizeof(uint8));
                CsrMemCpy(params.subGroupsData[i].metadata,
                       req->subgroupInfo[i]->metadataValue,
                       req->subgroupInfo[i]->metadataLen *sizeof(uint8));

                CsrPmemFree(req->subgroupInfo[i]->metadataValue);
            }
            CsrPmemFree(req->subgroupInfo[i]);
        }

        GattBassClientAddSourceRequest(connection->bass.srvcHndl, &params, !responseOp, longWrite);

        /*  Update source params */
        bapBroadcastAssistantAddSourceParam(connection,
                                            &params,
                                            req->syncHandle,
                                            req->srcCollocated);

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_ADDING_SOURCE);

        /* Free memory for params */
        for (i = 0; i < req->numbSubGroups; i++)
        {
            if (params.subGroupsData[i].metadataLen != 0)
                CsrPmemFree(params.subGroupsData[i].metadata);
        }

        CsrPmemFree(params.subGroupsData);

    }
    else
    {
        bapBroadcastAssistantUtilsSendAddSourceCfm(bap->appPhandle,
                                                   req->handle,
                                                   BAP_RESULT_INVALID_OPERATION);
    }
}

static void handleBapBroadcastAssistantModifySrcReq(BAP * const bap,
                                                    BapInternalBroadcastAssistantModifySrcReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        GattBassClientModifySourceParam params;
        uint8 i;
        bool responseOp;

        bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);

        params.paSyncState = req->paSyncState;
        params.paInterval = req->paInterval;
        params.numSubGroups = req->numbSubGroups;
        params.sourceId = req->sourceId;


        params.subGroupsData =
            (GattBassClientSubGroupsData *) CsrPmemZalloc(params.numSubGroups * sizeof(GattBassClientSubGroupsData));

        for (i = 0; i < req->numbSubGroups; i++)
        {
            params.subGroupsData[i].bisSync = req->subgroupInfo[i]->bisSyncState;
            params.subGroupsData[i].metadataLen = req->subgroupInfo[i]->metadataLen;

            if (req->subgroupInfo[i]->metadataLen != 0)
            {
                params.subGroupsData[i].metadata = CsrPmemZalloc(req->subgroupInfo[i]->metadataLen *sizeof(uint8));
                CsrMemCpy(params.subGroupsData[i].metadata,
                       req->subgroupInfo[i]->metadataValue,
                       req->subgroupInfo[i]->metadataLen *sizeof(uint8));

                CsrPmemFree(req->subgroupInfo[i]->metadataValue);
            }
            CsrPmemFree(req->subgroupInfo[i]);
        }

        GattBassClientModifySourceRequest(connection->bass.srvcHndl, &params, !responseOp);

        /*  Update source params rquested */
        bapBroadcastAssistantModifySourceParam(connection,
                                               &params,
                                               req->syncHandle,
                                               req->srcCollocated,
                                               req->sourceAdvSid);

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_MODIFYING_SOURCE);

        /* Free memory for params */
        for (i = 0; i < req->numbSubGroups; i++)
        {
            if (params.subGroupsData[i].metadataLen != 0)
                CsrPmemFree(params.subGroupsData[i].metadata);
        }

        CsrPmemFree(params.subGroupsData);

    }
    else
    {
        bapBroadcastAssistantUtilsSendModifySourceCfm(bap->appPhandle,
                                                      req->handle,
                                                      BAP_RESULT_INVALID_OPERATION);
    }
}

static void handleBapBroadcastAssistantRemoveSrcReq(BAP * const bap,
                                                    BapInternalBroadcastAssistantRemoveSrcReq * const req)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, req->handle, &connection)
        && bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        bool responseOp;
        bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);

        GattBassClientRemoveSourceRequest(connection->bass.srvcHndl, req->sourceId, !responseOp);

        /*  Update source params rquested */
        bapBroadcastAssistantRemoveSourceId(connection, req->sourceId);

        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_REMOVING_SOURCE);
    }
    else
    {
        bapBroadcastAssistantUtilsSendRemoveSourceCfm(bap->appPhandle,
                                                      req->handle,
                                                      BAP_RESULT_INVALID_OPERATION);
    }

}

static void handleBapBroadcastAssistantSetCodeRsp(BAP * const bap,
                                                  BapInternalBroadcastAssistantSetCodeRsp * const rsp)
{
    BapConnection* connection = NULL;

    if (bapClientFindConnectionByCid(bap, rsp->handle, &connection) &&
        bapBroadcastAssistantGetState(connection) == BAP_ASSISTANT_STATE_IDLE)
    {
        bool responseOp;
        uint8 *broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE);

        bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);
        CsrMemCpy(broadcastCode, rsp->broadcastCode, BAP_BROADCAST_CODE_SIZE);

        GattBassClientSetBroadcastCodeRequest(connection->bass.srvcHndl,
                                              rsp->sourceId,
                                              broadcastCode,
                                              !responseOp);
        /* Update the assistant state */
        bapBroadcastAssistantUpdateState(connection, BAP_ASSISTANT_STATE_SEND_CODES);
    }
}

void bapBroadcastAssistantInternalMsgHandler(BAP * const bap,
                                             BapUPrim * const primitive)
{

    switch (primitive->type)
    {
        case BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ:
             handleBapBroadcastAssistantStartScanReq(bap,&primitive->bapInternalBroadcastAssistantStartScanReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ:
             handleBapBroadcastAssistantStopScanReq(bap,&primitive->bapInternalBroadcastAssistantStopScanReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ:
             handleBapBroadcastAssistantSyncToSrcStartReq(bap,&primitive->bapInternalBroadcastAssistantSyncToSrcStartReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ:
             handleBapBroadcastAssistantSyncToSrcCancelReq(bap,&primitive->bapInternalBroadcastAssistantSyncToSrcCancelReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ:
             handleBapBroadcastAssistantSyncToSrcTerminateReq(bap,&primitive->bapInternalBroadcastAssistantSyncToSrcTerminateReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ:
             handleBapBroadcastAssistantBrsRegisterForNotificationReq(bap,&primitive->bapInternalBroadcastAssistantBrsRegisterForNotifcationReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ:
             handleBapBroadcastAssistantReadBrsCccReq(bap, &primitive->bapInternalBroadcastAssistantReadBrsCccReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ:
             handleBapBroadcastAssistantReadBrsReq(bap,&primitive->bapInternalBroadcastAssistantReadBrsReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ:
             handleBapBroadcastAssistantAddSrcReq(bap,&primitive->bapInternalBroadcastAssistantAddSrcReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ:
             handleBapBroadcastAssistantModifySrcReq(bap,&primitive->bapInternalBroadcastAssistantModifySrcReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ:
             handleBapBroadcastAssistantRemoveSrcReq(bap,&primitive->bapInternalBroadcastAssistantRemoveSrcReq);
             break;
        case BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP:
             handleBapBroadcastAssistantSetCodeRsp(bap,&primitive->bapInternalBroadcastAssistantSetCodeRsp);
             break;
        default:
             break;
    }
}

static bool checkBaasUUIDAdvReport(uint8 *dataAdv,
                                   uint16 dataLenghtAdv,
                                   uint16 *offset)
{

    uint16 i;
    uint8 upBassUuid = (uint8)(UUID_BASIC_AUDIO_ANNOUNCMENT_SERVICE >> 8);
    uint8 lowBassUuid =(uint8) UUID_BASIC_AUDIO_ANNOUNCMENT_SERVICE;
    uint8 partial = 0;
    bool adTypeServiceDataFound = FALSE;

    for (i=0; i<dataLenghtAdv; i++)
    {
        if(dataAdv[i] == BLE_AD_TYPE_SERVICE_DATA)
        {
            adTypeServiceDataFound = TRUE;
            continue;
        }

        if(adTypeServiceDataFound)
        {
            if (!partial)
            {
                if (dataAdv[i] == lowBassUuid)
                {
                    partial = dataAdv[i];
                }
            }
            else
            {
                if (dataAdv[i] == upBassUuid)
                {
                    *offset = i;
                    return TRUE;
                }
                else if (dataAdv[i] == lowBassUuid)
                {
                    partial = dataAdv[i];
                }
                else
                {
                    partial = 0;
                    adTypeServiceDataFound = FALSE;
                }
            }
        }
    }

    offset = 0;
    return FALSE;
}


static uint8 getCodecId(uint8 *data, uint8 offset)
{
    return (data[offset]);
}

static uint8 getCompanyVendorId(uint8 *data, uint8 offset)
{
    return((((uint16)data[offset + 1]) >> 8) || (data[offset ]));
}

static bool getCodecConfiguration(uint8 *data, uint8 codecLength,
                                  BapCodecConfiguration* codecConfiguration)
{
    QblLtv ltv;
    uint8* ltvFormatData;
    bool status = TRUE;

    /* Set default value if not present */
    codecConfiguration->lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;

    /* Decode LTV format data from the Codec specific parameter */
    for (ltvFormatData = data;
         ltvFormatData < data + codecLength;
         ltvFormatData = qblLtvGetNextLtvStart(&ltv))
    {
        qblLtvInitialise(&ltv, ltvFormatData);

        if ( ! qblLtvDecodeCodecSpecificCapabilities(&ltv, codecConfiguration))
        {
            BAP_CLIENT_ERROR(" BAP:: Wrong Codec Config parameter for Type %d\n", ltv.type);
            status = FALSE;
            break; /* Exit the loop */
        }
    }
    return status;
}

static void parseBaseDataInAudioAnnouncements(BapBroadcastAssistantSrcReportInd *indPrim,
                                              uint8* data)
{
    uint8 metadataLength;
    uint8 i,j, offsetL2;
    uint8 subGroupLen = 0, subGroupCodecLength, subGroupCodecLengthL3;

    /* Num of Subgroups */
    indPrim->numSubgroup = data[BAP_NUM_SUBGROUP_OFFSET];

    indPrim->subgroupInfo = CsrPmemZalloc(indPrim->numSubgroup * sizeof(BapBigSubgroup));

    offsetL2 = BAP_NUM_OF_BIS_OFFSET_L2;

       /* Level 2 and level3 info */
    for(i = 0; i < indPrim->numSubgroup; i++)
    {
        indPrim->subgroupInfo[i].numBis = data[offsetL2 + subGroupLen];
        subGroupLen +=BAP_NUM_BIS_LEN;

        indPrim->subgroupInfo[i].codecId.codecId = getCodecId(data, offsetL2 + subGroupLen);
        indPrim->subgroupInfo[i].codecId.companyId = getCompanyVendorId(data, offsetL2 + subGroupLen + 1);
        indPrim->subgroupInfo[i].codecId.vendorCodecId = getCompanyVendorId(data , offsetL2 + subGroupLen + 3);
        subGroupLen += BAP_CODEC_ID_SIZE;


        /* codec configuration */
        subGroupCodecLength = data[offsetL2 + subGroupLen];

        subGroupLen += BAP_CODEC_SPECIFIC_CONFIG_LEN ; /* add subGroupCodecLength of 1 octet */

        /* Get Codec specific configuration parameter */
        if((subGroupCodecLength) && (indPrim->subgroupInfo[i].codecId.codecId == BAP_CODEC_ID_LC3))
        {
            BapCodecConfiguration  codecConfigurationL2;

            memset(&codecConfigurationL2, 0, sizeof(BapCodecConfiguration));

            if(getCodecConfiguration(&(data[offsetL2 + subGroupLen]), subGroupCodecLength,
                &codecConfigurationL2))
            {
                BAP_CLIENT_DEBUG("BAP: Codec config returned 2\n");
            }

            /* We need to convert Sampling frequency and frame duration in user freindly format
             * from bit mask values
             */
            switch(codecConfigurationL2.samplingFrequency)
            {
                case SAMPLING_FREQUENCY_8kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_8kHz;
                    break;
                case SAMPLING_FREQUENCY_16kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_16kHz;
                    break;
                case SAMPLING_FREQUENCY_24kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_24kHz;
                case SAMPLING_FREQUENCY_32kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_32kHz;
                break;
                case SAMPLING_FREQUENCY_44_1kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_44_1kHz;
                break;
                case SAMPLING_FREQUENCY_48kHz:
                    codecConfigurationL2.samplingFrequency = BAP_SAMPLING_FREQUENCY_48kHz;
                break;
            }

            switch(codecConfigurationL2.frameDuaration)
            {
                case 0:
                    codecConfigurationL2.frameDuaration = BAP_SUPPORTED_FRAME_DURATION_7P5MS;
                    break;
                case 1:
                    codecConfigurationL2.frameDuaration = BAP_SUPPORTED_FRAME_DURATION_10MS;
                    break;
            }


            CsrMemCpy(&(indPrim->subgroupInfo[i].codecConfigurations), &codecConfigurationL2, sizeof(BapCodecConfiguration));
            subGroupLen += subGroupCodecLength;
        }

        /* Parse metadata */
        metadataLength = data[offsetL2 + subGroupLen];

        subGroupLen += BAP_METADATA_LEN;

        if(metadataLength != 0)
        {
            uint8* metadata = &(data[offsetL2 + subGroupLen]);

            /* Parse metadata for streamingAudioConetxt */
            if(metadata[1] == BAP_PAC_SUPPORTED_AUDIO_CONTEXT)
            {
                indPrim->subgroupInfo[i].metadata.streamingAudioContext = ((uint16)(metadata[3] << 8) | metadata[2]);
            }

            /* TODO vendor metadata to be supported later */
            indPrim->subgroupInfo[i].metadata.metadataLen = 0;

            if (indPrim->subgroupInfo[i].metadata.metadataLen != 0)
            {
                indPrim->subgroupInfo[i].metadata.metadata =
                       CsrPmemZalloc(indPrim->subgroupInfo[i].metadata.metadataLen);

                 CsrMemCpy((indPrim->subgroupInfo[i].metadata.metadata), metadata, metadataLength);
            }

            subGroupLen += metadataLength;
        }

        if(indPrim->subgroupInfo[i].numBis > 0)
        {
            indPrim->subgroupInfo[i].bisInfo =  CsrPmemZalloc(indPrim->subgroupInfo[i].numBis*sizeof(BapBisInfo));

            for(j = 0; j < indPrim->subgroupInfo[i].numBis; j++)
            {
               indPrim->subgroupInfo[i].bisInfo[j].bisIndex = data[offsetL2 + subGroupLen ];
               subGroupLen += BAP_NUM_BIS_INDEX_LEN;

               /* codec configuration */
               subGroupCodecLengthL3 = data[offsetL2 + subGroupLen];

               /* Get Codec specific configuration parameter */
               if((subGroupCodecLengthL3) && (indPrim->subgroupInfo[i].codecId.codecId == BAP_CODEC_ID_LC3))
               {
                   BapCodecConfiguration  codecConfigurationL3;

                   memset(&codecConfigurationL3, 0, sizeof(BapCodecConfiguration));

                   subGroupLen += BAP_CODEC_SPECIFIC_CONFIG_LEN ; /* add subGroupCodecLength of 1 octet */

                   if(getCodecConfiguration(&(data[offsetL2 + subGroupLen]), subGroupCodecLengthL3,
                       &codecConfigurationL3))
                   {
                       BAP_CLIENT_DEBUG("BAP: Codec config returned 3\n");
                   }

                   CsrMemCpy(&(indPrim->subgroupInfo[i].bisInfo[j].codecConfigurations),
                          &codecConfigurationL3,
                          sizeof(BapCodecConfiguration));

                   subGroupLen += subGroupCodecLengthL3;
               }
            }
        }
    }
}

void bapBroadcastAssistantCmPrimitiveHandler(BAP * const bap,
                                             CsrBtCmPrim * const primitive)
{
    BapConnection* connection = NULL;
    BapResult result = BAP_RESULT_ERROR;

    BAP_CLIENT_INFO("BAP: bapBroadcastAssistantCmPrimitiveHandler prim %x\n", *primitive);

    switch (*primitive)
    {
        case CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND:
        {
            uint32 broadcastId = BAP_INVALID_BROADCAST_ID;
            char bigName[MAX_BAP_SRC_NAME_LENGTH];
            uint8 bigNameLen = 0, i;
            uint8 serviceData[MAX_SERVICE_DATA_LENGTH];
            uint8 serviceDataLen = 0;

            CmExtScanFilteredAdvReportInd *ind = (CmExtScanFilteredAdvReportInd*) primitive;
            BapBroadcastAssistantSrcReportInd *indPrim =
                CsrPmemZalloc(sizeof(BapBroadcastAssistantSrcReportInd));

            if (ind->dataLength == 0 ||
                !bapParseAdvDataForUuid16(ind->data,
                                          ind->dataLength,
                                          UUID_BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE,
                                          &broadcastId,
                                          BAP_BROADCAST_ID_SIZE,
                                          bigName,
                                          &bigNameLen,
                                          serviceData,
                                          &serviceDataLen))
            {
                BAP_CLIENT_ERROR("(BAP) : Device does not support Broadcast Source Service. Ignore Adv Report\n\n");
                return;
            }

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_IDLE,
                                                        &connection))
            {
                indPrim->type = BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND;
                indPrim->handle = connection->cid;
                indPrim->advSid = ind->advSid;
                indPrim->collocated = FALSE;
                indPrim->advHandle = 0;
                indPrim->broadcastId = broadcastId;
                indPrim->bigNameLen = bigNameLen;
                if (bigNameLen)
                {
                    indPrim->bigName = CsrPmemZalloc(bigNameLen * sizeof(char));
                    for (i = 0; i < indPrim->bigNameLen; i++)
                        indPrim->bigName[i] = bigName[i];
                }

                indPrim->serviceDataLen = serviceDataLen;
                if (serviceDataLen)
                {
                    indPrim->serviceData = CsrPmemZalloc(serviceDataLen * sizeof(uint8));
                    for (i = 0; i < indPrim->serviceDataLen; i++)
                        indPrim->serviceData[i] = serviceData[i];
                }

                tbdaddr_copy(&(indPrim->sourceAddrt), &(ind->currentAddrt));

                putMessageSynergy(bap->appPhandle, BAP_PRIM, indPrim);
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
        {
            CmPeriodicScanSyncAdvReportInd *ind = (CmPeriodicScanSyncAdvReportInd*) primitive;
            BapBroadcastAssistantSrcReportInd *indPrim =
                CsrPmemZalloc(sizeof(BapBroadcastAssistantSrcReportInd));
            uint16 offset;

            if (ind->dataLength ==0 ||
                !checkBaasUUIDAdvReport(ind->data, ind->dataLength, &offset))
            {
                BAP_CLIENT_DEBUG("(BAP) : Device does not support Basic Audio Announcement UUID. Ignore Adv Report\n\n");
                return;
            }

            BAP_CLIENT_DEBUG("(BAP) : Basic Audio Announcement found. Offset %x \n", offset);
            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_IDLE,
                                                        &connection))
            {
                TYPED_BD_ADDR_T sourceAddress;
                uint8 advSid;

                bapBroadcastAssistantGetSyncParams(connection, &sourceAddress, &advSid);
                BAP_CLIENT_DEBUG("(BAP : advSid = 0x%02x\n", advSid);


                indPrim->type = BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND;
                indPrim->handle = connection->cid;
                indPrim->advSid = advSid;
                indPrim->collocated = FALSE;
                indPrim->advHandle = (uint8)ind->syncHandle;
                tbdaddr_copy(&(indPrim->sourceAddrt), &sourceAddress);

                /* Parse the adv_data to extract Level-1, Level-2, Level-3 info */
                 if(ind->dataLength != 0)
                 {
                     parseBaseDataInAudioAnnouncements(indPrim, &(ind->data[offset+1]));
                 }

                putMessageSynergy(bap->appPhandle, BAP_PRIM, indPrim);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
        {
            CmExtScanSetGlobalParamsCfm *cfm = (CmExtScanSetGlobalParamsCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_START_SCAN,
                                                        &connection))
            {
                if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    /* Start Find Sync Train Procedure */
                    uint32 flags = 0;
                    uint16 scanForXSeconds = CM_EXT_SCAN_FOREVER;
                    uint16 adStructureFilter = CM_EXT_SCAN_AD_STRUCT_FILTER_NONE; /* 0 -, 1 -, 2 -, 3 - */
                    uint16 adStructureFilterSubField1 = CM_EXT_SCAN_SUB_FIELD_INVALID;
                    uint16 adStructureFilterSubField2 = CM_EXT_SCAN_SUB_FIELD_INVALID;
                    uint8 adStructureInfoLen = 0;
                    uint8* adStructureInfo = NULL;

                    CmPeriodicScanStartFindTrainsReqSend(CSR_BT_BAP_IFACEQUEUE,
                                                         flags,
                                                         scanForXSeconds,
                                                         adStructureFilter,
                                                         adStructureFilterSubField1,
                                                         adStructureFilterSubField2,
                                                         adStructureInfoLen,
                                                         adStructureInfo);

                }
                else
                {
                    bapBroadcastAssistantUtilsSendStartScanCfm(bap->appPhandle,
                                                               connection->cid,
                                                               result,
                                                               0);
                    /* Update the assistant state */
                    bapBroadcastAssistantUpdateState(connection,
                                                     BAP_ASSISTANT_STATE_IDLE);
                }
            }
        }
        break;
        case CSR_BT_CM_EXT_SCAN_CTRL_SCAN_INFO_IND:
        {
            CmExtScanCtrlScanInfoInd *ind = (CmExtScanCtrlScanInfoInd*)primitive;

            CSR_UNUSED(ind);
        }
        break;
        case CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM:
        {
            CmPeriodicScanStartFindTrainsCfm *cfm = (CmPeriodicScanStartFindTrainsCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_START_SCAN,
                                                        &connection))
            {
                bool responseOp;
                bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);
                if(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    result = BAP_RESULT_INPROGRESS;

                    /* Inform Delegator that Assistant is scanning SRC */
                    GattBassClientRemoteScanStartRequest(connection->bass.srvcHndl,
                                                         !responseOp);
                }
                bapBroadcastAssistantUtilsSendStartScanCfm(bap->appPhandle,
                                                           connection->cid,
                                                           result,
                                                           cfm->scanHandle);

                /* Update the assistant state, come back to idle state */
                bapBroadcastAssistantUpdateState(connection,
                                                 BAP_ASSISTANT_STATE_IDLE);
            }
        }
        break;
        case CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM:
        {
            CmPeriodicScanStopFindTrainsCfm *cfm = (CmPeriodicScanStopFindTrainsCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_STOP_SCAN,
                                                        &connection))
            {
                bool responseOp;
                bapBroadcastAssistantGetControlPointOp(connection, &responseOp, NULL);

                if(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    result = BAP_RESULT_SUCCESS;

                    /* Inform Delegator that Assistant is stopping scanning SRC */
                    GattBassClientRemoteScanStopRequest(connection->bass.srvcHndl,
                                                        !responseOp);
                }

                bapBroadcastAssistantUtilsSendStopScanCfm(bap->appPhandle,
                                                          connection->cid,
                                                          result);

                /* Update the assistant state */
                bapBroadcastAssistantUpdateState(connection,
                                                 BAP_ASSISTANT_STATE_IDLE);
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM:
        {
            CmPeriodicScanSyncToTrainCfm *cfm = (CmPeriodicScanSyncToTrainCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_SYNC_TO_SRC,
                                                        &connection))
            {
                if (cfm->resultCode == 0xFFFF)
                {
                    result = BAP_RESULT_INPROGRESS;
                }
                else if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    result = BAP_RESULT_SUCCESS;

                    /* Update the assistant state */
                    bapBroadcastAssistantUpdateState(connection,
                                                     BAP_ASSISTANT_STATE_IDLE);
                }
                else
                {
                    /* Update the assistant state */
                    bapBroadcastAssistantUpdateState(connection,
                                                     BAP_ASSISTANT_STATE_IDLE);
                }

                bapBroadcastAssistantUtilsSendSyncToSrcStartCfm(bap->appPhandle,
                                                                connection->cid,
                                                                cfm->syncHandle,
                                                                cfm->advSid,
                                                                cfm->advClockAccuracy,
                                                                cfm->advPhy,
                                                                cfm->periodicAdvInterval,
                                                                cfm->addrt,
                                                                result);
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM:
        {
            CmPeriodicScanSyncToTrainCancelCfm *cfm =
                (CmPeriodicScanSyncToTrainCancelCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_SYNC_TO_SRC,
                                                        &connection))
            {
                if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                    result = BAP_RESULT_SUCCESS;

                /* Update the assistant state */
                bapBroadcastAssistantUpdateState(connection,
                                                 BAP_ASSISTANT_STATE_IDLE);

                bapBroadcastAssistantUtilsSendSyncToSrcCancelCfm(bap->appPhandle,
                                                                 connection->cid,
                                                                 result);
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM:
        {
            CmPeriodicScanSyncTerminateCfm *cfm =
                (CmPeriodicScanSyncTerminateCfm *) primitive;

            if (findConnectionByBroadcastAssistantState(bap,
                                                        BAP_ASSISTANT_STATE_IDLE,
                                                        &connection))
            {

                if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                    result = BAP_RESULT_SUCCESS;

                bapBroadcastAssistantUtilsSendSyncToSrcTerminateCfm(bap->appPhandle,
                                                                    connection->cid,
                                                                    cfm->syncHandle,
                                                                    result);
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM:
        {
            CmPeriodicAdvSetTransferCfm *prim = (CmPeriodicAdvSetTransferCfm *) primitive;
            BAP_CLIENT_DEBUG("(BAP)  : CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM status =%x\n", prim->resultCode);
            CSR_UNUSED(prim);
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM:
        {
            CmPeriodicScanSyncTransferCfm *prim = (CmPeriodicScanSyncTransferCfm *) primitive;
            BAP_CLIENT_DEBUG("(BAP)  : CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM status =%x\n", prim->resultCode);
            CSR_UNUSED(prim);
        }
        break;

        case CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND:
        {
            CmPeriodicScanSyncLostInd *prim = (CmPeriodicScanSyncLostInd *)primitive;
            BAP_CLIENT_DEBUG("(BAP)  : CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND syncHandle =%x\n", prim->syncHandle);

            if (findConnectionByBroadcastAssistantState(bap,
                   BAP_ASSISTANT_STATE_IDLE,
                   &connection))
            {
                bapBroadcastAssistantUtilsSendSyncLossInd(bap->appPhandle,
                                                          connection->cid,
                                                          prim->syncHandle);
            }
        }
        break;

        default:
            BAP_CLIENT_WARNING(" BAP: CM broadcast msg not handled\n");
        break;
        }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
/** @}*/

