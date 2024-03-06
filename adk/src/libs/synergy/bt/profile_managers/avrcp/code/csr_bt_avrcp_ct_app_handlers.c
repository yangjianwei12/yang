/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE

#include "csr_types.h"
#include "csr_bt_result.h"
#include "csr_bt_util.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif

void CsrBtAvrcpCtPassThroughReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPassThroughReq *reqMsg = (CsrBtAvrcpCtPassThroughReq *) instData->recvMsgP;
    CsrBtAvrcpCtPassThroughCfm *cfmMsg = CsrBtAvrcpCtPassThroughCfmBuild(reqMsg->connectionId,
                                                                         reqMsg->operationId,
                                                                         reqMsg->state);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_10,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
        CsrUint8 state;

        if (reqMsg->state == CSR_BT_AVRCP_PT_STATE_PRESS_RELEASE)
        {
            state = CSR_BT_AVRCP_PT_STATE_PRESS;
            pendingMsgInfo->tmpState = AVRCP_STATE_PM_PT_RELEASE_PENDING;
        }
        else
        {
            state = reqMsg->state;
        }

        if ((reqMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_NEXT) ||
            (reqMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_PREV))
        {
            if (!CsrBtAvrcpCtGroupNavigationCmdSend(connInst,
                                                    pendingMsgInfo,
                                                    (CsrUint16) (reqMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_NEXT ?
                                                                    AVRCP_DATA_PT_GN_OPERATION_NEXT :
                                                                    AVRCP_DATA_PT_GN_OPERATION_PREV),
                                                    state))
            {
                /* Transaction label not available */
                cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
                cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
            }
        }
        else
#else
        if (reqMsg->state == CSR_BT_AVRCP_PT_STATE_PRESS_RELEASE)
        {
            pendingMsgInfo->tmpState = AVRCP_STATE_PM_PT_RELEASE_PENDING;
        }
#endif
        {
            if (!CsrBtAvrcpCtPassThroughCmdSend(connInst,
                                                pendingMsgInfo,
                                                reqMsg->operationId,
                                                reqMsg->state))
            {
                /* Transaction label not available */
                cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
                cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
            }
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

CsrBtAvrcpCtPassThroughCfm *CsrBtAvrcpCtPassThroughCfmBuild(CsrUint8 appConnId, CsrUint8 opId, CsrUint8 state)
{
    CsrBtAvrcpCtPassThroughCfm *cfmMsg = (CsrBtAvrcpCtPassThroughCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PASS_THROUGH_CFM;
    cfmMsg->connectionId    = appConnId;
    cfmMsg->operationId     = opId;
    cfmMsg->state           = state;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
void CsrBtAvrcpCtUnitInfoCmdReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtUnitInfoCmdReq *reqMsg = (CsrBtAvrcpCtUnitInfoCmdReq *) instData->recvMsgP;
    CsrBtAvrcpCtUnitInfoCmdCfm *cfmMsg = CsrBtAvrcpCtUnitInfoCmdCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_10,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {/* Build and send UNIT INFO message */
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);

        if (!CsrBtAvrcpCtUnitInfoCmdSend(connInst,
                                         pendingMsgInfo,
                                         reqMsg->pDataLen,
                                         reqMsg->pData))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->pData);
}

void CsrBtAvrcpCtSubUnitInfoCmdReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtSubUnitInfoCmdReq *reqMsg = (CsrBtAvrcpCtSubUnitInfoCmdReq *) instData->recvMsgP;
    CsrBtAvrcpCtSubUnitInfoCmdCfm *cfmMsg = CsrBtAvrcpCtSubUnitInfoCmdCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_10,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {/* Build and send SUB UNIT INFO message */
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtSubUnitInfoCmdSend(connInst,
                                            pendingMsgInfo,
                                            reqMsg->pDataLen,
                                            reqMsg->pData))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->pData);
}

CsrBtAvrcpCtUnitInfoCmdCfm *CsrBtAvrcpCtUnitInfoCmdCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtUnitInfoCmdCfm *cfmMsg = (CsrBtAvrcpCtUnitInfoCmdCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_UNIT_INFO_CMD_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->pDataLen        = 0;
    cfmMsg->pData           = NULL;

    return cfmMsg;
}

CsrBtAvrcpCtSubUnitInfoCmdCfm *CsrBtAvrcpCtSubUnitInfoCmdCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtSubUnitInfoCmdCfm *cfmMsg = (CsrBtAvrcpCtSubUnitInfoCmdCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_SUB_UNIT_INFO_CMD_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->pDataLen        = 0;
    cfmMsg->pData           = NULL;

    return cfmMsg;
}
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
void CsrBtAvrcpCtNotiRegisterReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtNotiRegisterReq *reqMsg = (CsrBtAvrcpCtNotiRegisterReq *) instData->recvMsgP;
    CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = CsrBtAvrcpCtNotiRegisterCfmBuild(reqMsg->connectionId,
                                                                           reqMsg->playbackInterval,
                                                                           reqMsg->notiMask);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        connInst->ctLocal->ctRequestedNotifications = reqMsg->notiMask;
        connInst->ctLocal->playbackInterval = reqMsg->playbackInterval;
        connInst->ctLocal->notiConfig = reqMsg->config;
        /* Start by determining which notifications the remote device supports */
        /* Try sending command */
        if (!CsrBtAvrcpCtGetCapabilitiesCmdSend(connInst,
                                                pendingMsgInfo,
                                                AVRCP_DATA_PDU_GET_CAP_CMN_NOTI_SUP))
        {
            /* Transaction label not available */
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtSetVolumeReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtSetVolumeReq *reqMsg = (CsrBtAvrcpCtSetVolumeReq *) instData->recvMsgP;
    CsrBtAvrcpCtSetVolumeCfm *cfmMsg = CsrBtAvrcpCtSetVolumeCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_AVRCP_SUCCESS;

/* Do not check remote supported feature for Source Dongle application. 
 * Below condition is to fix an IOP issue where few commercial devices support the feature, 
 * but do not respond in SDP query.  
 */
#ifndef SYNERGY_VM_SOURCE_APP_ENABLED    
    resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                TRUE);
#endif /* !SYNERGY_VM_SOURCE_APP_ENABLED */

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtSetAbsoluteVolumeCmdSend(connInst,
                                                  pendingMsgInfo,
                                                  reqMsg->volume))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

/******************************************************************************
******************* Building of primitives for application ********************
******************************************************************************/
CsrBtAvrcpCtNotiRegisterCfm *CsrBtAvrcpCtNotiRegisterCfmBuild(CsrUint8 connId, CsrUint32 playbackInterval,CsrBtAvrcpNotiMask notiMask)
{
    CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = (CsrBtAvrcpCtNotiRegisterCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_NOTI_REGISTER_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->notiMask        = notiMask;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->playbackInterval = playbackInterval;

    return cfmMsg;
}

CsrBtAvrcpCtSetVolumeCfm *CsrBtAvrcpCtSetVolumeCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtSetVolumeCfm *cfmMsg = (CsrBtAvrcpCtSetVolumeCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_SET_VOLUME_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->volume          = 0;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

/*********************** Notification send functions *************************/
static void csrBtAvrcpNotiPlaybackStatusIndSend(CsrSchedQid ctrlHandle,
                                                CsrUint8 appConnId,
                                                CsrBtAvrcpPlaybackStatus playbackStatus)
{
    CsrBtAvrcpCtNotiPlaybackStatusInd *indMsg = (CsrBtAvrcpCtNotiPlaybackStatusInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_PLAYBACK_STATUS_IND;
    indMsg->connectionId = appConnId;
    indMsg->playbackStatus = playbackStatus;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

static void csrBtAvrcpCtNotiVolumeIndSend(CsrSchedQid ctrlHandle,
                                          CsrUint8 appConnId,
                                          CsrUint8 volume)
{
    CsrBtAvrcpCtNotiVolumeInd *indMsg = (CsrBtAvrcpCtNotiVolumeInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_VOLUME_IND;
    indMsg->connectionId = appConnId;
    indMsg->volume = volume;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
void CsrBtAvrcpCtGetAttributesIndSend(CsrSchedQid phandle, CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid, CsrUint8 attribCount, CsrUint16 attribDataLen, CsrUint8 *attribData, CsrUint16 offset)
{
    CsrBtAvrcpCtGetAttributesInd *indMsg = (CsrBtAvrcpCtGetAttributesInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type            = CSR_BT_AVRCP_CT_GET_ATTRIBUTES_IND;
    indMsg->connectionId    = connId;
    indMsg->scope           = scope;
    indMsg->attributeCount  = attribCount;
    indMsg->attribDataLen   = attribDataLen;
    indMsg->attribData      = attribData;
    CSR_BT_AVRCP_UID_COPY(indMsg->uid, *uid);
    indMsg->attribDataPayloadOffset = offset;
    CsrBtAvrcpMessagePut(phandle, indMsg);
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
void CsrBtAvrcpCtInformDispCharsetReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtInformDispCharsetReq *reqMsg = (CsrBtAvrcpCtInformDispCharsetReq *) instData->recvMsgP;
    CsrBtAvrcpCtInformDispCharsetCfm *cfmMsg = CsrBtAvrcpCtInformDispCharsetCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (reqMsg->charset == NULL)
    {
        reqMsg->charset = CsrPmemAlloc(sizeof(CsrBtAvrcpCharSet));
        *reqMsg->charset = CSR_BT_AVRCP_CHARACTER_SET_UTF_8;
    }

    if (reqMsg->charsetCount == 0)
    {
        reqMsg->charsetCount = 1;
    }

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);

        /* Try sending command */
        if (!CsrBtAvrcpCtInformDispCharsetCmdSend(connInst,
                                                  pendingMsgInfo,
                                                  reqMsg->charsetCount,
                                                  reqMsg->charset))
        {
            /* Transaction label not available */
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }

    CsrPmemFree(reqMsg->charset);
}

void CsrBtAvrcpCtPasAttIdReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasAttIdReq *reqMsg = (CsrBtAvrcpCtPasAttIdReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasAttIdCfm *cfmMsg = CsrBtAvrcpCtPasAttIdCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasListAttCmdSend(connInst, pendingMsgInfo))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtPasValIdReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasValIdReq *reqMsg = (CsrBtAvrcpCtPasValIdReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasValIdCfm *cfmMsg = CsrBtAvrcpCtPasValIdCfmBuild(reqMsg->connectionId,
                                                                   reqMsg->attId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasListValCmdSend(connInst,
                                           pendingMsgInfo,
                                           reqMsg->attId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtPasAttTxtReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasAttTxtReq *reqMsg = (CsrBtAvrcpCtPasAttTxtReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasAttTxtCfm *cfmMsg = CsrBtAvrcpCtPasAttTxtCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasGetAttTextCmdSend(connInst,
                                              pendingMsgInfo,
                                              reqMsg->attIdCount,
                                              reqMsg->attId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->attId);
}

void CsrBtAvrcpCtPasAttTxtResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasAttTxtRes *reqMsg = (CsrBtAvrcpCtPasAttTxtRes *)instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst, CSR_BT_AVRCP_CONFIG_SR_VERSION_13, TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpCtRequestAbortContinuationCmdSend(connInst, connInst->ctLocal->pendingMsgInfo, reqMsg->proceed, AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT);
    }
    else
    {/* Remote device have been disconnected - confirmation already sent */
    }
}

void CsrBtAvrcpCtPasValTxtReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasValTxtReq *reqMsg = (CsrBtAvrcpCtPasValTxtReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasValTxtCfm *cfmMsg = CsrBtAvrcpCtPasValTxtCfmBuild(reqMsg->connectionId,
                                                                     reqMsg->attId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasGetValTextCmdSend(connInst,
                                              pendingMsgInfo,
                                              reqMsg->attId,
                                              reqMsg->valIdCount,
                                              reqMsg->valId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->valId);
}

void CsrBtAvrcpCtPasValTxtResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasValTxtRes *reqMsg = (CsrBtAvrcpCtPasValTxtRes *)instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst, CSR_BT_AVRCP_CONFIG_SR_VERSION_13, TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpCtRequestAbortContinuationCmdSend(connInst, connInst->ctLocal->pendingMsgInfo, reqMsg->proceed, AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT);
    }
    else
    {/* Remote device have been disconnected - confirmation already sent */
    }
}

void CsrBtAvrcpCtPasCurrentReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasCurrentReq *reqMsg = (CsrBtAvrcpCtPasCurrentReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasCurrentCfm *cfmMsg = CsrBtAvrcpCtPasCurrentCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasGetCurrentValCmdSend(connInst,
                                                 pendingMsgInfo,
                                                 reqMsg->attIdCount,
                                                 reqMsg->attId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->attId);
}

void CsrBtAvrcpCtPasSetReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPasSetReq *reqMsg = (CsrBtAvrcpCtPasSetReq *) instData->recvMsgP;
    CsrBtAvrcpCtPasSetCfm *cfmMsg = CsrBtAvrcpCtPasSetCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtPasSetValCmdSend(connInst,
                                          pendingMsgInfo,
                                          reqMsg->attValPairCount,
                                          reqMsg->attValPair))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->attValPair);
}

void CsrBtAvrcpCtInformBatteryStatusReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtInformBatteryStatusReq *reqMsg = (CsrBtAvrcpCtInformBatteryStatusReq *) instData->recvMsgP;
    CsrBtAvrcpCtInformBatteryStatusCfm *cfmMsg = CsrBtAvrcpCtInformBatteryStatusCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtInformBatterySend(connInst,
                                           pendingMsgInfo,
                                           reqMsg->batStatus))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

CsrBtAvrcpCtInformBatteryStatusCfm *CsrBtAvrcpCtInformBatteryStatusCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtInformBatteryStatusCfm *cfmMsg = (CsrBtAvrcpCtInformBatteryStatusCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_INFORM_BATTERY_STATUS_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtInformDispCharsetCfm *CsrBtAvrcpCtInformDispCharsetCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtInformDispCharsetCfm *cfmMsg = (CsrBtAvrcpCtInformDispCharsetCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_INFORM_DISP_CHARSET_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtPasAttIdCfm *CsrBtAvrcpCtPasAttIdCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtPasAttIdCfm *cfmMsg = (CsrBtAvrcpCtPasAttIdCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_ATT_ID_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->attIdCount   = 0;
    cfmMsg->attId        = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    return cfmMsg;
}

CsrBtAvrcpCtPasValIdCfm *CsrBtAvrcpCtPasValIdCfmBuild(CsrUint8 connId, CsrBtAvrcpPasAttId attId)
{
    CsrBtAvrcpCtPasValIdCfm *cfmMsg = (CsrBtAvrcpCtPasValIdCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_VAL_ID_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->attId        = attId;
    cfmMsg->valIdCount      = 0;
    cfmMsg->valId           = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtPasAttTxtCfm *CsrBtAvrcpCtPasAttTxtCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtPasAttTxtCfm *cfmMsg = (CsrBtAvrcpCtPasAttTxtCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_ATT_TXT_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->pasDataLen      = 0;
    cfmMsg->pasData         = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtPasValTxtCfm *CsrBtAvrcpCtPasValTxtCfmBuild(CsrUint8 connId, CsrBtAvrcpPasAttId attId)
{
    CsrBtAvrcpCtPasValTxtCfm *cfmMsg = (CsrBtAvrcpCtPasValTxtCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_VAL_TXT_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->attId           = attId;
    cfmMsg->pasDataLen      = 0;
    cfmMsg->pasData         = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtPasCurrentCfm *CsrBtAvrcpCtPasCurrentCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtPasCurrentCfm *cfmMsg = (CsrBtAvrcpCtPasCurrentCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_CURRENT_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->attValPairCount        = 0;
    cfmMsg->attValPair             = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtPasSetCfm *CsrBtAvrcpCtPasSetCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtPasSetCfm *cfmMsg = (CsrBtAvrcpCtPasSetCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PAS_SET_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

void CsrBtAvrcpCtPasValTxtIndSend(CsrUint8 connId, CsrBtAvrcpPasAttId attId, CsrUint16 pasDataLen, CsrUint8 *pasData, CsrSchedQid phandle)
{
    CsrBtAvrcpCtPasValTxtInd *indMsg = (CsrBtAvrcpCtPasValTxtInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type            = CSR_BT_AVRCP_CT_PAS_VAL_TXT_IND;
    indMsg->connectionId    = connId;
    indMsg->attId        = attId;
    indMsg->pasDataLen      = pasDataLen;
    indMsg->pasData         = pasData;
    CsrBtAvrcpMessagePut(phandle, indMsg);
}

void CsrBtAvrcpCtPasAttTxtIndSend(CsrUint8 connId, CsrUint16 pasDataLen, CsrUint8 *pasData, CsrSchedQid phandle)
{
    CsrBtAvrcpCtPasAttTxtInd *indMsg = (CsrBtAvrcpCtPasAttTxtInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type            = CSR_BT_AVRCP_CT_PAS_ATT_TXT_IND;
    indMsg->connectionId    = connId;
    indMsg->pasDataLen      = pasDataLen;
    indMsg->pasData         = pasData;
    CsrBtAvrcpMessagePut(phandle, indMsg);
}

static void csrBtAvrcpCtPasSetIndSend(CsrSchedQid ctrlHandle,
                                      CsrUint8 appConnId,
                                      CsrBtAvrcpPasAttValPair* attValPair,
                                      CsrUint8 attValPairCount)
{
    CsrBtAvrcpCtPasSetInd *indMsg = (CsrBtAvrcpCtPasSetInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_PAS_SET_IND;
    indMsg->connectionId = appConnId;
    indMsg->attValPairCount = attValPairCount;
    indMsg->attValPair = attValPair;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

/* Compares list of attribute-value pairs */
static CsrBool csrBtAvrcpCtCompareAttribValPair(CsrBtAvrcpPasAttValPair *oldAttValPair,
                                                CsrBtAvrcpPasAttValPair *newAttValPair,
                                                CsrUint8 count)
{
    CsrUintFast8 i;

    for (i = 0; i < count; i++)
    {
        CsrUintFast8 j;
        CsrBool found = FALSE;

        for (j = 0; j < count; j++)
        {
            if (newAttValPair[i].attribId == oldAttValPair[j].attribId)
            {
                if (newAttValPair[i].valueId == oldAttValPair[j].valueId)
                {
                    found = TRUE;
                }
                break;
            }
        }

        if (!found)
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
void CsrBtAvrcpCtInitiateImagingConnection(AvrcpInstanceData_t *instData, AvrcpConnInstance_t *connInst)
{
    if (connInst->ctLocal->ctObexState == AVRCP_CT_OBEX_DISCONNECTED || 
        connInst->ctLocal->ctObexState == AVRCP_CT_OBEX_TRANSPORT_CONNECTED)
    {
        connInst->ctLocal->ctObexState = AVRCP_CT_OBEX_CONNECTING_GET;
        /* Establish an obex connection */
        CsrBtAvrcpImagingClientConnectReqSend(connInst->appConnId, 
            connInst->address, connInst->ctLocal->obexPsm, 
            instData->secOutgoingCont);
    }
    else
    {
        /* We could just wait for the connection to complete */
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_CT_COVER_ART */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpCtGetFolderItemsReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtGetFolderItemsReq *reqMsg = (CsrBtAvrcpCtGetFolderItemsReq *) instData->recvMsgP;
    CsrBtAvrcpCtGetFolderItemsCfm *cfmMsg = CsrBtAvrcpCtGetFolderItemsCfmBuild(reqMsg->connectionId,
                                                                               reqMsg->scope,
                                                                               reqMsg->startItem,
                                                                               reqMsg->endItem);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                FALSE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_BROWSING_PSM);

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
        if ((connInst->ctLocal->obexPsm != L2CA_PSM_INVALID) && 
            (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_SERVICE_CONNECTED))
        {
            pendingMsgInfo->reqMsg = reqMsg;
            instData->recvMsgP = NULL;
            pendingMsgInfo->appConnId = connInst->appConnId;

            CsrBtAvrcpCtInitiateImagingConnection(instData, connInst);
        }
        else
#endif
        if (!CsrBtAvrcpCtGetFolderItemsCmdSend(connInst,
                                               pendingMsgInfo,
                                               reqMsg->scope,
                                               reqMsg->startItem,
                                               reqMsg->endItem,
                                               reqMsg->attributeMask))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtSearchReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtSearchReq *reqMsg = (CsrBtAvrcpCtSearchReq *) instData->recvMsgP;
    CsrBtAvrcpCtSearchCfm *cfmMsg = CsrBtAvrcpCtSearchCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                FALSE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_BROWSING_PSM);
        if (!CsrBtAvrcpCtSearchCmdSend(connInst,
                                       pendingMsgInfo,
                                       reqMsg->charsetId,
                                       reqMsg->text))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
    CsrPmemFree(reqMsg->text);
}

void CsrBtAvrcpCtChangePathReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtChangePathReq *reqMsg = (CsrBtAvrcpCtChangePathReq *) instData->recvMsgP;
    CsrBtAvrcpCtChangePathCfm *cfmMsg = CsrBtAvrcpCtChangePathCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                FALSE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpUid uid;
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_BROWSING_PSM);

        if (reqMsg->folderDir == CSR_BT_AVRCP_CHANGE_PATH_UP)
        {/* According to 1.4 spec, if folder direction is "UP" then the folderUid is reserved; that is all zeros */
            CsrMemSet(uid, 0, sizeof(CsrBtAvrcpUid));
        }
        else
        {
            CSR_BT_AVRCP_UID_COPY(uid, reqMsg->folderUid);
        }

        if (!CsrBtAvrcpCtChangePathCmdSend(connInst,
                                           pendingMsgInfo,
                                           reqMsg->uidCounter,
                                           reqMsg->folderDir,
                                           &reqMsg->folderUid))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
        /* For the Media player which is database unware player the OBEX 
            service connection has to be disconnected. But let that be done
            only if change path was successful. */
        pendingMsgInfo->uidCounter = reqMsg->uidCounter;
#endif
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtSetBrowsedPlayerReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtSetBrowsedPlayerReq *reqMsg = (CsrBtAvrcpCtSetBrowsedPlayerReq *) instData->recvMsgP;
    CsrBtAvrcpCtSetBrowsedPlayerCfm *cfmMsg = CsrBtAvrcpCtSetBrowsedPlayerCfmBuild(reqMsg->connectionId,
                                                                                   reqMsg->playerId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                FALSE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_BROWSING_PSM);
        if (!CsrBtAvrcpCtSetBrowsedPlayerCmdSend(connInst,
                                                 pendingMsgInfo,
                                                 (CsrUint16) reqMsg->playerId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtGetTotalNumberOfItemsReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtGetTotalNumberOfItemsReq *reqMsg = (CsrBtAvrcpCtGetTotalNumberOfItemsReq *) instData->recvMsgP;
    CsrBtAvrcpCtGetTotalNumberOfItemsCfm *cfmMsg = CsrBtAvrcpCtGetTotalNumberOfItemsCfmBuild(reqMsg->connectionId, reqMsg->scope);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_16,
                                                                FALSE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_BROWSING_PSM);
        /* GetTotalNumberOfItems PDU*/
        if (!CsrBtAvrcpCtGetTotalNumberOfItemsCmdSend(connInst,
                                           pendingMsgInfo,
                                           reqMsg->scope))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

CsrBtAvrcpCtGetFolderItemsCfm *CsrBtAvrcpCtGetFolderItemsCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrUint32 startItem, CsrUint32 endItem)
{
    CsrBtAvrcpCtGetFolderItemsCfm *cfmMsg = (CsrBtAvrcpCtGetFolderItemsCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_GET_FOLDER_ITEMS_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->scope           = scope;
    cfmMsg->uidCounter      = 0;
    cfmMsg->startItem       = startItem;
    cfmMsg->endItem         = endItem;
    cfmMsg->itemsCount      = 0;
    cfmMsg->itemsDataLen    = 0;
    cfmMsg->itemsData       = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtSearchCfm *CsrBtAvrcpCtSearchCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtSearchCfm *cfmMsg = (CsrBtAvrcpCtSearchCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_SEARCH_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->numberOfItems   = 0;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtChangePathCfm *CsrBtAvrcpCtChangePathCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtChangePathCfm *cfmMsg = (CsrBtAvrcpCtChangePathCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_CHANGE_PATH_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtSetBrowsedPlayerCfm *CsrBtAvrcpCtSetBrowsedPlayerCfmBuild(CsrUint8 connId, CsrUint32 playerId)
{
    CsrBtAvrcpCtSetBrowsedPlayerCfm *cfmMsg = (CsrBtAvrcpCtSetBrowsedPlayerCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_SET_BROWSED_PLAYER_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->playerId        = playerId;
    cfmMsg->uidCounter      = 0;
    cfmMsg->itemsCount      = 0;
    cfmMsg->folderDepth     = 0;
    cfmMsg->folderNamesLen  = 0;
    cfmMsg->folderNames     = NULL;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->charsetId       = CSR_BT_AVRCP_CHARACTER_SET_UTF_8;

    return cfmMsg;
}

CsrBtAvrcpCtGetTotalNumberOfItemsCfm *CsrBtAvrcpCtGetTotalNumberOfItemsCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope)
{
    CsrBtAvrcpCtGetTotalNumberOfItemsCfm *cfmMsg = (CsrBtAvrcpCtGetTotalNumberOfItemsCfm *) CsrPmemZalloc(sizeof(*cfmMsg));
    cfmMsg->type            = CSR_BT_AVRCP_CT_GET_TOTAL_NUMBER_OF_ITEMS_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->scope           = scope;
    cfmMsg->uidCounter      = 0;
    cfmMsg->noOfItems       = 0;

    return cfmMsg;
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
void CsrBtAvrcpCtGetPlayStatusReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtGetPlayStatusReq *reqMsg = (CsrBtAvrcpCtGetPlayStatusReq *) instData->recvMsgP;
    CsrBtAvrcpCtGetPlayStatusCfm *cfmMsg = CsrBtAvrcpCtGetPlayStatusCfmBuild(reqMsg->connectionId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);

    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtGetPlayStatusCmdSend(connInst, pendingMsgInfo))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

CsrBtAvrcpCtGetPlayStatusCfm *CsrBtAvrcpCtGetPlayStatusCfmBuild(CsrUint8 connId)
{
    CsrBtAvrcpCtGetPlayStatusCfm *cfmMsg = (CsrBtAvrcpCtGetPlayStatusCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_GET_PLAY_STATUS_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->songLength      = 0;
    cfmMsg->songPosition    = 0;
    cfmMsg->playStatus      = CSR_BT_AVRCP_PLAYBACK_STATUS_ERROR;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

static void csrBtAvrcpCtNotiTrackChangeIndSend(CsrSchedQid ctrlHandle,
                                               CsrUint8 appConnId,
                                               CsrBtAvrcpUid *trackUid)
{
    CsrBtAvrcpCtNotiTrackChangedInd *indMsg = (CsrBtAvrcpCtNotiTrackChangedInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_TRACK_CHANGED_IND;
    indMsg->connectionId = appConnId;
    SynMemCpyS(&indMsg->trackUid,
              sizeof(indMsg->trackUid),
              trackUid,
              sizeof(indMsg->trackUid));
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_NOTIFICATIONS
static void csrBtAvrcpCtNotiTrackEndIndSend(CsrSchedQid ctrlHandle,
                                            CsrUint8 appConnId)
{
    CsrBtAvrcpCtNotiTrackEndInd *indMsg = (CsrBtAvrcpCtNotiTrackEndInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_TRACK_END_IND;
    indMsg->connectionId = appConnId;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

static void csrBtAvrcpCtNotiTrackStartIndSend(CsrSchedQid ctrlHandle,
                                              CsrUint8 appConnId)
{
    CsrBtAvrcpCtNotiTrackStartInd *indMsg = (CsrBtAvrcpCtNotiTrackStartInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_TRACK_START_IND;
    indMsg->connectionId = appConnId;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

static void csrBtAvrcpCtNotiPlaybackPosIndSend(CsrSchedQid ctrlHandle,
                                               CsrUint8 appConnId,
                                               CsrUint32 playbackPos)
{
    CsrBtAvrcpCtNotiPlaybackPosInd *indMsg = (CsrBtAvrcpCtNotiPlaybackPosInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_PLAYBACK_POS_IND;
    indMsg->connectionId = appConnId;
    indMsg->playbackPos = playbackPos;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

static void csrBtAvrcpCtNotiBatteryStatusIndSend(CsrSchedQid ctrlHandle,
                                                 CsrUint8 appConnId,
                                                 CsrBtAvrcpBatteryStatus batteryStatus)
{
    CsrBtAvrcpCtNotiBatteryStatusInd *indMsg = (CsrBtAvrcpCtNotiBatteryStatusInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_BATTERY_STATUS_IND;
    indMsg->connectionId = appConnId;
    indMsg->batteryStatus = batteryStatus;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

static void csrBtAvrcpCtNotiSystemStatusIndSend(CsrSchedQid ctrlHandle,
                                                CsrUint8 appConnId,
                                                CsrBtAvrcpSystemStatus systemStatus)
{
    CsrBtAvrcpCtNotiSystemStatusInd *indMsg = (CsrBtAvrcpCtNotiSystemStatusInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_SYSTEM_STATUS_IND;
    indMsg->connectionId = appConnId;
    indMsg->systemStatus = systemStatus;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}
#endif /* INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
void CsrBtAvrcpCtSetAddressedPlayerReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtSetAddressedPlayerReq *reqMsg = (CsrBtAvrcpCtSetAddressedPlayerReq *) instData->recvMsgP;
    CsrBtAvrcpCtSetAddressedPlayerCfm *cfmMsg = CsrBtAvrcpCtSetAddressedPlayerCfmBuild(reqMsg->connectionId,
                                                                                       reqMsg->playerId);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);
        if (!CsrBtAvrcpCtSetAddressedPlayerCmdSend(connInst,
                                                   pendingMsgInfo,
                                                   (CsrUint16) reqMsg->playerId))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

static void csrBtAvrcpCtSetAddressedPlayerIndSend(CsrSchedQid ctrlHandle,
                                                  CsrUint8 appConnId,
                                                  CsrUint32 playerId,
                                                  CsrUint16 uidCounter)
{
    CsrBtAvrcpCtSetAddressedPlayerInd *indMsg = (CsrBtAvrcpCtSetAddressedPlayerInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_SET_ADDRESSED_PLAYER_IND;
    indMsg->connectionId = appConnId;
    indMsg->playerId = playerId;
    indMsg->uidCounter = uidCounter;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}

CsrBtAvrcpCtSetAddressedPlayerCfm *CsrBtAvrcpCtSetAddressedPlayerCfmBuild(CsrUint8 connId, CsrUint32 playerId)
{
    CsrBtAvrcpCtSetAddressedPlayerCfm *cfmMsg = (CsrBtAvrcpCtSetAddressedPlayerCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_SET_ADDRESSED_PLAYER_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->playerId        = playerId;
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (INSTALL_AVRCP_METADATA_ATTRIBUTES)
void CsrBtAvrcpCtGetAttributesReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtGetAttributesReq *reqMsg = (CsrBtAvrcpCtGetAttributesReq *) instData->recvMsgP;
    CsrBtAvrcpCtGetAttributesCfm *cfmMsg = CsrBtAvrcpCtGetAttributesCfmBuild(reqMsg->connectionId,
                                                                             reqMsg->scope,
                                                                             reqMsg->uid);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_AVRCP_DEVICE_NOT_CONNECTED;
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrBtAvrcpUid zeroUid = {0,0,0,0,0,0,0,0};
#endif

    if (connInst)
    {
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        /* Determine if GetElementAttributes or GetItemAttributes should be used.
           UID value zero is not allowed for GetItemAttributes */
        if ((instData->ctLocal.srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)
            && (connInst->ctLocal->tgSdpAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)
            && CSR_MASK_IS_SET(connInst->ctLocal->tgSdpSupportedFeatures,
                               CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING) 
            && (CsrMemCmp(reqMsg->uid, &zeroUid, CSR_BT_AVRCP_UID_SIZE) != 0))
        {/* GetItemAttributes */
            resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                        CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                        FALSE);
            if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
            {
                AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                                      cfmMsg,
                                                                                      reqMsg->phandle,
                                                                                      CSR_BT_AVCTP_BROWSING_PSM);

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                if ((connInst->ctLocal->obexPsm != L2CA_PSM_INVALID) && 
                    (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_SERVICE_CONNECTED))
                {
                    pendingMsgInfo->reqMsg = reqMsg;
                    instData->recvMsgP = NULL;
                    pendingMsgInfo->appConnId = connInst->appConnId;

                    CsrBtAvrcpCtInitiateImagingConnection(instData, connInst);
                }
                else
#endif
                if (!CsrBtAvrcpCtGetItemAttributesCmdSend(connInst,
                                                          pendingMsgInfo,
                                                          reqMsg->scope,
                                                          reqMsg->uidCounter,
                                                          &reqMsg->uid,
                                                          reqMsg->attributeMask))
                {
                    cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
                    cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                }
            }
        }
        else
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
        {/* GetElementAttributes */
#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
            resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                        CSR_BT_AVRCP_CONFIG_SR_VERSION_13,
                                                        TRUE);
            if (resultCode == CSR_BT_RESULT_CODE_AVRCP_INVALID_VERSION)
            {
                if ((connInst->ctLocal->tgSdpAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                        && (CSR_MASK_IS_SET(connInst->ctLocal->tgSdpSupportedFeatures,
                                            CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING) == FALSE)
#endif
                   )
                {/* The remote device claims support for rev 1.4 of the AVRCP spec, but no support for the browsing feature:
                 Issue the GetElementAttributes instead of the GetItemAttributes */
                    resultCode = CSR_BT_RESULT_CODE_AVRCP_SUCCESS;
                }
            }

            if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
            {
                AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                                      cfmMsg,
                                                                                      reqMsg->phandle,
                                                                                      CSR_BT_AVCTP_PSM);
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                if ((connInst->ctLocal->obexPsm != L2CA_PSM_INVALID) && 
                    (connInst->ctLocal->ctObexState != AVRCP_CT_OBEX_SERVICE_CONNECTED))
                {
                    pendingMsgInfo->reqMsg = reqMsg;
                    instData->recvMsgP = NULL;
                    pendingMsgInfo->appConnId = connInst->appConnId;

                    CsrBtAvrcpCtInitiateImagingConnection(instData, connInst);
                }
                else
#endif
                if (!CsrBtAvrcpCtGetElementAttributesCmdSend(connInst,
                                                             pendingMsgInfo,
                                                             reqMsg->attributeMask))
                {
                    cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
                    cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                }
            }
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */
        }
    }

    if (resultCode != CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtGetAttributesResHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtGetAttributesRes *resMsg = (CsrBtAvrcpCtGetAttributesRes *)instData->recvMsgP;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList, resMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst, CSR_BT_AVRCP_CONFIG_SR_VERSION_13, TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpCtRequestAbortContinuationCmdSend(connInst, connInst->ctLocal->pendingMsgInfo, resMsg->proceed, AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES);
    }
    else
    {/* Remote device have been disconnected - confirmation already sent */
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || INSTALL_AVRCP_METADATA_ATTRIBUTES */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpCtPlayReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtPlayReq *reqMsg = (CsrBtAvrcpCtPlayReq *) instData->recvMsgP;
    CsrBtAvrcpCtPlayCfm *cfmMsg = CsrBtAvrcpCtPlayCfmBuild(reqMsg->connectionId,
                                                           reqMsg->scope,
                                                           &reqMsg->uid);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpUid uid;
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);

        CSR_BT_AVRCP_UID_COPY(uid, reqMsg->uid);
        if (!CsrBtAvrcpCtPlayItemCmdSend(connInst,
                                         pendingMsgInfo,
                                         reqMsg->scope,
                                         reqMsg->uidCounter,
                                         &uid))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

void CsrBtAvrcpCtAddToNowPlayingReqHandler(AvrcpInstanceData_t *instData)
{
    CsrBtAvrcpCtAddToNowPlayingReq *reqMsg = (CsrBtAvrcpCtAddToNowPlayingReq *) instData->recvMsgP;
    CsrBtAvrcpCtAddToNowPlayingCfm *cfmMsg = CsrBtAvrcpCtAddToNowPlayingCfmBuild(reqMsg->connectionId,
                                                                                 reqMsg->scope,
                                                                                 &reqMsg->uid);
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_AID((CsrCmnList_t *)&instData->connList,
                                                            reqMsg->connectionId);
    CsrBtResultCode resultCode = CsrBtAvrcpCtUtilCheckRemoteRdy(connInst,
                                                                CSR_BT_AVRCP_CONFIG_SR_VERSION_14,
                                                                TRUE);

    if (resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
    {
        CsrBtAvrcpUid uid;
        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                              cfmMsg,
                                                                              reqMsg->phandle,
                                                                              CSR_BT_AVCTP_PSM);

        CSR_BT_AVRCP_UID_COPY(uid, reqMsg->uid);
        if (!CsrBtAvrcpCtAddToNPLCmdSend(connInst,
                                         pendingMsgInfo,
                                         reqMsg->scope,
                                         reqMsg->uidCounter,
                                         &uid))
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_TLABELS_EXHAUSTED;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = resultCode;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpMessagePut(reqMsg->phandle, cfmMsg);
    }
}

static void csrBtAvrcpCtNotiNowPlayingIndSend(CsrSchedQid ctrlHandle,
                                              CsrUint8 appConnId)
{
    CsrBtAvrcpCtNotiNowPlayingInd *indMsg = (CsrBtAvrcpCtNotiNowPlayingInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_NOW_PLAYING_IND;
    indMsg->connectionId = appConnId;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
static void csrBtAvrcpCtNotiAvailablePlayersIndSend(CsrSchedQid ctrlHandle,
                                                    CsrUint8 appConnId)
{
    CsrBtAvrcpCtNotiAvailablePlayersInd *indMsg = (CsrBtAvrcpCtNotiAvailablePlayersInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_AVAILABLE_PLAYERS_IND;
    indMsg->connectionId = appConnId;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}
#endif

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
static void csrBtAvrcpCtNotiUidsIndSend(CsrSchedQid ctrlHandle,
                                        CsrUint8 appConnId,
                                        CsrUint16 uidCounter)
{
    CsrBtAvrcpCtNotiUidsInd *indMsg = (CsrBtAvrcpCtNotiUidsInd *) CsrPmemZalloc(sizeof(*indMsg));

    indMsg->type = CSR_BT_AVRCP_CT_NOTI_UIDS_IND;
    indMsg->connectionId = appConnId;
    indMsg->uidCounter = uidCounter;
    CsrBtAvrcpMessagePut(ctrlHandle, indMsg);
}
#endif

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (INSTALL_AVRCP_METADATA_ATTRIBUTES)
CsrBtAvrcpCtGetAttributesCfm *CsrBtAvrcpCtGetAttributesCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid uid)
{
    CsrBtAvrcpCtGetAttributesCfm *cfmMsg = (CsrBtAvrcpCtGetAttributesCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_GET_ATTRIBUTES_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->scope           = scope;
    cfmMsg->attributeCount  = 0;
    cfmMsg->attribDataLen   = 0;
    cfmMsg->attribData      = NULL;
    SynMemCpyS(cfmMsg->uid, sizeof(CsrBtAvrcpUid), uid, sizeof(CsrBtAvrcpUid));
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;
    cfmMsg->attribDataPayloadOffset = 0;

    return cfmMsg;
}
#endif

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
CsrBtAvrcpCtPlayCfm *CsrBtAvrcpCtPlayCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid)
{
    CsrBtAvrcpCtPlayCfm *cfmMsg = (CsrBtAvrcpCtPlayCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_PLAY_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->scope           = scope;
    SynMemCpyS(cfmMsg->uid, sizeof(CsrBtAvrcpUid), *uid, sizeof(CsrBtAvrcpUid));
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}

CsrBtAvrcpCtAddToNowPlayingCfm *CsrBtAvrcpCtAddToNowPlayingCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid)
{
    CsrBtAvrcpCtAddToNowPlayingCfm *cfmMsg = (CsrBtAvrcpCtAddToNowPlayingCfm *) CsrPmemZalloc(sizeof(*cfmMsg));

    cfmMsg->type            = CSR_BT_AVRCP_CT_ADD_TO_NOW_PLAYING_CFM;
    cfmMsg->connectionId    = connId;
    cfmMsg->scope           = scope;
    SynMemCpyS(cfmMsg->uid, sizeof(CsrBtAvrcpUid), *uid, sizeof(CsrBtAvrcpUid));
    cfmMsg->resultCode      = CSR_BT_RESULT_CODE_AVRCP_TIMEOUT;
    cfmMsg->resultSupplier  = CSR_BT_SUPPLIER_AVRCP;

    return cfmMsg;
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

/*****************************************************************************
 * If comparison is not asked, notification indication will be sent to application.
 * Otherwise notification indication will sent to application only if the
 * notification parameter values have changed from last reported values.
 * Notifications without parameters will never be reported if comparison is asked.
 *****************************************************************************/
void CsrBtAvrcpCtNotiIndSend(AvrcpConnInstance_t *connInst,
                             CsrUint16 rxDataLen,
                             CsrUint8 *rxData,
                             CsrBool compare)
{
    CsrBtAvrcpCtNotiParams *notiParams = &connInst->ctLocal->notiParams;

    switch (rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX])
    {
        case CSR_BT_AVRCP_NOTI_ID_PLAYBACK_STATUS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_PS_HEADER_SIZE)
            {
                CsrBtAvrcpPlaybackStatus playbackStatus = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_VAL_INDEX];

                if (!compare || notiParams->playbackStatus != playbackStatus)
                {
                    csrBtAvrcpNotiPlaybackStatusIndSend(connInst->instData->ctrlHandle,
                                                        connInst->appConnId,
                                                        playbackStatus);
                    /* Save the notification parameter */
                    notiParams->playbackStatus = playbackStatus;
                }
            }
            break;
        }

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
        case CSR_BT_AVRCP_NOTI_ID_TRACK:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_TC_HEADER_SIZE)
            {
                CsrBtAvrcpUid trackUid;

                CSR_BT_AVRCP_UID_COPY(trackUid,
                                      &rxData[AVRCP_DATA_PDU_REG_NOTI_RES_VAL_INDEX]);

                if (!compare || CsrMemCmp(&notiParams->trackUid,
                                          &trackUid,
                                          sizeof(trackUid)))
                {
                    csrBtAvrcpCtNotiTrackChangeIndSend(connInst->instData->ctrlHandle,
                                                       connInst->appConnId,
                                                       &trackUid);
                    /* Save the notification parameter */
                    SynMemCpyS(&notiParams->trackUid,
                              sizeof(notiParams->trackUid),
                              &trackUid,
                              sizeof(notiParams->trackUid));
                }
            }
            break;
        }
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_NOTIFICATIONS
        case CSR_BT_AVRCP_NOTI_ID_TRACK_END:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_TE_HEADER_SIZE)
            {
                if (!compare)
                {
                    csrBtAvrcpCtNotiTrackEndIndSend(connInst->instData->ctrlHandle,
                                                    connInst->appConnId);
                }
            }
            break;
        }

        case CSR_BT_AVRCP_NOTI_ID_TRACK_START:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_TS_HEADER_SIZE)
            {
                if (!compare)
                {
                    csrBtAvrcpCtNotiTrackStartIndSend(connInst->instData->ctrlHandle,
                                                      connInst->appConnId);
                }
            }
            break;
        }

        case CSR_BT_AVRCP_NOTI_ID_PLAYBACK_POS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_PBP_HEADER_SIZE)
            {
                CsrUint32 playbackPos = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_REG_NOTI_RES_VAL_INDEX]);

                if (!compare || notiParams->playbackPos != playbackPos)
                {
                    csrBtAvrcpCtNotiPlaybackPosIndSend(connInst->instData->ctrlHandle,
                                                       connInst->appConnId,
                                                       playbackPos);
                    /* Save the notification parameter */
                    notiParams->playbackPos = playbackPos;
                }
            }
            break;
        }

        case CSR_BT_AVRCP_NOTI_ID_BATT_STATUS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_BATT_HEADER_SIZE)
            {
                CsrBtAvrcpBatteryStatus batteryStatus = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_BATT_LEVEL_INDEX];

                if (!compare || notiParams->batteryStatus != batteryStatus)
                {
                    csrBtAvrcpCtNotiBatteryStatusIndSend(connInst->instData->ctrlHandle,
                                                         connInst->appConnId,
                                                         batteryStatus);
                    /* Save the notification parameter */
                    notiParams->batteryStatus = batteryStatus;
                }
            }
            break;
        }

        case CSR_BT_AVRCP_NOTI_ID_SYSTEM_STATUS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_SYS_HEADER_SIZE)
            {
                CsrBtAvrcpSystemStatus systemStatus = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_VAL_INDEX];

                if (!compare || notiParams->systemStatus != systemStatus)
                {
                    csrBtAvrcpCtNotiSystemStatusIndSend(connInst->instData->ctrlHandle,
                                                        connInst->appConnId,
                                                        systemStatus);
                    /* Save the notification parameter */
                    notiParams->systemStatus = systemStatus;
                }
            }
            break;
        }
#endif /* INSTALL_AVRCP_NOTIFICATIONS */

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
        case CSR_BT_AVRCP_NOTI_ID_PAS:
        {
            if ((rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_PAS_MIN_HEADER_SIZE) &&
                (rxDataLen == (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_REG_NOTI_RES_PAS_HEADER_SIZE
                               + rxData[AVRCP_DATA_PDU_REG_NOTI_RES_PAS_NUM_INDEX]
                                        * AVRCP_DATA_PDU_REG_NOTI_RES_PAS_PART_SIZE)))
            {
                CsrUintFast8 i;
                CsrUint8 attValPairCount = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_PAS_NUM_INDEX];
                CsrBtAvrcpPasAttValPair *attValPair = CsrPmemAlloc(attValPairCount
                                * AVRCP_DATA_PDU_REG_NOTI_RES_PAS_PART_SIZE);

                for (i = 0; i < attValPairCount; i++)
                {
                    attValPair[i].attribId = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_PAS_ATT_INDEX
                                    + i * AVRCP_DATA_PDU_REG_NOTI_RES_PAS_PART_SIZE];
                    attValPair[i].valueId = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_PAS_VAL_INDEX
                                    + i * AVRCP_DATA_PDU_REG_NOTI_RES_PAS_PART_SIZE];
                }

                if (!compare ||
                    notiParams->attValPairCount != attValPairCount ||
                    !csrBtAvrcpCtCompareAttribValPair(notiParams->attValPair,
                                                      attValPair,
                                                      attValPairCount))
                {
                    csrBtAvrcpCtPasSetIndSend(connInst->instData->ctrlHandle,
                                              connInst->appConnId,
                                              attValPair,
                                              attValPairCount);
                    /* Save the notification parameters */
                    CsrPmemFree(notiParams->attValPair);
                    notiParams->attValPairCount = attValPairCount;
                    notiParams->attValPair = CsrMemDup(attValPair,
                                                       attValPairCount);
                }
                else
                {
                    CsrPmemFree(attValPair);
                }
            }
            break;
        }
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
        case CSR_BT_AVRCP_NOTI_ID_ADDRESSED_PLAYER:
        {
            if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_SYS_HEADER_SIZE)
            {
                CsrUint16 playerId = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_REG_NOTI_RES_AP_PID_INDEX]);
                CsrUint16 uidCounter = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_REG_NOTI_RES_AP_UIDC_INDEX]);

                if (!compare ||
                    notiParams->playerId != playerId ||
                    notiParams->addressedPlayerUidCounter != uidCounter)
                {
                    csrBtAvrcpCtSetAddressedPlayerIndSend(connInst->instData->ctrlHandle,
                                                          connInst->appConnId,
                                                          playerId,
                                                          uidCounter);

                    if (!(connInst->ctLocal->notiConfig & CSR_BT_AVRCP_NOTI_REG_NON_PERSISTENT) && /* Persistent notification */
                        !(AVRCP_LIST_CT_PMSG_GET_TLABEL((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList,
                                                        AVRCP_TLABEL_GET(rxData)))) /* No pending notification registration */
                    {
                        /* Different player may have different capabilities
                         * The new player may support events which were not supported by previous player.
                         * AVRCP controller must register for such notification events if application
                         * have requested for those events. */
                        CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = CsrBtAvrcpCtNotiRegisterCfmBuild(connInst->appConnId,
                                                                                               connInst->ctLocal->playbackInterval,
                                                                                               connInst->ctLocal->ctRequestedNotifications);
                        AvrcpCtPendingMsgInfo_t *pendingMsgInfo = CsrBtAvrcpCtUtilMsgQueueAdd(connInst,
                                                                                              cfmMsg,
                                                                                              CSR_SCHED_QID_INVALID, /* Avoid sending confirmation to application */
                                                                                              CSR_BT_AVCTP_PSM);

                         /* Start by determining which notifications the remote device supports. */
                        CsrBtAvrcpCtGetCapabilitiesCmdSend(connInst,
                                                           pendingMsgInfo,
                                                           AVRCP_DATA_PDU_GET_CAP_CMN_NOTI_SUP);
                    }

                    /* Save the notification parameters */
                    notiParams->playerId = playerId;
                    notiParams->addressedPlayerUidCounter = uidCounter;
                }
            }
            break;
        }
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        case CSR_BT_AVRCP_NOTI_ID_NOW_PLAYING_CONTENT:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_NPL_HEADER_SIZE)
            {
                if (!compare)
                {
                    csrBtAvrcpCtNotiNowPlayingIndSend(connInst->instData->ctrlHandle,
                                                      connInst->appConnId);
                }
            }
            break;
        }
#endif

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
        case CSR_BT_AVRCP_NOTI_ID_AVAILABLE_PLAYERS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE + 1)
            {
                if (!compare)
                {
                    csrBtAvrcpCtNotiAvailablePlayersIndSend(connInst->instData->ctrlHandle,
                                                            connInst->appConnId);
                }
            }
            break;
        }
#endif

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        case CSR_BT_AVRCP_NOTI_ID_UIDS:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_UIDS_HEADER_SIZE)
            {
                CsrUint16 uidCounter = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_REG_NOTI_RES_UIDS_UIDC_INDEX]);

                if (!compare || notiParams->uidCounter != uidCounter)
                {
                    csrBtAvrcpCtNotiUidsIndSend(connInst->instData->ctrlHandle,
                                                connInst->appConnId,
                                                uidCounter);
                    /* Save the notification parameter */
                    notiParams->uidCounter = uidCounter;
                }
            }
            break;
        }
#endif

        case CSR_BT_AVRCP_NOTI_ID_VOLUME:
        {
            if (rxDataLen == AVRCP_DATA_MD_HEADER_SIZE
                            + AVRCP_DATA_PDU_REG_NOTI_RES_VOL_HEADER_SIZE)
            {
                CsrUint8 volume = rxData[AVRCP_DATA_PDU_REG_NOTI_RES_VOL_INDEX];

                if (!compare || notiParams->volume != volume)
                {
                    csrBtAvrcpCtNotiVolumeIndSend(connInst->instData->ctrlHandle,
                                                  connInst->appConnId,
                                                  volume);
                    /* Save the notification parameter */
                    notiParams->volume = volume;
                }
            }
            break;
        }

        default:
        {
            break;
        }
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER */
#endif /* #ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE */
#endif /* #ifndef EXCLUDE_CSR_BT_AVRCP_MODULE */

