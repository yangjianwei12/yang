/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE

#include "csr_bt_util.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"
#ifndef EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif

void CsrBtAvrcpTgMpRegisterReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgMpRegisterReq *prim = (CsrBtAvrcpTgMpRegisterReq *)instData->recvMsgP;
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    CsrBtAvrcpTgMp *mp = CsrBtAvrcpTgUtilMpListAdd(instData, prim);
    if (!instData->tgLocal.mpDefault ||
        CSR_MASK_IS_SET(prim->configMask, CSR_BT_AVRCP_TG_MP_REGISTER_CONFIG_SET_DEFAULT))
    {/* No default player is set or this one should be set instead of the current one */
        instData->tgLocal.mpDefault = mp;
    }

    CsrBtAvrcpTgUtilAvailableMPChanged(instData);
#else
    CsrBtAvrcpTgMp *mp = (CsrBtAvrcpTgMp *) CsrPmemAlloc(sizeof(CsrBtAvrcpTgMp));
    mp->mpHandle            = prim->playerHandle;
    mp->mpId                = 0;
    mp->majorType           = prim->majorType;
    mp->subType             = prim->subType;
    mp->playerName          = prim->playerName;
    SynMemCpyS(mp->featureMask, sizeof(prim->featureMask), prim->featureMask, sizeof(prim->featureMask));

    instData->tgLocal->mpDefault = mp;
#endif

    CsrBtAvrcpTgMpRegisterCfmSend(mp->mpId, prim->playerHandle,
        CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
        CSR_BT_SUPPLIER_AVRCP);
}

void CsrBtAvrcpTgMpRegisterCfmSend(CsrUint32 playerId, CsrSchedQid phandle,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpTgMpRegisterCfm *prim = (CsrBtAvrcpTgMpRegisterCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type     = CSR_BT_AVRCP_TG_MP_REGISTER_CFM;
    prim->playerId = playerId;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(phandle, prim);
}

void CsrBtAvrcpTgPassThroughResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgPassThroughRes  *resMsg         = (CsrBtAvrcpTgPassThroughRes *)instData->recvMsgP;
    AvrcpConnInstance_t         *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t     *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if ((pendingMsgInfo->msgType == CSR_BT_AVRCP_TG_PASS_THROUGH_RES) && (resMsg->status == CSR_BT_AVRCP_PT_STATUS_ACCEPT))
        {
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
            if (AVRCP_DATA_PT_OPID_GET(pendingMsgInfo->rxData) == CSR_BT_AVRCP_PT_OP_ID_VENDOR_DEP)
            {/* Group navigation */
                CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_PT_GN_SIZE);

                CsrBtAvrcpDataInsertAvctpHeader(txData,
                                           AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                           AVRCP_DATA_AVCTP_CR_RES);

                CsrBtAvrcpDataInsertAvcCommonHeader(txData, CSR_BT_AVRCP_DATA_AVC_RTYPE_ACCEPTED);

                CsrBtAvrcpDataInsertAvcGroupNavigationHeader(txData,
                                                             AVRCP_DATA_PT_STATE_GET(pendingMsgInfo->rxData),
                                                             (CsrUint16)AVRCP_DATA_PT_GN_OPERATION_GET(pendingMsgInfo->rxData));

                CsrBtAvrcpControlDataSend(connInst, AVRCP_DATA_PT_GN_SIZE, txData);
            }
            else
#endif
            {/* Ordinary pass-through */
                CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_PT_SIZE);

                CsrBtAvrcpDataInsertAvctpHeader(txData,
                                           AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                           AVRCP_DATA_AVCTP_CR_RES);

                CsrBtAvrcpDataInsertAvcCommonHeader(txData, CSR_BT_AVRCP_DATA_AVC_RTYPE_ACCEPTED);

                CsrBtAvrcpDataInsertAvcPassThroughHeader(txData,
                                                    AVRCP_DATA_PT_STATE_GET(pendingMsgInfo->rxData),
                                                    AVRCP_DATA_PT_OPID_GET(pendingMsgInfo->rxData));

                CsrBtAvrcpControlDataSend(connInst, AVRCP_DATA_PT_SIZE, txData);
            }
        }
        else
        {
            if (resMsg->status == CSR_BT_AVRCP_PT_STATUS_NOT_IMPL)
            {
                CsrBtAvrcpTgRejectAvcSend(connInst, pendingMsgInfo->rxData, CSR_BT_AVRCP_DATA_AVC_RTYPE_NOT_IMP);
            }
            else
            {
                CsrBtAvrcpTgRejectAvcSend(connInst, pendingMsgInfo->rxData, CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED);
            }
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgPassThroughIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 opId, CsrUint8 state)
{
    CsrBtAvrcpTgPassThroughInd *prim = (CsrBtAvrcpTgPassThroughInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_PASS_THROUGH_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->operationId   = opId;
    prim->state         = state;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

#ifndef CSR_TARGET_PRODUCT_VM
void CsrBtAvrcpTgMpUnregisterReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgMpUnregisterReq *prim = (CsrBtAvrcpTgMpUnregisterReq *)instData->recvMsgP;
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    CsrBtAvrcpTgMp *mp = AVRCP_LIST_TG_MP_GET_ID((CsrCmnList_t *)&instData->tgLocal.mpList, prim->playerId);
#else
    CsrBtAvrcpTgMp *mp = instData->tgLocal.mpDefault;
#endif
    if (mp)
    {/* The currently addressed player must not be removed - check if is addressed by a remote device */
        AvrcpConnInstance_t *connInst;

        for (connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *)&instData->connList); connInst; connInst = connInst->next)
        {
            /* control channel could be disconnected but connInst can be 
                alive due to AVRCP Imaging Server Deactivation.
                So ensure we allow deregister players if control channel is 
                nonexistant */
            if ((connInst->control.state == AVRCP_STATE_CONN_CONNECTED) && (connInst->tgLocal->mpAddressed == mp))
            {
                CsrBtAvrcpTgMpUnregisterCfmSend(prim->playerId,
                    prim->phandle,
                    CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
                    CSR_BT_SUPPLIER_AVRCP);
                return;
            }
        }

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
        CsrBtAvrcpTgUtilAvailableMPChanged(instData);
#endif
        CsrBtAvrcpTgMpUnregisterCfmSend(prim->playerId,
            prim->phandle,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_SUPPLIER_AVRCP);

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
        CsrBtAvrcpTgUtilMpListRemove((CsrCmnListElm_t *)mp, NULL);
        AVRCP_LIST_TG_MP_REMOVE((CsrCmnList_t *)&instData->tgLocal.mpList, mp);

        if (instData->tgLocal.mpDefault == mp)
        {/* The default player is being removed - set the first player as default */
            instData->tgLocal.mpDefault = AVRCP_LIST_TG_MP_GET_FIRST((CsrCmnList_t *)&instData->tgLocal.mpList);
        }
#else
        CsrPmemFree(mp->playerName);
        mp->playerName = NULL;
        CsrPmemFree(instData->tgLocal->mpDefault);
        instData->tgLocal->mpDefault=NULL;
#endif
    }
    else
    {
        CsrBtAvrcpTgMpUnregisterCfmSend(prim->playerId,
            prim->phandle,
            CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
            CSR_BT_SUPPLIER_AVRCP);
    }
}

void CsrBtAvrcpTgMpUnregisterCfmSend(CsrUint32 playerId, CsrSchedQid phandle,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpTgMpUnregisterCfm *prim = (CsrBtAvrcpTgMpUnregisterCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type     = CSR_BT_AVRCP_TG_MP_UNREGISTER_CFM;
    prim->playerId = playerId;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(phandle, prim);
}
#endif /* !CSR_TARGET_PRODUCT_VM */

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
void CsrBtAvrcpTgNotiResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgNotiRes     *resMsg         = (CsrBtAvrcpTgNotiRes *)instData->recvMsgP;
    AvrcpConnInstance_t     *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (resMsg->notiId == CSR_BT_AVRCP_NOTI_ID_UIDS)
    {
        instData->tgLocal.uidCount = CSR_GET_UINT16_FROM_BIG_ENDIAN(resMsg->notiData);
    }

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrUint8 notiIndex = pendingMsgInfo->rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX] - CSR_BT_AVRCP_NOTI_ID_OFFSET;

            CSR_BIT_SET(connInst->tgLocal->notificationsActive, notiIndex);
            connInst->tgLocal->notiList[notiIndex] = AVRCP_TLABEL_GET(pendingMsgInfo->rxData);

            CsrBtAvrcpTgRegisterNotificationRspSend(connInst,
                                               pendingMsgInfo->rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX],
                                               resMsg->notiData,
                                               CSR_BT_AVRCP_DATA_AVC_RTYPE_INTERIM, FALSE);
        }
        else if (resMsg->status == CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED)
        {
            CsrUint8 notiIndex = pendingMsgInfo->rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX] - CSR_BT_AVRCP_NOTI_ID_OFFSET;

            connInst->tgLocal->notiList[notiIndex] = AVRCP_TLABEL_GET(pendingMsgInfo->rxData);
            CsrBtAvrcpTgRegisterNotificationRspSend(connInst,
                                                    pendingMsgInfo->rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX],
                                                    resMsg->notiData,
                                                    CSR_BT_AVRCP_DATA_AVC_RTYPE_CHANGED, FALSE);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
    else if (resMsg->status == CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED && connInst)
    {
        CsrBtAvrcpTgRegisterNotificationRspSend(connInst,
                                                resMsg->notiId,
                                                resMsg->notiData,
                                                CSR_BT_AVRCP_DATA_AVC_RTYPE_CHANGED, FALSE);
    }
}

void CsrBtAvrcpTgNotiIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 notiType, CsrUint32 playbackInterval)
{
    CsrBtAvrcpTgNotiInd *prim = (CsrBtAvrcpTgNotiInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_NOTI_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->notiId        = notiType;
    prim->playbackInterval = playbackInterval;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

void CsrBtAvrcpTgSetVolumeIndSend(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData)
{
    CsrBtAvrcpTgSetVolumeInd *prim = (CsrBtAvrcpTgSetVolumeInd *) CsrPmemZalloc(sizeof(*prim));

/* Do not store the indication info into pending msg list and direct
 * forward to application. This is to avoid un-necessary memory allocation
 * for non-Auto product requirement */
#ifndef CSR_BT_AVRCP_IGNORE_PENDING_LIST
    AvrcpTgPendingMsgInfo_t *pendingMsgInfo;

    pendingMsgInfo = CsrBtAvrcpTgUtilMsgQueueAdd(connInst,
                                                 CSR_BT_AVCTP_PSM,
                                                 CSR_BT_AVRCP_TG_SET_VOLUME_RES,/* Pending message type */
                                                 CSR_BT_AVRCP_TIMER_MTC,        /* Time */
                                                 rxDataLen,
                                                 rxData);                       /* Received command for later use */
    prim->msgId    = pendingMsgInfo->msgId;
    prim->volume   = (pendingMsgInfo->rxData)[AVRCP_DATA_PDU_SET_VOLUME_CMD_INDEX];
    prim->tLabel   = AVRCP_TLABEL_GET(pendingMsgInfo->rxData);
#else
    prim->msgId    = AVRCP_MSG_ID_ASSIGN(connInst->instData->tgLocal.tgMsgId);
    prim->volume   = (*rxData)[AVRCP_DATA_PDU_SET_VOLUME_CMD_INDEX];
    prim->tLabel   = AVRCP_TLABEL_GET(*rxData);
    CSR_UNUSED(rxDataLen);
#endif /* CSR_BT_AVRCP_IGNORE_PENDING_LIST */

    prim->type         = CSR_BT_AVRCP_TG_SET_VOLUME_IND;
    prim->connectionId = connInst->appConnId;
    prim->playerId     = connInst->tgLocal->mpAddressed->mpId;

    CsrBtAvrcpMessagePut(connInst->tgLocal->mpAddressed->mpHandle, prim);
}

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
void CsrBtAvrcpTgPasSetReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgPasSetReq *reqMsg = (CsrBtAvrcpTgPasSetReq *)instData->recvMsgP;
    AvrcpConnInstance_t *connInst, *currentConnInst = NULL;
    CsrUint32 playerId = reqMsg->playerId;

    for (connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *)&instData->connList); connInst; connInst = connInst->next)
    {
        if (CSR_MASK_IS_SET(connInst->tgLocal->notificationsActive, CSR_BT_AVRCP_NOTI_FLAG_PAS) &&
            (connInst->control.state == AVRCP_STATE_CONN_CONNECTED))
        {/* The remote controller requests a changed event for the PAS */
            currentConnInst = connInst;
            CsrBtAvrcpTgRegisterNotificationPasRspSend(connInst, CSR_BT_AVRCP_DATA_AVC_RTYPE_CHANGED, reqMsg->attValPairCount, reqMsg->attValPair);
            CSR_MASK_UNSET(connInst->tgLocal->notificationsActive, CSR_BT_AVRCP_NOTI_FLAG_PAS);
        }
    }

    if (currentConnInst != NULL)
    {
        playerId = currentConnInst->appConnId;
    }
    CsrBtAvrcpTgPasSetCfmSend(playerId,reqMsg->phandle,
        CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
        CSR_BT_SUPPLIER_AVRCP);
    CsrPmemFree(reqMsg->attValPair);
}

void CsrBtAvrcpTgPasSetResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgPasSetRes  *resMsg         = (CsrBtAvrcpTgPasSetRes *)instData->recvMsgP;
    AvrcpConnInstance_t     *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpDataSimpleVendorFrameSend(connInst,
                                           AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                           AVRCP_DATA_AVCTP_CR_RES,
                                           CSR_BT_AVRCP_DATA_AVC_RTYPE_ACCEPTED,
                                           AVRCP_DATA_PDU_ID_SET_PAS_VALUES);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgPasCurrentResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgPasCurrentRes  *resMsg         = (CsrBtAvrcpTgPasCurrentRes *)instData->recvMsgP;
    AvrcpConnInstance_t         *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t     *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (pendingMsgInfo->rxData[AVRCP_DATA_MD_PDU_ID_INDEX] == AVRCP_DATA_PDU_ID_REG_NOTI)
        {
            CsrUint8 notiIndex = pendingMsgInfo->rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX] - CSR_BT_AVRCP_NOTI_ID_OFFSET;

            CSR_BIT_SET(connInst->tgLocal->notificationsActive, notiIndex);
            connInst->tgLocal->notiList[notiIndex] = AVRCP_TLABEL_GET(pendingMsgInfo->rxData);

            CsrBtAvrcpTgRegisterNotificationPasRspSend(connInst, CSR_BT_AVRCP_DATA_AVC_RTYPE_INTERIM, resMsg->attValPairCount, resMsg->attValPair);
        }
        else if (pendingMsgInfo->rxData[AVRCP_DATA_MD_PDU_ID_INDEX] == AVRCP_DATA_PDU_ID_GET_CUR_PAS_VALUES)
        {/* Send response */
            CsrUint8 *txData   = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_HEADER_SIZE + (resMsg->attValPairCount*sizeof(CsrBtAvrcpPasAttValPair)));

            CsrBtAvrcpDataVendorDataInsert(txData,                              /* Data to transmit */
                                      AVRCP_TLABEL_GET(pendingMsgInfo->rxData), /* Transaction label */
                                      AVRCP_DATA_AVCTP_CR_RES,                  /* Command or response */
                                      CSR_BT_AVRCP_DATA_AVC_RTYPE_STABLE,       /* Command/response type */
                                      AVRCP_DATA_PDU_ID_GET_CUR_PAS_VALUES,     /* PDU ID */
                                      (CsrUint16)(AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_HEADER_SIZE + (resMsg->attValPairCount*sizeof(CsrBtAvrcpPasAttValPair)))); /* MD parameter length */

            txData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_NUM_INDEX] = resMsg->attValPairCount;
            SynMemCpyS(&txData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_ATT_ID_INDEX],
                       resMsg->attValPairCount*sizeof(CsrBtAvrcpPasAttValPair),
                       resMsg->attValPair,
                       resMsg->attValPairCount*sizeof(CsrBtAvrcpPasAttValPair));

            CsrBtAvrcpControlDataSend(connInst, (CsrUint16)(AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_HEADER_SIZE + (resMsg->attValPairCount*sizeof(CsrBtAvrcpPasAttValPair))), txData);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }
        /* In any case, an answer has been issued, so remove the message from the pending list to avoid a second response */
        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }

    CsrPmemFree(resMsg->attValPair);
}

void CsrBtAvrcpTgPasSetCfmSend(CsrUint32 playerId, CsrSchedQid phandle,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpTgPasSetCfm *prim = (CsrBtAvrcpTgPasSetCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_PAS_SET_CFM;
    prim->playerId      = playerId;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(phandle, prim);
}

void CsrBtAvrcpTgPasCurrentIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 attIdCount, CsrBtAvrcpPasAttId *attId)
{
    CsrBtAvrcpTgPasCurrentInd *prim = (CsrBtAvrcpTgPasCurrentInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_PAS_CURRENT_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->attIdCount    = attIdCount;
    prim->attId         = attId;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

void CsrBtAvrcpTgPasSetIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 attValPairCount, CsrBtAvrcpPasAttValPair* changedPas)
{
    CsrBtAvrcpTgPasSetInd *prim = (CsrBtAvrcpTgPasSetInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type              = CSR_BT_AVRCP_TG_PAS_SET_IND;
    prim->connectionId      = pendingMsgInfo->connInst->appConnId;
    prim->playerId          = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->attValPairCount   = attValPairCount;
    prim->attValPair        = changedPas;
    prim->msgId             = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

void CsrBtAvrcpTgInformDispCharsetIndSend(CsrUint8 conId, CsrUint32 playerId, CsrSchedQid mpHandle, CsrUint8 charsetCount, CsrBtAvrcpCharSet *charset)
{
    CsrBtAvrcpTgInformDispCharsetInd *prim = (CsrBtAvrcpTgInformDispCharsetInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_INFORM_DISP_CHARSET_IND;
    prim->connectionId  = conId;
    prim->playerId      = playerId;
    prim->charsetCount  = charsetCount;
    prim->charset       = charset;

    CsrBtAvrcpMessagePut(mpHandle, prim);
}

void CsrBtAvrcpTgBatteryStatusOfCtIndSend(CsrUint8 conId, CsrSchedQid mpHandle, CsrUint32 playerId, CsrUint8 battLevel)
{
    CsrBtAvrcpTgInformBatteryStatusInd *prim = (CsrBtAvrcpTgInformBatteryStatusInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_INFORM_BATTERY_STATUS_IND;
    prim->connectionId  = conId;
    prim->playerId      = playerId;
    prim->batStatus     = battLevel;

    CsrBtAvrcpMessagePut(mpHandle, prim);
}
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpTgSearchResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSearchRes   *resMsg         = (CsrBtAvrcpTgSearchRes *)instData->recvMsgP;
    AvrcpConnInstance_t     *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    instData->tgLocal.uidCount = resMsg->uidCounter;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgSearchRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->status, resMsg->uidCounter, resMsg->numberOfItems);
        }
        else
        {
            CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
        }
        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList,
                                  pendingMsgInfo);
    }
}

void CsrBtAvrcpTgChangePathResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgChangePathRes  *resMsg         = (CsrBtAvrcpTgChangePathRes *)instData->recvMsgP;
    AvrcpConnInstance_t         *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t     *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgChangePathRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->status, resMsg->itemsCount);
        }
        else
        {
            CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgGetFolderItemsResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgGetFolderItemsRes *resMsg         = (CsrBtAvrcpTgGetFolderItemsRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    instData->tgLocal.uidCount = resMsg->uidCounter;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {/* resMsg->items* is already preformatted as a single packet AVCTP frame - insert remaining fields */
            CsrBtAvrcpTgGetFolderItemsRspSend(connInst,
                                         AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                         resMsg->itemsLen,
                                         resMsg->items,
                                         resMsg->itemsCount,
                                         resMsg->uidCounter);
        }
        else
        {
            CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
            CsrPmemFree(resMsg->items);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
    else
    {
        CsrPmemFree(resMsg->items);
    }
}

void CsrBtAvrcpTgGetTotalNumberOfItemsResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgGetTotalNumberOfItemsRes *resMsg         = (CsrBtAvrcpTgGetTotalNumberOfItemsRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    instData->tgLocal.uidCount = resMsg->uidCounter;
    
    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgGetTotalNumberOfItemsRspSend(connInst,
                                         AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                         resMsg->noOfItems,
                                         resMsg->uidCounter);
        }
        else
        {
            CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgSetBrowsedPlayerResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSetBrowsedPlayerRes *resMsg         = (CsrBtAvrcpTgSetBrowsedPlayerRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    instData->tgLocal.uidCount = resMsg->uidCounter;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgMp *mp = AVRCP_LIST_TG_MP_GET_ID((CsrCmnList_t *)&instData->tgLocal.mpList, resMsg->playerId);

            if (mp)
            {
                CsrUint8 *data;
                CsrUint16 dataLen;

                CsrBtAvrcpTgUtilMpListUpdateBrowsed(connInst, mp);

                resMsg->folderDepth = CsrBtAvrcpTgUtilSBPFolderNamesAdd(&data,&dataLen,(CsrCharString *)(resMsg->folderNames));

                CsrBtAvrcpTgSetBrowsedPlayerRspSend(connInst,
                                               AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                               resMsg->status,
                                               resMsg->uidCounter,
                                               resMsg->itemsCount,
                                               resMsg->folderDepth,
                                               dataLen,
                                               data);
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
                CsrBtAvrcpImagingServerMpHandleUpdateReqSend(connInst->appConnId, connInst->tgLocal->mpBrowsed->mpHandle);
#endif
                CsrPmemFree(data);
            }
            else
            {
                CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, CSR_BT_AVRCP_STATUS_INTERNAL_ERROR);
            }
        }
        else
        {
            CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
    CsrPmemFree(resMsg->folderNames);
}

void CsrBtAvrcpTgSearchIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrCharString *text)
{
    CsrBtAvrcpTgSearchInd *prim = (CsrBtAvrcpTgSearchInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_SEARCH_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->text          = text;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}
void CsrBtAvrcpTgChangePathIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpFolderDirection folderDir, CsrUint8 *folderUid)
{
    CsrBtAvrcpTgChangePathInd *prim = (CsrBtAvrcpTgChangePathInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_CHANGE_PATH_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpId;
    prim->folderDir     = folderDir;
    prim->msgId         = pendingMsgInfo->msgId;
    CSR_BT_AVRCP_UID_COPY(prim->folderUid, folderUid);
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

void CsrBtAvrcpTgGetFolderItemsIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope, CsrUint32 startItem, CsrUint32 endItem, CsrUint32 attribMask, CsrUint32 maxData)
{
    CsrBtAvrcpTgGetFolderItemsInd *prim = (CsrBtAvrcpTgGetFolderItemsInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    prim->scope         = scope;
    prim->startItem     = startItem;
    prim->endItem       = endItem;
    prim->attributeMask = attribMask;
    prim->maxData       = maxData;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpHandle, prim);
}

void CsrBtAvrcpTgGetTotalNumberOfItemsIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope)
{
    CsrBtAvrcpTgGetTotalNumberOfItemsInd *prim = (CsrBtAvrcpTgGetTotalNumberOfItemsInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    prim->scope         = scope;
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpHandle, prim);
}

void CsrBtAvrcpTgSetBrowsedPlayerIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp)
{
    CsrBtAvrcpTgSetBrowsedPlayerInd *prim = (CsrBtAvrcpTgSetBrowsedPlayerInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = mp->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    prim->phandle       = mp->mpHandle;
    CsrBtAvrcpMessagePut(mp->mpHandle, prim);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined(INSTALL_AVRCP_METADATA_ATTRIBUTES)
void CsrBtAvrcpTgGetAttributesResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgGetAttributesRes   *resMsg         = (CsrBtAvrcpTgGetAttributesRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (((resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE) ||
            (resMsg->status == CSR_BT_AVRCP_STATUS_UID_CHANGED)) && (resMsg->attribDataLen != 0))
        {
            CsrUint8 *pData;

            if (pendingMsgInfo->psm == CSR_BT_AVCTP_PSM)
            {/* GetElementAttributes (v1.3) */
#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
                CsrUint8 headerSize = AVRCP_DATA_MD_HEADER_SIZE - 1;
                CsrBtAvrcpTgUtilContinuingRspReset(connInst, TRUE); /* Make sure continuation info is completely reset */
                /* The application does not allocate room for headers; make sure to do it here */
                resMsg->attribDataLen += headerSize; 
                pData = CsrPmemAlloc(resMsg->attribDataLen);
                SynMemCpyS(&pData[headerSize], (resMsg->attribDataLen - headerSize), resMsg->attribData,(resMsg->attribDataLen - headerSize));
                CsrPmemFree(resMsg->attribData);
                resMsg->attribData = pData; 

                connInst->tgLocal->pduId             = AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES;
                connInst->tgLocal->getAttribResPrim  = resMsg;
                instData->recvMsgP                  = NULL;

                CsrBtAvrcpTgGetElementAttributesRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData));
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */
            }
            else
            {/* GetItemAttributes (v1.4+) - is never fragmented */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                resMsg->attribDataLen += AVRCP_DATA_BROWSING_HEADER_SIZE;
                pData = CsrPmemAlloc(resMsg->attribDataLen);

                SynMemCpyS(&pData[AVRCP_DATA_BROWSING_HEADER_SIZE],
                           (resMsg->attribDataLen - AVRCP_DATA_BROWSING_HEADER_SIZE),
                           resMsg->attribData,
                           (resMsg->attribDataLen - AVRCP_DATA_BROWSING_HEADER_SIZE));

                CsrBtAvrcpTgGetItemAttributesRspSend(connInst,
                                                AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                                resMsg->attribDataLen,
                                                pData,
                                                resMsg->attribCount,
                                                resMsg->status);
                CsrPmemFree(resMsg->attribData);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
            }
        }
        else
        {
            if (pendingMsgInfo->psm == CSR_BT_AVCTP_PSM)
            {/* GetElementAttributes (v1.3) */
                CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                    pendingMsgInfo->rxData,
                    CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                    resMsg->status,
                    CSR_BT_AVRCP_TLABEL_INVALID,
                    AVRCP_DATA_PDU_ID_INVALID);
            }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
            else
            {/* GetItemAttributes (v1.4+) */
                CsrBtAvrcpTgNormalRejectBrowsingSend(connInst, pendingMsgInfo->rxData, resMsg->status);
            }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
            CsrPmemFree(resMsg->attribData);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
    else
    {
        CsrPmemFree(resMsg->attribData);
    }
}

void CsrBtAvrcpTgGetAttributesIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp, CsrBtAvrcpItemAttMask attribMask, CsrUint32 maxData, CsrBtAvrcpScope scope, CsrBtAvrcpUid uid, CsrUint16 uidCounter)
{
    CsrBtAvrcpTgGetAttributesInd *prim = (CsrBtAvrcpTgGetAttributesInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_GET_ATTRIBUTES_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = mp->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    prim->attributeMask = attribMask;
    prim->maxData       = maxData;
    prim->scope         = scope;
    prim->uidCounter    = uidCounter;
    CSR_BT_AVRCP_UID_COPY(prim->uid, uid);
    CsrBtAvrcpMessagePut(mp->mpHandle, prim);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || INSTALL_AVRCP_METADATA_ATTRIBUTES */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
void CsrBtAvrcpTgGetPlayStatusResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgGetPlayStatusRes  *resMsg         = (CsrBtAvrcpTgGetPlayStatusRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        CsrBool removePmsg = TRUE;

        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            /* Determine if the response is for a GetPlayStatus or actually a GetFolderItems (Media Players) */
            if (connInst->tgLocal->currentPlayer)
            {/* GetFolderItems */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                connInst->tgLocal->currentPlayer->playStatus = resMsg->playStatus;

                /* Skip to next media player */
                connInst->tgLocal->currentPlayer = connInst->tgLocal->currentPlayer->next;
                connInst->tgLocal->itemCnt--;

                if ((connInst->tgLocal->currentPlayer) && (connInst->tgLocal->itemCnt > 0))
                {
                    removePmsg = FALSE;
                    CsrBtAvrcpTgUtilPendingMsgUpdate(pendingMsgInfo);
                    CsrBtAvrcpTgGetPlayStatusIndSend(pendingMsgInfo, connInst->tgLocal->currentPlayer);
                }
                else
                {/* Status has been retrieved for all players - send GetFolderItems response */
                    CsrUint8 *txData;
                    CsrUint16 txDataLen;
                    CsrUint32 startIdx = CSR_GET_UINT32_FROM_BIG_ENDIAN(&(pendingMsgInfo->rxData)[AVRCP_DATA_PDU_GFI_CMD_START_INDEX]);
                    CsrUint32 endIdx = CSR_GET_UINT32_FROM_BIG_ENDIAN(&(pendingMsgInfo->rxData)[AVRCP_DATA_PDU_GFI_CMD_END_INDEX]);
                    CsrUint32 totalCnt = (CsrUint32)(endIdx - startIdx + 1);

                    if ((AVRCP_LIST_TG_MP_GET_COUNT((CsrCmnList_t *)&connInst->instData->tgLocal.mpList) - startIdx) <= totalCnt )
                    {
                        totalCnt = (CsrUint16)(AVRCP_LIST_TG_MP_GET_COUNT((CsrCmnList_t *)&connInst->instData->tgLocal.mpList) - startIdx);
                    }

                    CsrBtAvrcpTgUtilGetFolderItemsMPListBuild(instData, &txDataLen, &txData, startIdx, endIdx);
                    CsrBtAvrcpTgGetFolderItemsRspSend(connInst,
                                                 AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                                 txDataLen,
                                                 txData,
                                                 (CsrUint16) totalCnt,
                                                 instData->tgLocal.uidCount);
                    connInst->tgLocal->currentPlayer = NULL;
                }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
            }
            else
            {/* GetPlayStatus */
                CsrBtAvrcpTgGetPlayStatusRspSend(connInst,
                                            AVRCP_TLABEL_GET(pendingMsgInfo->rxData),
                                            resMsg->songLength,
                                            resMsg->songPosition,
                                            resMsg->playStatus);
            }
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        if (removePmsg)
        {
            CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
            AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
        }
    }
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

void CsrBtAvrcpTgGetPlayStatusIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp)
{
    CsrBtAvrcpTgGetPlayStatusInd *prim = (CsrBtAvrcpTgGetPlayStatusInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_GET_PLAY_STATUS_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = mp->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(mp->mpHandle, prim);
}

void CsrBtAvrcpTgNotiCfmSend(CsrUint32 playerId, CsrSchedQid phandle,
    CsrBtAvrcpNotiId notiType, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpTgNotiCfm *prim = (CsrBtAvrcpTgNotiCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_NOTI_CFM;
    prim->playerId      = playerId;
    prim->notiId        = notiType;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(phandle, prim);
}

void CsrBtAvrcpTgNotiReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgNotiReq *reqMsg = (CsrBtAvrcpTgNotiReq *)instData->recvMsgP;
    AvrcpConnInstance_t *connInst;
    CsrBtAvrcpTgMp *mp = AVRCP_LIST_TG_MP_GET_ID((CsrCmnList_t *)&instData->tgLocal.mpList, reqMsg->playerId);
    
    if (reqMsg->notiId == CSR_BT_AVRCP_NOTI_ID_UIDS)
    {
        instData->tgLocal.uidCount = CSR_GET_UINT16_FROM_BIG_ENDIAN(reqMsg->notiData);
    }

    if (mp)
    {
        for (connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *)&instData->connList); connInst; connInst = connInst->next)
        {
            if (CSR_BIT_IS_SET(connInst->tgLocal->notificationsActive, reqMsg->notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET) &&
                (connInst->control.state == AVRCP_STATE_CONN_CONNECTED))
            {/* The remote controller has requested a changed event for the notification */
                CsrBtAvrcpTgRegisterNotificationRspSend(connInst,
                                                   reqMsg->notiId,
                                                   reqMsg->notiData,
                                                   CSR_BT_AVRCP_DATA_AVC_RTYPE_CHANGED, FALSE);
                CSR_BIT_UNSET(connInst->tgLocal->notificationsActive, reqMsg->notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET);
            }
        }

        CsrBtAvrcpTgNotiCfmSend(reqMsg->playerId, mp->mpHandle, reqMsg->notiId,
                CSR_BT_RESULT_CODE_AVRCP_SUCCESS, CSR_BT_SUPPLIER_AVRCP);
    }
}

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
void CsrBtAvrcpTgSetAddressedPlayerCfmSend(CsrUint32 playerId, CsrSchedQid phandle,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtAvrcpTgSetAddressedPlayerCfm *prim = (CsrBtAvrcpTgSetAddressedPlayerCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type     = CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_CFM;
    prim->playerId = playerId;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;

    CsrBtAvrcpMessagePut(phandle, prim);
}

void CsrBtAvrcpTgSetAddressedPlayerIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp)
{
    CsrBtAvrcpTgSetAddressedPlayerInd *prim = (CsrBtAvrcpTgSetAddressedPlayerInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = mp->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    CsrBtAvrcpMessagePut(mp->mpHandle, prim);
}

void CsrBtAvrcpTgSetAddressedPlayerReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSetAddressedPlayerReq *reqMsg = (CsrBtAvrcpTgSetAddressedPlayerReq *)instData->recvMsgP;
    CsrBtAvrcpTgMp *mp = AVRCP_LIST_TG_MP_GET_ID((CsrCmnList_t *)&instData->tgLocal.mpList, reqMsg->playerId);

    instData->tgLocal.uidCount = reqMsg->uidCounter;

    if (mp)
    {
        CsrBtAvrcpTgUtilMpListUpdateAddressed(instData, mp, reqMsg->uidCounter);
        CsrBtAvrcpTgSetAddressedPlayerCfmSend(mp->mpId, reqMsg->phandle,
            CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
            CSR_BT_SUPPLIER_AVRCP);
    }
    else
    {
        CsrBtAvrcpTgSetAddressedPlayerCfmSend(reqMsg->playerId, reqMsg->phandle,
            CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
            CSR_BT_SUPPLIER_AVRCP);
    }
}

void CsrBtAvrcpTgSetAddressedPlayerResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSetAddressedPlayerRes   *resMsg         = (CsrBtAvrcpTgSetAddressedPlayerRes *)instData->recvMsgP;
    AvrcpConnInstance_t                 *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t             *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;
 
    instData->tgLocal.uidCount = resMsg->uidCounter;
    
    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgMp *mp = AVRCP_LIST_TG_MP_GET_ID((CsrCmnList_t *)&instData->tgLocal.mpList, resMsg->playerId);

            if (mp)
            {
                CsrBtAvrcpTgUtilMpListUpdateAddressed(instData, mp, resMsg->uidCounter);
                CsrBtAvrcpTgSetAddressedPlayerRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->status, CSR_BT_AVRCP_DATA_AVC_RTYPE_ACCEPTED);
            }
            else
            {
                CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                    pendingMsgInfo->rxData,
                    CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                    CSR_BT_AVRCP_STATUS_INTERNAL_ERROR,
                    CSR_BT_AVRCP_TLABEL_INVALID,
                    AVRCP_DATA_PDU_ID_INVALID);
            }
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpTgPlayResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgPlayRes     *resMsg         = (CsrBtAvrcpTgPlayRes *)instData->recvMsgP;
    AvrcpConnInstance_t     *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgPlayRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->status);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgAddToNowPlayingResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgAddToNowPlayingRes  *resMsg         = (CsrBtAvrcpTgAddToNowPlayingRes *)instData->recvMsgP;
    AvrcpConnInstance_t             *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t         *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgAddToNPLRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->status);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList, pendingMsgInfo);
    }
}

void CsrBtAvrcpTgPlayIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpScope scope, CsrUint16 uidCounter, CsrUint8 *uid)
{
    CsrBtAvrcpTgPlayInd *prim = (CsrBtAvrcpTgPlayInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_PLAY_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpAddressed->mpId;
    prim->scope         = scope;
    prim->uidCounter    = uidCounter;
    prim->msgId         = pendingMsgInfo->msgId;
    CSR_BT_AVRCP_UID_COPY(prim->uid, uid);
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpAddressed->mpHandle, prim);
}

void CsrBtAvrcpTgAddToNPLIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope, CsrUint16 uidCounter, CsrUint8 *uid)
{
    CsrBtAvrcpTgAddToNowPlayingInd *prim = (CsrBtAvrcpTgAddToNowPlayingInd *) CsrPmemZalloc(sizeof(*prim));

    prim->type          = CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_IND;
    prim->connectionId  = pendingMsgInfo->connInst->appConnId;
    prim->playerId      = pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpId;
    prim->msgId         = pendingMsgInfo->msgId;
    prim->scope         = scope;
    prim->uidCounter    = uidCounter;
    CSR_BT_AVRCP_UID_COPY(prim->uid, uid);
    CsrBtAvrcpMessagePut(pendingMsgInfo->connInst->tgLocal->mpBrowsed->mpHandle, prim);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#ifdef CSR_BT_AVRCP_IGNORE_PENDING_LIST
void CsrBtAvrcpTgSetVolumeResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSetVolumeRes  *resMsg   = (CsrBtAvrcpTgSetVolumeRes *)instData->recvMsgP;
    AvrcpConnInstance_t       *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);

    if (connInst)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgSetVolumeRspSend(connInst, resMsg->tLabel, resMsg->volume);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                NULL,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                resMsg->tLabel,
                AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME);
        }
    }
}
#else
void CsrBtAvrcpTgSetVolumeResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpTgSetVolumeRes   *resMsg         = (CsrBtAvrcpTgSetVolumeRes *)instData->recvMsgP;
    AvrcpConnInstance_t         *connInst       = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    AvrcpTgPendingMsgInfo_t     *pendingMsgInfo = connInst ? AVRCP_LIST_TG_PMSG_GET_MSGID((CsrCmnList_t *)&connInst->tgLocal->pendingMsgList, resMsg->msgId) : NULL;

    if (pendingMsgInfo)
    {
        if (resMsg->status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
        {
            CsrBtAvrcpTgSetVolumeRspSend(connInst, AVRCP_TLABEL_GET(pendingMsgInfo->rxData), resMsg->volume);
        }
        else
        {
            CsrBtAvrcpTgRejectAvcVendorSend(connInst,
                pendingMsgInfo->rxData,
                CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED,
                resMsg->status,
                CSR_BT_AVRCP_TLABEL_INVALID,
                AVRCP_DATA_PDU_ID_INVALID);
        }

        CsrBtAvrcpTgUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
        AVRCP_LIST_TG_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->tgLocal->pendingMsgList,
                                  pendingMsgInfo);
    }
}
#endif /* CSR_BT_AVRCP_IGNORE_PENDING_LIST */
#endif /* CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER */
#endif /* ! EXCLUDE_CSR_BT_AVRCP_TG_MODULE */
#endif /* ! EXCLUDE_CSR_BT_AVRCP_MODULE */

