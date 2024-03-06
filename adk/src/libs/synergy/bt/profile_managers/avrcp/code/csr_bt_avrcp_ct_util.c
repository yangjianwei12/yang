/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE

#include "csr_bt_util.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"

static CsrBool csrBtAvrcpCtMsgFindChannelTLabel(CsrCmnListElm_t *elem,
                                                void *value)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo = (AvrcpCtPendingMsgInfo_t *) elem;
    CsrBtAvrcpChannelTLabel *channelTLabel = (CsrBtAvrcpChannelTLabel *) value;

    if (pendingMsgInfo->tLabel == channelTLabel->tLabel
        && pendingMsgInfo->psm == channelTLabel->psm)
    {
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

void CsrBtAvrcpCtCompletePendingMsg(AvrcpCtPendingMsgInfo_t *pendingMsgInfo)
{
    AvrcpConnInstance_t *connInst = pendingMsgInfo->connInst;
    AvrcpCtPendingMsgInfo_t *regNotifPendingMsg = connInst->pendingNotifReg;

    if (pendingMsgInfo->phandle != CSR_SCHED_QID_INVALID)
    {
        CsrBtAvrcpMessagePut(pendingMsgInfo->phandle, pendingMsgInfo->cfmMsg);
    }
    else
    {
        CsrPmemFree(pendingMsgInfo->cfmMsg);
    }

    pendingMsgInfo->cfmMsg = NULL;
    CsrBtAvrcpCtUtilMsgQueueRemove((CsrCmnListElm_t *)pendingMsgInfo, NULL);
    AVRCP_LIST_CT_PMSG_REMOVE((CsrCmnList_t *)&pendingMsgInfo->connInst->ctLocal->pendingMsgList,
                              pendingMsgInfo);

    /* Register pending notifications if any */
    if (regNotifPendingMsg)
    {
        CsrBtAvrcpCtRegisterNextNotification(connInst,
                                             regNotifPendingMsg);
    }
}

void CsrBtAvrcpCtUtilConvertAVCRspType(CsrUint8 rspType,
    CsrBtResultCode *resultCode, CsrBtSupplier *resultSupplier)
{
    switch (rspType)
    {
        case CSR_BT_AVRCP_DATA_AVC_RTYPE_ACCEPTED: /* Intentional fall-through */
        case CSR_BT_AVRCP_DATA_AVC_RTYPE_STABLE:
        {
            *resultCode = CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
            *resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
        break;

        case CSR_BT_AVRCP_DATA_AVC_RTYPE_NOT_IMP: /* Intentional fall-through */
        case CSR_BT_AVRCP_DATA_AVC_RTYPE_REJECTED:
        {
            *resultCode = rspType;
            *resultSupplier = CSR_BT_SUPPLIER_AVC;
        }
        break;

        default:
        {
            *resultCode = CSR_BT_RESULT_CODE_AVRCP_UNSPECIFIED_ERROR;
            *resultSupplier = CSR_BT_SUPPLIER_AVRCP; /* Sanity checking */
        }
    }
}

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
void CsrBtAvrcpCtUtilConvertOperationStatus(CsrUint8 status,
                                            CsrBtResultCode *resultCode,
                                            CsrBtSupplier *resultSupplier)
{
    if (status == CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE)
    {
        *resultCode = CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
        *resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }
    else
    {
        *resultCode = status;
        *resultSupplier = CSR_BT_SUPPLIER_AVCTP;
    }
}
#endif

void CsrBtAvrcpCtPendingMsgTimeout(CsrUint16 dummy, void *pendingMsg)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo = (AvrcpCtPendingMsgInfo_t *)pendingMsg;
    pendingMsgInfo->timerId = 0;
    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(dummy);
}

static const CsrUint8 avrcpCtUtilResultCodeOffsets[CSR_BT_AVRCP_CT_PRIM_CFM_HIGHEST - CSR_BT_AVRCP_CT_PRIM_UPSTREAM_LOWEST + 1] =
{/* Order MUST correspond to the numbering of the primitives */
    offsetof(CsrBtAvrcpCtInformDispCharsetCfm, resultCode),
    offsetof(CsrBtAvrcpCtNotiRegisterCfm, resultCode),
    offsetof(CsrBtAvrcpCtGetPlayStatusCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasAttIdCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasValIdCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasAttTxtCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasValTxtCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasCurrentCfm, resultCode),
    offsetof(CsrBtAvrcpCtPasSetCfm, resultCode),
    offsetof(CsrBtAvrcpCtPassThroughCfm, resultCode),
    offsetof(CsrBtAvrcpCtGetFolderItemsCfm, resultCode),
    offsetof(CsrBtAvrcpCtPlayCfm, resultCode),
    offsetof(CsrBtAvrcpCtSearchCfm, resultCode),
    offsetof(CsrBtAvrcpCtGetAttributesCfm, resultCode),
    offsetof(CsrBtAvrcpCtChangePathCfm, resultCode),
    offsetof(CsrBtAvrcpCtSetVolumeCfm, resultCode),
    offsetof(CsrBtAvrcpCtSetAddressedPlayerCfm, resultCode),
    offsetof(CsrBtAvrcpCtSetBrowsedPlayerCfm, resultCode),
    offsetof(CsrBtAvrcpCtAddToNowPlayingCfm, resultCode),
    offsetof(CsrBtAvrcpCtInformBatteryStatusCfm, resultCode),
    offsetof(CsrBtAvrcpCtUnitInfoCmdCfm, resultCode),
    offsetof(CsrBtAvrcpCtSubUnitInfoCmdCfm, resultCode),
    offsetof(CsrBtAvrcpCtGetTotalNumberOfItemsCfm, resultCode)
};

static const CsrUint8 avrcpCtUtilResultSupplierOffsets[CSR_BT_AVRCP_CT_PRIM_CFM_HIGHEST - CSR_BT_AVRCP_CT_PRIM_UPSTREAM_LOWEST + 1] =
{/* Order MUST correspond to the numbering of the primitives */
    offsetof(CsrBtAvrcpCtInformDispCharsetCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtNotiRegisterCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtGetPlayStatusCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasAttIdCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasValIdCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasAttTxtCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasValTxtCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasCurrentCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPasSetCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPassThroughCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtGetFolderItemsCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtPlayCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtSearchCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtGetAttributesCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtChangePathCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtSetVolumeCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtSetAddressedPlayerCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtSetBrowsedPlayerCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtAddToNowPlayingCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtInformBatteryStatusCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtUnitInfoCmdCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtSubUnitInfoCmdCfm, resultSupplier),
    offsetof(CsrBtAvrcpCtGetTotalNumberOfItemsCfm, resultSupplier)
};

void CsrBtAvrcpCtPendingMsgUpdateResult(AvrcpCtPendingMsgInfo_t *pendingMsg,
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{/* Use a look-up table instead of a huge switch to reduce code size */
    CsrBtAvrcpPrim primType = *(CsrBtAvrcpPrim *)pendingMsg->cfmMsg;

    if ((primType >= CSR_BT_AVRCP_CT_PRIM_UPSTREAM_LOWEST) && (primType <= CSR_BT_AVRCP_CT_PRIM_CFM_HIGHEST))
    {
        *((CsrBtResultCode *)((CsrUint8 *)pendingMsg->cfmMsg +
            avrcpCtUtilResultCodeOffsets[primType - CSR_BT_AVRCP_CT_PRIM_UPSTREAM_LOWEST])) = resultCode;
        *((CsrBtSupplier *)((CsrUint8 *)pendingMsg->cfmMsg +
            avrcpCtUtilResultSupplierOffsets[primType - CSR_BT_AVRCP_CT_PRIM_UPSTREAM_LOWEST])) = resultSupplier;
    }
}

void CsrBtAvrcpCtUtilPendingMsgCompleteFromPsm(AvrcpConnInstance_t *connInst,
    CsrUint16 psm, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier)
{
    AvrcpCtPendingMsgInfo_t *pendingMsg;

    for (pendingMsg = AVRCP_LIST_CT_PMSG_GET_FIRST((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList); pendingMsg; )
    {
        AvrcpCtPendingMsgInfo_t *tmpPendingMsg = pendingMsg->next;

        if (pendingMsg->psm == psm)
        {
            CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsg, resultCode,
                resultSupplier);
            CsrBtAvrcpCtCompletePendingMsg(pendingMsg);
        }
        pendingMsg = tmpPendingMsg;
    }
}

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
CsrBool CsrBtAvrcpCtMatchNotificationLabel(AvrcpConnInstance_t *connInst,
                                           CsrUint8 notiId,
                                           CsrUint8 tLabel)
{
    if ((notiId >= CSR_BT_AVRCP_NOTI_ID_OFFSET &&
         notiId <= CSR_BT_AVRCP_NOTI_ID_MAXIMUM) &&
        (connInst->ctLocal->notiList[notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET] == tLabel))
    {
        return TRUE;
    }

    return FALSE;
}

void CsrBtAvrcpCtRegisterNextNotification(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo)
{
    CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = (CsrBtAvrcpCtNotiRegisterCfm *) pendingMsgInfo->cfmMsg;

    /* Notifications pending to be registered */
    while (pendingMsgInfo->tmpState < CSR_BT_AVRCP_NOTI_ID_MAXIMUM)
    {
        pendingMsgInfo->tmpState++; /* Skip to next notification ID */

        if (CSR_BIT_IS_SET((connInst->ctLocal->tgSupportedNotifications & connInst->ctLocal->ctRequestedNotifications), pendingMsgInfo->tmpState - CSR_BT_AVRCP_NOTI_ID_OFFSET))
        {/* Register for the next notification that is supported by both the target and controller */
            if (CsrBtAvrcpCtMatchNotificationLabel(connInst,
                                                   pendingMsgInfo->tmpState,
                                                   CSR_BT_AVRCP_TLABEL_INVALID))
            { /* Notification is not already registered */

                if (CsrBtAvrcpCtRegisterNotificationCmdSend(connInst,
                                                            pendingMsgInfo,
                                                            pendingMsgInfo->tmpState,
                                                            cfmMsg->playbackInterval))
                {
                    /* Next notification registration will get triggered when
                     * response of this notification registration comes */
                    connInst->pendingNotifReg = NULL;
                }
                else
                {
                    /* Transaction allocation failed.
                     * We have to retry later when some pending transaction finishes
                     * Step back on notification ID. */
                    connInst->pendingNotifReg = pendingMsgInfo;
                    pendingMsgInfo->tmpState--;
                }
                return; /* A command was sent */
            }
        }
    }

    /* All notification registration has been completed
     * Inform application */
    cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
    cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);

    connInst->pendingNotifReg = NULL;
}
#endif

CsrBool CsrBtAvrcpCtUtilGetAvailableTLabel(AvrcpConnInstance_t *connInst, CsrBtAvrcpConnDetails *conn)
{/* Assign an unique TLabel */
    CsrUintFast8 i;
    CsrUint8 tmpTLabel;
    CsrUint16 psm = CSR_BT_AVCTP_PSM;

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (&connInst->control != conn)
    {/* Control channel */
        psm = CSR_BT_AVCTP_BROWSING_PSM;
    }
#endif

    tmpTLabel = conn->ctTLabel;

    for (i = AVRCP_DATA_AVCTP_TLABEL_MIN; i <= AVRCP_DATA_AVCTP_TLABEL_MAX; i++)
    {
#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
        CsrBool notifTLabel = FALSE;
#endif

        /* Increment transaction label */
        AVRCP_TLABEL_ASSIGN(tmpTLabel);

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
        if (psm == CSR_BT_AVCTP_PSM && connInst->ctLocal->activeNotifications)
        {/* At least one notification is already registered */
            CsrUintFast8 j;

            /* Check against all registered notification transaction labels */
            for (j = 0; j < CSR_BT_AVRCP_NOTI_ID_COUNT; j++)
            {
                if (tmpTLabel == connInst->ctLocal->notiList[j])
                {/* TLabel is already used for a notification */
                    notifTLabel = TRUE;
                    break;
                }
            }
        }

        if(!notifTLabel)
#endif
        {
            /* Check if any transaction is pending with same transaction label on same channel */
            if(CsrBtAvrcpCtUtilMsgGetTLabel((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList, tmpTLabel, psm) == NULL)
            {
                /* We can use this transaction label */
                conn->ctTLabel = tmpTLabel;
                return (TRUE);
            }
        }
    }

    /* All transaction labels are allocated */
    return (FALSE);
}

void CsrBtAvrcpCtResetTransactionTimer(AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                       CsrTime time)
{
    CsrSchedTimerCancel(pendingMsgInfo->timerId, NULL, NULL);
    pendingMsgInfo->timerId = CsrSchedTimerSet(time + CSR_BT_AVRCP_CT_TIMER_AIR_OVERHEAD,
                                               CsrBtAvrcpCtPendingMsgTimeout,
                                               0,
                                               pendingMsgInfo);
}

CsrBool CsrBtAvrcpCtResetPendingMsg(CsrBtAvrcpConnDetails *conn, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrTime time)
{
    /* Reset transaction label in case 'pendingMsgInfo' was reused */
    pendingMsgInfo->tLabel = CSR_BT_AVRCP_TLABEL_INVALID;

    if (CsrBtAvrcpCtUtilGetAvailableTLabel(pendingMsgInfo->connInst, conn))
    {
        pendingMsgInfo->tLabel = conn->ctTLabel;
        CsrBtAvrcpCtResetTransactionTimer(pendingMsgInfo, time);
        return (TRUE);
    }
    else
    {
        /* Transaction label could not be allocated for this channel */
        return (FALSE);
    }
}

CsrBtResultCode CsrBtAvrcpCtUtilCheckRemoteRdy(AvrcpConnInstance_t *connInst, CsrUint16 avrcpVersion, CsrBool control)
{
    if (connInst)
    {
        if (connInst->ctLocal->tgSdpAvrcpVersion >= avrcpVersion)
        {
            if (connInst->control.state == AVRCP_STATE_CONN_CONNECTED)
            {
                if (control)
                {
                    return CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
                }
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                else
                {
                    if (connInst->browsing.state == AVRCP_STATE_CONN_CONNECTED)
                    {
                        return CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
                    }
                    else if (CSR_MASK_IS_SET(connInst->remoteFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING))
                    {/* Browsing is supported but not yet connected - start connecting */
                        CsrBtAvrcpUtilConnect(connInst);
                        return CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
                    }
                    else
                    {/* Local or remote does not support browsing */
                        return CSR_BT_RESULT_CODE_AVRCP_INVALID_VERSION;
                    }
                }
#else
                else
                {/* Local or remote does not support browsing */
                    return CSR_BT_RESULT_CODE_AVRCP_INVALID_VERSION;
                }
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
            }
            else
            {
                return CSR_BT_RESULT_CODE_AVRCP_CHANNEL_NOT_CONNECTED;
            }
        }
        else
        {
            return CSR_BT_RESULT_CODE_AVRCP_INVALID_VERSION;
        }
    }

    return CSR_BT_RESULT_CODE_AVRCP_DEVICE_NOT_CONNECTED;
}

AvrcpCtPendingMsgInfo_t *CsrBtAvrcpCtUtilMsgGetTLabel(CsrCmnList_t *pendingMsgList, CsrUint8 tl, CsrUint16 psm)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo;
    CsrBtAvrcpChannelTLabel channelTLabel;

    channelTLabel.tLabel = tl;
    channelTLabel.psm = psm;

    pendingMsgInfo = (AvrcpCtPendingMsgInfo_t *) CsrCmnListSearch(pendingMsgList,
                                                                  csrBtAvrcpCtMsgFindChannelTLabel,
                                                                  &channelTLabel);
    return (pendingMsgInfo);
}

AvrcpCtPendingMsgInfo_t *CsrBtAvrcpCtUtilMsgQueueAdd(AvrcpConnInstance_t *connInst,
                                                     void *cfmMsg,
                                                     CsrSchedQid phandle,
                                                     CsrUint16 psm)
{
    if (connInst)
    {
        AvrcpCtPendingMsgInfo_t *newPendingMsgInfo = AVRCP_LIST_CT_PMSG_ADD_FIRST((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList);

        newPendingMsgInfo->cfmMsg = cfmMsg;
        newPendingMsgInfo->tLabel = CSR_BT_AVRCP_TLABEL_INVALID;
        newPendingMsgInfo->tmpState = AVRCP_STATE_PM_IDLE;
        newPendingMsgInfo->timerId = 0;
        newPendingMsgInfo->connInst = connInst;
        newPendingMsgInfo->phandle = phandle;
        newPendingMsgInfo->psm = psm;
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
        newPendingMsgInfo->reqMsg = NULL;
        newPendingMsgInfo->appConnId = CSR_BT_AVRCP_INSTANCE_ID_INVALID;
        newPendingMsgInfo->uidCounter = 0xffff;
#endif
        return newPendingMsgInfo;
    }
    else
    {
        return NULL ;
    }
}

CsrBool CsrBtAvrcpCtUtilMsgQueueRemove(CsrCmnListElm_t *elem, void *data)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo = (AvrcpCtPendingMsgInfo_t *)elem;

    CSR_UNUSED(data);
    CsrSchedTimerCancel(pendingMsgInfo->timerId, NULL, NULL);

    if (pendingMsgInfo->cfmMsg)
    {
        /* Complete pending message */
        CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
            CSR_BT_RESULT_CODE_AVRCP_DEVICE_NOT_CONNECTED,
            CSR_BT_SUPPLIER_AVRCP);

        if (pendingMsgInfo->phandle != CSR_SCHED_QID_INVALID)
        {
            CsrBtAvrcpMessagePut(pendingMsgInfo->phandle, pendingMsgInfo->cfmMsg);
        }
        else
        {
            CsrPmemFree(pendingMsgInfo->cfmMsg);
        }
    }

    return (TRUE);
}

void CsrBtAvrcpCtUtilInitConnLocal(CsrBtAvrcpCtConnInfo **ctInfo)
{
    CsrBtAvrcpCtConnInfo *connInfo;

    connInfo = CsrPmemZalloc(sizeof(*connInfo));

    /* SDP */
    connInfo->tgSdpAvrcpVersion         = CSR_BT_AVRCP_CONFIG_SR_VERSION_10;
    connInfo->tgSdpSupportedFeatures    = 0;

    /* Notifications */
    connInfo->activeNotifications       = 0;
    connInfo->ctRequestedNotifications  = 0;
    connInfo->notiConfig                = CSR_BT_AVRCP_NOTI_REG_STANDARD;
    connInfo->playbackInterval          = 0;
    connInfo->tgSupportedNotifications  = 0;
    CsrMemSet(connInfo->notiList, CSR_BT_AVRCP_TLABEL_INVALID, CSR_BT_AVRCP_NOTI_ID_COUNT);

    /* Cached notification parameters */
    connInfo->notiParams.attValPair = NULL;

    /* Misc */
    connInfo->pendingMsgInfo            = NULL;
    CsrMemSet((void *)&connInfo->pendingMsgList, 0, sizeof(connInfo->pendingMsgList));

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
    connInfo->obexPsm                   = L2CA_PSM_INVALID;
    connInfo->ctObexState               = AVRCP_CT_OBEX_DISCONNECTED;
#endif

    *ctInfo = connInfo;
}

CsrUint16 CsrBtAvrcpCtSBPFolderNamesGet(CsrUint8 **folderPathName, CsrUint8 *inputData, CsrUint8 depth)
{
    CsrUint16 folderNamesLen = 1; /* Space for the first '/' and NULL terminator */
    CsrUint8 *tmpData = inputData;
    CsrUintFast8 i;
    CsrUint16 tmp, idx;

    for (i=0; i < depth; i++)
    {/* First two bytes contain the length of the first directory name */
        tmp = (CsrUint16)(CSR_GET_UINT16_FROM_BIG_ENDIAN(tmpData));

        folderNamesLen += (tmp + 1); /* Accumulative data length including place for separator */
        /* Move pointer to the next folder name length */
        tmpData += (tmp + sizeof(CsrUint16));
    }

    *folderPathName = CsrPmemAlloc(folderNamesLen);

    tmpData = inputData;
    idx = 0;
    (*folderPathName)[idx] = '/';
    idx++;

    for (i=0; i < depth; i++)
    {
        tmp = (CsrUint16)(CSR_GET_UINT16_FROM_BIG_ENDIAN(tmpData));
        tmpData += sizeof(CsrUint16);
        SynMemCpyS(&(*folderPathName)[idx],folderNamesLen-idx,tmpData,tmp);
        tmpData += tmp;
        idx += tmp;
        (*folderPathName)[idx] = '/';
        idx++;
    }
    (*folderPathName)[folderNamesLen-1] = 0;
    return folderNamesLen;
}

#endif /* #ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE */

#endif

