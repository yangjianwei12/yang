/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE

#include "csr_bt_util.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_bt_avrcp_lib.h"

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif

/* PDU ID CFM primitive table structure */
typedef struct
{
    CsrUint8         pduId;         /* PDU ID */
    CsrBtAvrcpPrim   avrcpCfmPrim;  /* primitive ID */
} CsrBtAvrcpPduIdPrimTableType;

static const CsrBtAvrcpPduIdPrimTableType vendorDependentPduIdPrimTable[]=
{
   /* 0x10 */ {AVRCP_DATA_PDU_ID_GET_CAPABILITIES,    CSR_BT_AVRCP_CT_NOTI_REGISTER_CFM},
#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
   /* 0x11 */ {AVRCP_DATA_PDU_ID_LIST_PAS_ATTRIBUTES, CSR_BT_AVRCP_CT_PAS_ATT_ID_CFM},
   /* 0x12 */ {AVRCP_DATA_PDU_ID_LIST_PAS_VALUES,     CSR_BT_AVRCP_CT_PAS_VAL_ID_CFM},
   /* 0x13 */ {AVRCP_DATA_PDU_ID_GET_CUR_PAS_VALUES,  CSR_BT_AVRCP_CT_PAS_CURRENT_CFM},
   /* 0x14 */ {AVRCP_DATA_PDU_ID_SET_PAS_VALUES,      CSR_BT_AVRCP_CT_PAS_SET_CFM},
   /* 0x15 */ {AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT, CSR_BT_AVRCP_CT_PAS_ATT_TXT_CFM},
   /* 0x16 */ {AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT,  CSR_BT_AVRCP_CT_PAS_VAL_TXT_CFM},
   /* 0x17 */ {AVRCP_DATA_PDU_ID_INFO_DISP_CHAR_SET,  CSR_BT_AVRCP_CT_INFORM_DISP_CHARSET_CFM},
   /* 0x18 */ {AVRCP_DATA_PDU_ID_INFO_BAT_STAT_OF_CT, CSR_BT_AVRCP_CT_INFORM_BATTERY_STATUS_CFM},
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */
#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
   /* 0x20 */ {AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES, CSR_BT_AVRCP_CT_GET_ATTRIBUTES_CFM},
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */
#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
   /* 0x30 */ {AVRCP_DATA_PDU_ID_GET_PLAY_STATUS,     CSR_BT_AVRCP_CT_GET_PLAY_STATUS_CFM},
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */
   /* 0x31 */ {AVRCP_DATA_PDU_ID_REG_NOTI,            CSR_BT_AVRCP_CT_NOTI_REGISTER_CFM},
   /* 0x50 */ {AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME, CSR_BT_AVRCP_CT_SET_VOLUME_CFM},
#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
   /* 0x60 */ {AVRCP_DATA_PDU_ID_SET_ADDRESSED_PLAYER,CSR_BT_AVRCP_CT_SET_ADDRESSED_PLAYER_CFM}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */
};

static const CsrBtAvrcpPduIdPrimTableType pduIdPrimTable[]=
{
#ifdef INSTALL_AVRCP_UNIT_COMMANDS
   /* 0x30 */ {AVRCP_DATA_AVC_OPCODE_UNIT_INFO,       CSR_BT_AVRCP_CT_UNIT_INFO_CMD_CFM},
   /* 0x31 */ {AVRCP_DATA_AVC_OPCODE_SUBUNIT_INFO,    CSR_BT_AVRCP_CT_SUB_UNIT_INFO_CMD_CFM},
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */
   /* 0x50 */ {AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME, CSR_BT_AVRCP_CT_SET_VOLUME_CFM},
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
   /* 0x70 */ {AVRCP_DATA_PDU_ID_SET_BROWSED_PLAYER,  CSR_BT_AVRCP_CT_SET_BROWSED_PLAYER_CFM},
   /* 0x71 */ {AVRCP_DATA_PDU_ID_GET_FOLDER_ITEMS,    CSR_BT_AVRCP_CT_GET_FOLDER_ITEMS_CFM},
   /* 0x72 */ {AVRCP_DATA_PDU_ID_CHANGE_PATH,         CSR_BT_AVRCP_CT_CHANGE_PATH_CFM},
   /* 0x73 */ {AVRCP_DATA_PDU_ID_GET_ITEM_ATTRIBUTES, CSR_BT_AVRCP_CT_GET_ATTRIBUTES_CFM},
   /* 0x74 */ {AVRCP_DATA_PDU_ID_PLAY_ITEM,           CSR_BT_AVRCP_CT_PLAY_CFM},
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
   /* 0x7C */ {AVRCP_DATA_AVC_OPCODE_PASS_THROUGH,    CSR_BT_AVRCP_CT_PASS_THROUGH_CFM},
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
   /* 0x80 */ {AVRCP_DATA_PDU_ID_SEARCH,              CSR_BT_AVRCP_CT_SEARCH_CFM},
   /* 0x90 */ {AVRCP_DATA_PDU_ID_ADD_TO_NOW_PLAYING,  CSR_BT_AVRCP_CT_ADD_TO_NOW_PLAYING_CFM}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
};

/* Static functions declarations */
static void csrBtAvrcpCtVendorRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
static void csrBtAvrcpCtPasListAttRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtPasListValRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtPasGetCurrValRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtPasSetValueRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtPasGetAttTextRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtPasGetValTextRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtDispCharSetRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
static void csrBtAvrcpCtInformBatteryStatusOfCtRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData);
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

static CsrBool csrBtAvrcpCheckRspAgainstPendingMsg(AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint8 pduId, CsrUint8 *rxData)
{
    CsrUint8 i;
    
    if (pduId == AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT)
    {
        pduId           = rxData[AVRCP_DATA_MD_PDU_ID_INDEX];
        
        for (i=0; (i<CSR_ARRAY_SIZE(vendorDependentPduIdPrimTable) && (vendorDependentPduIdPrimTable[i].pduId <= pduId)); i++)
        {/* The PDU Id/ prim table is sorted in ascending PUD Id orden */
            if (vendorDependentPduIdPrimTable[i].pduId == pduId)
            {/* PDU Id found: check the primitive type */
                if (vendorDependentPduIdPrimTable[i].avrcpCfmPrim == *((CsrBtAvrcpPrim *)pendingMsgInfo->cfmMsg))
                {
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
        }
    }
    else
    {
        for (i=0; (i<CSR_ARRAY_SIZE(pduIdPrimTable) && (pduIdPrimTable[i].pduId <= pduId)); i++)
        {/* The PDU Id/ prim table is sorted in ascending PUD Id orden */
            if (pduIdPrimTable[i].pduId == pduId)
            {/* PDU Id found: check the primitive type */
                if (pduIdPrimTable[i].avrcpCfmPrim == *((CsrBtAvrcpPrim *)pendingMsgInfo->cfmMsg))
                {
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
        }
    }
    /* PDU not on the lists: just accept and handle it */
    return TRUE;
}

CsrBool CsrBtAvrcpCtPassThroughCmdSend(AvrcpConnInstance_t *connInst,
                                       AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                       CsrUint8 opId,
                                       CsrUint8 state)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_RCP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_PT_SIZE);

        CsrBtAvrcpDataInsertAvctpHeader(txData,
                                        connInst->control.ctTLabel, /* Transaction label */
                                        AVRCP_DATA_AVCTP_CR_CMD); /* Command or response */

        CsrBtAvrcpDataInsertAvcCommonHeader(txData,
                                            AVRCP_DATA_AVC_CTYPE_CONTROL);
        CsrBtAvrcpDataInsertAvcPassThroughHeader(txData, state, opId);
        CsrBtAvrcpControlDataSend(connInst, AVRCP_DATA_PT_SIZE, txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void csrBtAvrcpCtPassThroughRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPassThroughCfm *cfmMsg = (CsrBtAvrcpCtPassThroughCfm *)pendingMsgInfo->cfmMsg;

    if ((CsrBtAvrcpUtilDataCheckPT(rxDataLen, rxData)) &&
        ((*(CsrBtAvrcpPrim *)cfmMsg) == CSR_BT_AVRCP_CT_PASS_THROUGH_CFM))
    {

        if (pendingMsgInfo->tmpState == AVRCP_STATE_PM_PT_RELEASE_PENDING)
        {
            pendingMsgInfo->tmpState = AVRCP_STATE_PM_IDLE;
#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
            if ((cfmMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_NEXT) ||
                (cfmMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_PREV))
            {/* Group navigation */
                /* Transaction label allocation would never fail in this case */
                CsrBtAvrcpCtGroupNavigationCmdSend(connInst,
                                                   pendingMsgInfo,
                                                   (CsrUint16) (cfmMsg->operationId == CSR_BT_AVRCP_PT_OP_ID_GROUP_NAV_NEXT ?
                                                                   AVRCP_DATA_PT_GN_OPERATION_NEXT :
                                                                   AVRCP_DATA_PT_GN_OPERATION_PREV),
                                                   CSR_BT_AVRCP_PT_STATE_RELEASE);
            }
            else
#endif
            {/* Pass-through */
                /* Transaction label allocation would never fail in this case */
                CsrBtAvrcpCtPassThroughCmdSend(connInst,
                                               pendingMsgInfo,
                                               cfmMsg->operationId,
                                               CSR_BT_AVRCP_PT_STATE_RELEASE);
            }
        }
        else
        {
            CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                &cfmMsg->resultCode, &cfmMsg->resultSupplier);
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
CsrBool CsrBtAvrcpCtUnitInfoCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrUint16 pDataLen,
                                    CsrUint8 *pData)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_RCP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_AVC_UNIT_RES_SIZE
                        + pDataLen);

        CsrBtAvrcpDataInsertAvctpHeader(txData,
                                        connInst->control.ctTLabel,
                                        AVRCP_DATA_AVCTP_CR_CMD); /* Command or response */

        txData[AVRCP_DATA_AVC_CRTYPE_INDEX] = AVRCP_DATA_AVC_CTYPE_STATUS;
        txData[AVRCP_DATA_AVC_SUBUNIT_TYPE_INDEX] = 0xFF; /* Type + ID */
        txData[AVRCP_DATA_AVC_OPCODE_INDEX] = AVRCP_DATA_AVC_OPCODE_UNIT_INFO; /* 0x30 */

        CsrMemSet(&txData[AVRCP_DATA_AVC_OPERAND_INDEX],
                  0xFF,
                  (CsrUint16) (AVRCP_DATA_MD_PARAM_LEN_INDEX - AVRCP_DATA_AVC_OPERAND_INDEX));
        SynMemCpyS(&txData[AVRCP_DATA_MD_PARAM_LEN_INDEX], pDataLen, pData, pDataLen);

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_AVC_UNIT_RES_SIZE + pDataLen),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtSubUnitInfoCmdSend(AvrcpConnInstance_t *connInst,
                                       AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                       CsrUint16 pDataLen,
                                       CsrUint8 *pData)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_RCP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_AVC_SUBUNIT_RES_SIZE
                        + pDataLen);

        CsrBtAvrcpDataInsertAvctpHeader(txData,
                                        connInst->control.ctTLabel,
                                        AVRCP_DATA_AVCTP_CR_CMD); /* Command or response */

        txData[AVRCP_DATA_AVC_CRTYPE_INDEX] = AVRCP_DATA_AVC_CTYPE_STATUS;
        txData[AVRCP_DATA_AVC_SUBUNIT_TYPE_INDEX] = 0xFF; /* Type + ID */
        txData[AVRCP_DATA_AVC_OPCODE_INDEX] = AVRCP_DATA_AVC_OPCODE_SUBUNIT_INFO; /* 0x31 */

        CsrMemSet(&txData[AVRCP_DATA_AVC_OPERAND_INDEX],
                  0xFF,
                  (CsrUint16) (AVRCP_DATA_MD_PARAM_LEN_INDEX - AVRCP_DATA_AVC_OPERAND_INDEX));
        SynMemCpyS(&txData[AVRCP_DATA_MD_PARAM_LEN_INDEX], pDataLen, pData, pDataLen);

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_AVC_SUBUNIT_RES_SIZE + pDataLen),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
CsrBool CsrBtAvrcpCtGroupNavigationCmdSend(AvrcpConnInstance_t *connInst,
                                           AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                           CsrUint16 operation,
                                           CsrUint8 state)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_PT_GN_SIZE);

        CsrBtAvrcpDataInsertAvctpHeader(txData,
                                        connInst->control.ctTLabel, /* Transaction label */
                                        AVRCP_DATA_AVCTP_CR_CMD); /* Command or response */

        CsrBtAvrcpDataInsertAvcCommonHeader(txData,
                                            AVRCP_DATA_AVC_CTYPE_CONTROL);
        CsrBtAvrcpDataInsertAvcGroupNavigationHeader(txData, state, operation);

        CsrBtAvrcpControlDataSend(connInst, AVRCP_DATA_PT_GN_SIZE, txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtGetCapabilitiesCmdSend(AvrcpConnInstance_t *connInst,
                                           AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                           CsrUint8 capaType)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {

        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_GET_CAP_CMD_HEADER_SIZE);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_GET_CAPABILITIES,/* PDU ID */
                                       AVRCP_DATA_PDU_GET_CAP_CMD_HEADER_SIZE); /* MD parameter length */

        txData[AVRCP_DATA_PDU_GET_CAP_CMN_ID_INDEX] = capaType;

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_GET_CAP_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtRegisterNotificationCmdSend(AvrcpConnInstance_t *connInst,
                                                AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                CsrUint8 notificationId,
                                                CsrUint32 interval)
{
    CsrUint8 *txData;

    if (pendingMsgInfo)
    {/* if pendingMsgInfo is a NULL pointer, then the ctTLabel already has a value */
        if (!CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                         pendingMsgInfo,
                                         CSR_BT_AVRCP_TIMER_MTP))
        {
            /* Transaction label not available */
            return (FALSE);
        }
    }

    txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                          + AVRCP_DATA_PDU_REG_NOTI_CMD_HEADER_SIZE);

    CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                   connInst->control.ctTLabel, /* Transaction label */
                                   AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                   AVRCP_DATA_AVC_CTYPE_NOTIFY, /* Command/response type */
                                   AVRCP_DATA_PDU_ID_REG_NOTI, /* PDU ID */
                                   AVRCP_DATA_PDU_REG_NOTI_CMD_HEADER_SIZE); /* MD parameter length */

    txData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX] = notificationId;

    CSR_COPY_UINT32_TO_BIG_ENDIAN(notificationId == CSR_BT_AVRCP_NOTI_ID_PLAYBACK_POS ? interval : 0,
                                  &txData[AVRCP_DATA_PDU_REG_NOTI_CMD_PLAY_INT_INDEX]);

    /* Store the TLabel for use when a response is received */
    connInst->ctLocal->notiList[notificationId - CSR_BT_AVRCP_NOTI_ID_OFFSET] = connInst->control.ctTLabel;

    CsrBtAvrcpControlDataSend(connInst,
    AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_REG_NOTI_CMD_HEADER_SIZE,
                              txData);
    connInst->ctLocal->activeNotifications++;

    return (TRUE);
}

CsrBool CsrBtAvrcpCtSetAbsoluteVolumeCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrUint8 volume)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {

        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_SET_VOLUME_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertControlHeader(txData,
                                          pendingMsgInfo->tLabel,
                                          AVRCP_DATA_AVCTP_CR_CMD,
                                          AVRCP_DATA_AVC_CTYPE_CONTROL,
                                          AVRCP_DATA_AVC_SUBUNIT_TYPE_PANEL,
                                          0x00, /* Subunit ID*/
                                          AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT,
                                          AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME,
                                          AVRCP_DATA_PDU_SET_VOLUME_CMD_HEADER_SIZE);

        txData[AVRCP_DATA_PDU_SET_VOLUME_CMD_INDEX] = volume;

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_SET_VOLUME_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void csrBtAvrcpCtGetCapRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{/* Only handled in relation to a CSR_BT_AVRCP_CT_NOTI_REGISTER_REQ */
    CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = (CsrBtAvrcpCtNotiRegisterCfm *)pendingMsgInfo->cfmMsg;

    if (rxData[AVRCP_DATA_PDU_GET_CAP_RES_CAPA_ID_INDEX] == AVRCP_DATA_PDU_GET_CAP_CMN_NOTI_SUP)
    {/* This is a response to the get supported notifications command */
        if (rxDataLen == AVRCP_DATA_PDU_GET_CAP_RES_CAPA_INDEX + rxData[AVRCP_DATA_PDU_GET_CAP_RES_COUNT_INDEX])
        {
            CsrUintFast8 i;

            connInst->ctLocal->tgSupportedNotifications = 0;

            for (i = 0; i < rxData[AVRCP_DATA_PDU_GET_CAP_RES_COUNT_INDEX]; i++)
            {/* Skip through all the announced notifications */
                if (rxData[AVRCP_DATA_PDU_GET_CAP_RES_CAPA_INDEX + i] <= CSR_BT_AVRCP_NOTI_ID_MAXIMUM)
                {/* Only handle known notifications */
                    /* Convert notification IDs to a bitmask */
                    CSR_BIT_SET(connInst->ctLocal->tgSupportedNotifications, rxData[AVRCP_DATA_PDU_GET_CAP_RES_CAPA_INDEX + i] - CSR_BT_AVRCP_NOTI_ID_OFFSET);
                }
            }

            /* Start registering for the notifications */
            CsrBtAvrcpCtRegisterNextNotification(connInst, pendingMsgInfo);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_UNSPECIFIED_ERROR;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}

static void csrBtAvrcpCtReleaseNotiTLabel(AvrcpConnInstance_t *connInst,
                                          CsrUint8 notiId)
{
    /* Invalidate used TLabel */
    if (notiId >= CSR_BT_AVRCP_NOTI_ID_OFFSET &&
        notiId <= CSR_BT_AVRCP_NOTI_ID_MAXIMUM)
    { /* Valid Notification ID */
        if (connInst->ctLocal->notiList[notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET] != CSR_BT_AVRCP_TLABEL_INVALID)
        { /* Notification has reserved a transaction label */
            connInst->ctLocal->notiList[notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET] = CSR_BT_AVRCP_TLABEL_INVALID;
            if (connInst->ctLocal->activeNotifications > 0)
            {
                connInst->ctLocal->activeNotifications--;
            }
        }
    }
}

static void csrBtAvrcpCtRegisterNotificationRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    if (pendingMsgInfo)
    {/* An CSR_BT_AVRCP_CT_NOTI_REGISTER_REQ is being processed (multiple notifications being registered for) */
        CsrBtAvrcpCtNotiRegisterCfm *cfmMsg = (CsrBtAvrcpCtNotiRegisterCfm *)pendingMsgInfo->cfmMsg;

        if (AVRCP_DATA_AVC_CRTYPE_GET(rxData) == CSR_BT_AVRCP_DATA_AVC_RTYPE_INTERIM)
        {/* Registration succeeded - set corresponding flag */
            CSR_BIT_SET(cfmMsg->notiMask, pendingMsgInfo->tmpState - CSR_BT_AVRCP_NOTI_ID_OFFSET);

            /* Send first interim response to application */
            CsrBtAvrcpCtNotiIndSend(connInst, rxDataLen, rxData, FALSE);
        }
        else
        { /* Registration failed */
            csrBtAvrcpCtReleaseNotiTLabel(connInst, pendingMsgInfo->tmpState);
        }

        /* Register pending notifications if any */
        CsrBtAvrcpCtRegisterNextNotification(connInst, pendingMsgInfo);
    }
    else
    {/* No confirmation message is pending */
        if (AVRCP_DATA_AVC_CRTYPE_GET(rxData) == CSR_BT_AVRCP_DATA_AVC_RTYPE_CHANGED)
        {
            CsrUint8 notiId = rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX];

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
            /* For UID changed indication release the OBEX service connection */
            if (notiId == CSR_BT_AVRCP_NOTI_ID_UIDS)
            {
                if (connInst->ctLocal->ctObexState == AVRCP_CT_OBEX_SERVICE_CONNECTED)
                {
                    CsrBtAvrcpImagingClientDisconnectReqSend(connInst->appConnId, TRUE);
                    connInst->ctLocal->ctObexState = AVRCP_CT_OBEX_SERVICE_DISCONNECTING;
                }
            }
#endif

            /* Check if TLabel matches the registered notification ID */
            if (CsrBtAvrcpCtMatchNotificationLabel(connInst,
                                                   notiId,
                                                   AVRCP_TLABEL_GET(rxData)))
            {/* Notification has been previously registered */
                /* Reregister for a new notification */
                if (CSR_BIT_IS_SET(connInst->ctLocal->ctRequestedNotifications,
                                   notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET)
                    && !(connInst->ctLocal->notiConfig & CSR_BT_AVRCP_NOTI_REG_NON_PERSISTENT))
                {
                    /* We reserve and re-use same transaction label for notifications.
                     * Thus we don't allocated new transaction label while re-registering
                     * notifications. But CsrBtAvrcpCtRegisterNotificationCmdSend() uses
                     * 'ctTLabel' from 'connInst' directly. To resolve this we set
                     * transaction label counter (ctTLabel) to notification transaction
                     * label temporarily */
                    CsrUint8 ctTLabelBackup = connInst->control.ctTLabel;

                    /* Reuse the same TLabel for re-registration */
                    connInst->control.ctTLabel = connInst->ctLocal->notiList[notiId - CSR_BT_AVRCP_NOTI_ID_OFFSET];
                    CsrBtAvrcpCtRegisterNotificationCmdSend(connInst,
                                                            pendingMsgInfo,
                                                            notiId, /* Notification ID */
                                                            connInst->ctLocal->playbackInterval); /* Playback interval */

                    /* Restore transaction label counter */
                    connInst->control.ctTLabel = ctTLabelBackup;
                }
                else
                {/* Invalidate used TLabel */
                    csrBtAvrcpCtReleaseNotiTLabel(connInst, notiId);
                }

                /* Send indication to application */
                CsrBtAvrcpCtNotiIndSend(connInst, rxDataLen, rxData, FALSE);
            }
        }
        else if (AVRCP_DATA_AVC_CRTYPE_GET(rxData) == CSR_BT_AVRCP_DATA_AVC_RTYPE_INTERIM)
        {
            CsrUint8 notiId = rxData[AVRCP_DATA_PDU_REG_NOTI_CMN_NOTI_ID_INDEX];

            /* Check if TLabel matches the registered notification ID */
            if (CsrBtAvrcpCtMatchNotificationLabel(connInst,
                                                   notiId,
                                                   AVRCP_TLABEL_GET(rxData)))
            {/* Notification has been previously registered */
                /* Send notification indications only if parameter values have changed */
                CsrBtAvrcpCtNotiIndSend(connInst, rxDataLen, rxData, TRUE);
            }
            else
            {/* Invalidate used TLabel */
                csrBtAvrcpCtReleaseNotiTLabel(connInst, notiId);
            }
        }
        else
        { /* NOT_IMPLEMENETED or REJECTED */
            CsrUint8 notiId;
            CsrUint8 tLabel = AVRCP_TLABEL_GET(rxData);

            /* NOT_IMPLEMENTED or REJECTED response does not contain notification event ID.
             * Find notification ID by comparing notification transaction label */
            for (notiId = CSR_BT_AVRCP_NOTI_ID_OFFSET; notiId <= CSR_BT_AVRCP_NOTI_ID_MAXIMUM; notiId++)
            {
                if (CsrBtAvrcpCtMatchNotificationLabel(connInst,
                                                       notiId,
                                                       tLabel))
                { /* Found correct notification */
                    /* Release notification transaction label */
                    csrBtAvrcpCtReleaseNotiTLabel(connInst, notiId);
                    break;
                }
            }
        }
    }
}

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
static void csrBtAvrcpCtGetElemAttRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtGetAttributesCfm *cfmMsg = (CsrBtAvrcpCtGetAttributesCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrUint8 *pData = CsrPmemAlloc(rxDataLen);

        SynMemCpyS(pData, rxDataLen, rxData,rxDataLen);

        switch(AVRCP_DATA_MD_PACKET_TYPE_GET(rxData))
        {
            case AVRCP_DATA_MD_PACKET_TYPE_SINGLE:
            {  /* Single frame: just send confirmation */
                CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                                &cfmMsg->resultCode, &cfmMsg->resultSupplier);
                cfmMsg->attributeCount     = rxData[AVRCP_DATA_PDU_GET_ELEM_ATT_RES_ATT_COUNT_INDEX];
                cfmMsg->attribDataLen      = rxDataLen;
                cfmMsg->attribData         = pData;
                cfmMsg->attribDataPayloadOffset = AVRCP_DATA_PDU_GET_ELEM_ATT_RES_ID_INDEX;
                CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                break;
            }
            case AVRCP_DATA_MD_PACKET_TYPE_START:
            {
                /* start fragment: store attribute count(not present in continue/end packets) */
                cfmMsg->attributeCount = rxData[AVRCP_DATA_PDU_GET_ELEM_ATT_RES_ATT_COUNT_INDEX];
                CsrBtAvrcpCtGetAttributesIndSend(pendingMsgInfo->phandle,
                                                 cfmMsg->connectionId,
                                                 cfmMsg->scope,
                                                 &cfmMsg->uid,
                                                 cfmMsg->attributeCount,
                                                 rxDataLen,
                                                 pData,
                                                 AVRCP_DATA_PDU_GET_ELEM_ATT_RES_ID_INDEX);
                connInst->ctLocal->pendingMsgInfo = pendingMsgInfo;

                /* We can reuse same pendingMsgInfo for new transaction
                 * Just reset the transaction timer */
                CsrBtAvrcpCtResetTransactionTimer(pendingMsgInfo,
                                                  CSR_BT_AVRCP_TIMER_MTC);
                break;
            }   
            case AVRCP_DATA_MD_PACKET_TYPE_END:
            {
                /* end fragment: no attribute count in fragment, adjust attrib data offset */
                CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                                                    &cfmMsg->resultCode, &cfmMsg->resultSupplier);
                cfmMsg->attribDataLen      = rxDataLen;
                cfmMsg->attribData         = pData;
                cfmMsg->attribDataPayloadOffset = AVRCP_DATA_MD_PARAM_INDEX;
                CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                break;
            }
            default: /* AVRCP_DATA_MD_PACKET_TYPE_CONTINUE */
            {
                /* continue fragment: no attribute count in fragment, adjust attrib data offset */
                CsrBtAvrcpCtGetAttributesIndSend(pendingMsgInfo->phandle,
                                                 cfmMsg->connectionId,
                                                 cfmMsg->scope,
                                                 &cfmMsg->uid,
                                                 cfmMsg->attributeCount,
                                                 rxDataLen,
                                                 pData,
                                                 AVRCP_DATA_MD_PARAM_INDEX);
                connInst->ctLocal->pendingMsgInfo = pendingMsgInfo;

                /* We can reuse same pendingMsgInfo for new transaction
                 * Just reset the transaction timer */
                CsrBtAvrcpCtResetTransactionTimer(pendingMsgInfo,
                                                  CSR_BT_AVRCP_TIMER_MTC);
                break;
            }    
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
static void csrBtAvrcpCtGetPlayStatusRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtGetPlayStatusCfm *cfmMsg = (CsrBtAvrcpCtGetPlayStatusCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
            &cfmMsg->resultCode, &cfmMsg->resultSupplier);
        cfmMsg->playStatus      = rxData[AVRCP_DATA_PDU_GET_PLAY_STATUS_RES_PS_INDEX];
        cfmMsg->songLength      = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_GET_PLAY_STATUS_RES_SL_INDEX]);
        cfmMsg->songPosition    = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_GET_PLAY_STATUS_RES_SP_INDEX]);
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

static void csrBtAvrcpCtSetAbsoluteVolumeRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtSetVolumeCfm *cfmMsg = (CsrBtAvrcpCtSetVolumeCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_SET_VOLUME_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                            &cfmMsg->resultCode, &cfmMsg->resultSupplier);
        if (cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
        {
            cfmMsg->volume  = rxData[AVRCP_DATA_PDU_SET_VOLUME_RES_INDEX];
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
static void csrBtAvrcpCtSetAddressedPlayerRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtSetAddressedPlayerCfm *cfmMsg = (CsrBtAvrcpCtSetAddressedPlayerCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_SET_AP_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_SET_AP_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
static void csrBtAvrcpCtPlayItemRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPlayCfm *cfmMsg = (CsrBtAvrcpCtPlayCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PLAY_ITEM_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_PLAY_ITEM_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtAddToNPLRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtAddToNowPlayingCfm *cfmMsg = (CsrBtAvrcpCtAddToNowPlayingCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_ADD_TO_NPL_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_ADD_TO_NPL_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

static void csrBtAvrcpCtVendorRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    if (CsrBtAvrcpUtilDataCheckVendor(rxDataLen, rxData))
    {
        switch (rxData[AVRCP_DATA_MD_PDU_ID_INDEX])
        {
            case AVRCP_DATA_PDU_ID_GET_CAPABILITIES:
            {
                csrBtAvrcpCtGetCapRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
            case AVRCP_DATA_PDU_ID_LIST_PAS_ATTRIBUTES:
            {
                csrBtAvrcpCtPasListAttRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_LIST_PAS_VALUES:
            {
                csrBtAvrcpCtPasListValRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_GET_CUR_PAS_VALUES:
            {
                csrBtAvrcpCtPasGetCurrValRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_SET_PAS_VALUES:
            {
                csrBtAvrcpCtPasSetValueRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT:
            {
                csrBtAvrcpCtPasGetAttTextRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT:
            {
                csrBtAvrcpCtPasGetValTextRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_INFO_DISP_CHAR_SET:
            {
                csrBtAvrcpCtDispCharSetRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_INFO_BAT_STAT_OF_CT:
            {
                csrBtAvrcpCtInformBatteryStatusOfCtRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
            case AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES:
            {
                csrBtAvrcpCtGetElemAttRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
            case AVRCP_DATA_PDU_ID_GET_PLAY_STATUS:
            {
                csrBtAvrcpCtGetPlayStatusRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

            case AVRCP_DATA_PDU_ID_REG_NOTI:
            {
                csrBtAvrcpCtRegisterNotificationRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

            case AVRCP_DATA_PDU_ID_ABORT_CONTINUING_RES:
            {
                CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                break;
            }

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
            case AVRCP_DATA_PDU_ID_SET_ADDRESSED_PLAYER:
            {
                csrBtAvrcpCtSetAddressedPlayerRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

            case AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME:
            {
                csrBtAvrcpCtSetAbsoluteVolumeRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
            case AVRCP_DATA_PDU_ID_ADD_TO_NOW_PLAYING:
            {
                csrBtAvrcpCtAddToNPLRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
            case AVRCP_DATA_PDU_ID_PLAY_ITEM:
            {
                csrBtAvrcpCtPlayItemRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                break;
            }
#endif

            default:
            {
                break;
            }
        }
    }
    else
    {/* AVRCP message incorrectly formatted */
        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
CsrBool CsrBtAvrcpCtPasListAttCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrBtAvrcpDataSimpleVendorFrameSend(connInst,
                                            connInst->control.ctTLabel,
                                            AVRCP_DATA_AVCTP_CR_CMD, /* Command/response */
                                            AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                            AVRCP_DATA_PDU_ID_LIST_PAS_ATTRIBUTES); /* PDU ID */
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtPasListValCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrBtAvrcpPasAttId attId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_PAS_LIST_VAL_CMD_HEADER_SIZE);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_LIST_PAS_VALUES, /* PDU ID */
                                       AVRCP_DATA_PDU_PAS_LIST_VAL_CMD_HEADER_SIZE); /* MD parameter length */

        txData[AVRCP_DATA_PDU_PAS_LIST_VAL_CMD_ATT_ID_INDEX] = attId;

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_LIST_VAL_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtPasGetCurrentValCmdSend(AvrcpConnInstance_t *connInst,
                                            AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                            CsrUint8 attIdCount,
                                            CsrBtAvrcpPasAttId *attId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_PAS_GET_CURR_VAL_CMD_HEADER_SIZE
                        + attIdCount);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_GET_CUR_PAS_VALUES, /* PDU ID */
                                       (CsrUint16) (AVRCP_DATA_PDU_PAS_GET_CURR_VAL_CMD_HEADER_SIZE + attIdCount)); /* MD parameter length */

        txData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_CMD_NUM_INDEX] = attIdCount;
        SynMemCpyS(&txData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_CMD_ATT_ID_INDEX],
                  attIdCount,
                  attId,
                  attIdCount);

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_GET_CURR_VAL_CMD_HEADER_SIZE
                                               + attIdCount),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtPasSetValCmdSend(AvrcpConnInstance_t *connInst,
                                     AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                     CsrUint8 changedPasCount,
                                     CsrBtAvrcpPasAttValPair *changedPas)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {
        CsrUint16 additionalSize = AVRCP_DATA_PDU_PAS_SET_VAL_CMD_HEADER_SIZE
                        + changedPasCount * AVRCP_DATA_PDU_PAS_SET_VAL_CMD_PART_SIZE;
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + additionalSize);
        CsrUintFast8 i;

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_CONTROL, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_SET_PAS_VALUES, /* PDU ID */
                                       additionalSize); /* MD parameter length */

        txData[AVRCP_DATA_PDU_PAS_SET_VAL_CMD_NUM_INDEX] = changedPasCount;

        /* Insert the changed PAS att. IDs and value IDs */
        for (i = 0; i < changedPasCount; i++)
        {
            txData[AVRCP_DATA_PDU_PAS_SET_VAL_CMD_ATT_ID_INDEX + i
                            * AVRCP_DATA_PDU_PAS_SET_VAL_CMD_PART_SIZE] = changedPas[i].attribId;
            txData[AVRCP_DATA_PDU_PAS_SET_VAL_CMD_VAL_ID_INDEX + i
                            * AVRCP_DATA_PDU_PAS_SET_VAL_CMD_PART_SIZE] = changedPas[i].valueId;
        }

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE + additionalSize),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtPasGetAttTextCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                         CsrUint8 attIdCount,
                                         CsrBtAvrcpPasAttId *attId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_PAS_GET_ATT_TXT_CMD_HEADER_SIZE
                        + attIdCount);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT, /* PDU ID */
                                       (CsrUint16) (AVRCP_DATA_PDU_PAS_GET_ATT_TXT_CMD_HEADER_SIZE + attIdCount)); /* MD parameter length */

        txData[AVRCP_DATA_PDU_PAS_GET_ATT_TXT_CMD_NUM_INDEX] = attIdCount;
        SynMemCpyS(&txData[AVRCP_DATA_PDU_PAS_GET_ATT_TXT_CMD_ATT_ID_INDEX],
                  attIdCount,
                  attId,
                  attIdCount);

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_GET_ATT_TXT_CMD_HEADER_SIZE
                                               + attIdCount),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtPasGetValTextCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                         CsrBtAvrcpPasAttId attId,
                                         CsrUint8 valIdCount,
                                         CsrBtAvrcpPasValId *valId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {

        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_HEADER_SIZE
                        + valIdCount);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT, /* PDU ID */
                                       (CsrUint16) (AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_HEADER_SIZE + valIdCount)); /* MD parameter length */

        txData[AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_ATT_ID_INDEX] = attId;
        txData[AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_NUM_INDEX] = valIdCount;
        SynMemCpyS(&txData[AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_VAL_ID_INDEX],
                  valIdCount,
                  valId,
                  valIdCount);

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PAS_GET_VAL_TXT_CMD_HEADER_SIZE
                                               + valIdCount),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtInformDispCharsetCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrUint8 charsetCount,
                                             CsrBtAvrcpCharSet *charset)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {

        CsrUint8 *txData;
        CsrUintFast8 i;
        CsrUint16 len = (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_DISP_CSET_CMD_HEADER_SIZE
                        + CSR_BT_AVRCP_CHARACTER_SET_SIZE * charsetCount);

        txData = CsrPmemAlloc(len);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_CONTROL, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_INFO_DISP_CHAR_SET,/* PDU ID */
                                       (CsrUint16) (len - AVRCP_DATA_MD_HEADER_SIZE)); /* MD parameter length */

        txData[AVRCP_DATA_PDU_DISP_CSET_CMD_NUM_INDEX] = charsetCount;

        for (i = 0; i < charsetCount; i++)
        {
            CSR_COPY_UINT16_TO_BIG_ENDIAN(charset[i],
                                          &txData[AVRCP_DATA_PDU_DISP_CSET_CMD_CSET_INDEX + i * CSR_BT_AVRCP_CHARACTER_SET_SIZE]);
        }

        CsrBtAvrcpControlDataSend(connInst, len, txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

void CsrBtAvrcpCtRequestAbortContinuationCmdSend(AvrcpConnInstance_t *connInst,
                                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                    CsrBool proceed,
                                                    CsrUint8 pduId)
{
    CsrUint8 *txData;
    if (pendingMsgInfo)
    {
        /* We are going to use same transaction label.
         * Just reset the transaction timer */
        CsrBtAvrcpCtResetTransactionTimer(pendingMsgInfo,
                                          CSR_BT_AVRCP_TIMER_MTC);

        /* Update result code.... */
        switch (pduId)
        {
            case AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT:
            {
                CsrBtAvrcpCtPasAttTxtCfm *cfmMsg = (CsrBtAvrcpCtPasAttTxtCfm *) pendingMsgInfo->cfmMsg;
                cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_UNSPECIFIED_ERROR;
                cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                break;
            }

            case AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT:
            {
                CsrBtAvrcpCtPasValTxtCfm *cfmMsg = (CsrBtAvrcpCtPasValTxtCfm *) pendingMsgInfo->cfmMsg;
                cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_UNSPECIFIED_ERROR;
                cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                break;
            }

            case AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES:
            {
                CsrBtAvrcpCtGetAttributesCfm *cfmMsg = (CsrBtAvrcpCtGetAttributesCfm *) pendingMsgInfo->cfmMsg;
                cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_UNSPECIFIED_ERROR;
                cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
                break;
            }
        }
    }

    txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                          + AVRCP_DATA_PDU_CONTINUING_CMD_HEADER_SIZE);

    CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                   connInst->control.ctTLabel, /* Transaction label */
                                   AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                   AVRCP_DATA_AVC_CTYPE_CONTROL, /* Command/response type */
                                   (CsrUint8) (proceed ?
                                                   AVRCP_DATA_PDU_ID_REQ_CONTINUING_RES :
                                                   AVRCP_DATA_PDU_ID_ABORT_CONTINUING_RES),/* PDU ID */
                                   AVRCP_DATA_PDU_CONTINUING_CMD_HEADER_SIZE); /* MD parameter length */

    txData[AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ID_INDEX] = pduId; /* Either continue or response the handling of this PDU ID */

    CsrBtAvrcpControlDataSend(connInst,
    AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_CONTINUING_CMD_HEADER_SIZE,
                              txData);
}

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
CsrBool CsrBtAvrcpCtInformBatterySend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrBtAvrcpBatteryStatus batStatus)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_BAT_STATUS_CMD_HEADER_SIZE);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_CONTROL, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_INFO_BAT_STAT_OF_CT,/* PDU ID */
                                       1); /* MD parameter length */
        txData[AVRCP_DATA_PDU_BAT_STATUS_INDEX] = batStatus;
        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_BAT_STATUS_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void csrBtAvrcpCtPasListAttRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasAttIdCfm *cfmMsg = (CsrBtAvrcpCtPasAttIdCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
            &cfmMsg->resultCode, &cfmMsg->resultSupplier);

        if (cfmMsg->resultSupplier == CSR_BT_SUPPLIER_AVRCP &&
            cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
        {
            cfmMsg->attIdCount   = rxData[AVRCP_DATA_PDU_PAS_LIST_ATT_RES_NUM_INDEX];
            cfmMsg->attId        = CsrMemDup(&rxData[AVRCP_DATA_PDU_PAS_LIST_ATT_RES_ATT_ID_INDEX], cfmMsg->attIdCount);
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtPasListValRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasValIdCfm *cfmMsg = (CsrBtAvrcpCtPasValIdCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
            &cfmMsg->resultCode, &cfmMsg->resultSupplier);

        if (cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
        {
            cfmMsg->valIdCount  = rxData[AVRCP_DATA_PDU_PAS_LIST_VAL_RES_NUM_INDEX];
            cfmMsg->valId       = CsrMemDup(&rxData[AVRCP_DATA_PDU_PAS_LIST_VAL_RES_VAL_ID_INDEX], cfmMsg->valIdCount);
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtPasGetCurrValRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasCurrentCfm *cfmMsg = (CsrBtAvrcpCtPasCurrentCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrUintFast8 i;

        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
            &cfmMsg->resultCode, &cfmMsg->resultSupplier);

        if (cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
        {
            cfmMsg->attValPairCount    = rxData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_NUM_INDEX];
            cfmMsg->attValPair         = CsrPmemAlloc(cfmMsg->attValPairCount * sizeof(CsrBtAvrcpPasAttValPair));
        }

        /* Insert the attributes and corresponding values */
        for (i = 0; i < cfmMsg->attValPairCount; i++)
        {
            cfmMsg->attValPair[i].attribId = rxData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_ATT_ID_INDEX + i * AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_PART_SIZE];
            cfmMsg->attValPair[i].valueId = rxData[AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_VAL_ID_INDEX + i * AVRCP_DATA_PDU_PAS_GET_CURR_VAL_RES_PART_SIZE];
        }
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtPasSetValueRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasSetCfm *cfmMsg = (CsrBtAvrcpCtPasSetCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
            &cfmMsg->resultCode, &cfmMsg->resultSupplier);
    }
    else
    {
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtPasGetAttTextRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasAttTxtCfm *cfmMsg = (CsrBtAvrcpCtPasAttTxtCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        if ((AVRCP_DATA_MD_PACKET_TYPE_GET(rxData) == AVRCP_DATA_MD_PACKET_TYPE_SINGLE) ||
            (AVRCP_DATA_MD_PACKET_TYPE_GET(rxData) == AVRCP_DATA_MD_PACKET_TYPE_END))
        {/* Procedure is complete - send confirmation */
            CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                &cfmMsg->resultCode, &cfmMsg->resultSupplier);

            if (cfmMsg->resultSupplier == CSR_BT_SUPPLIER_AVRCP &&
                cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
            {
                cfmMsg->pasDataLen = rxDataLen;
                cfmMsg->pasData = CsrMemDup(rxData, rxDataLen);
            }

            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
        else
        {/* Start or continue packet */
            CsrUint8 *pData = CsrMemDup(rxData, rxDataLen);
            connInst->ctLocal->pendingMsgInfo = pendingMsgInfo;
            CsrBtAvrcpCtPasAttTxtIndSend(connInst->appConnId, rxDataLen, pData, pendingMsgInfo->phandle);
        }
    }
    else
    {/* Message incorrectly formatted */
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}

static void csrBtAvrcpCtPasGetValTextRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtPasValTxtCfm *cfmMsg = (CsrBtAvrcpCtPasValTxtCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen >= AVRCP_DATA_MD_HEADER_SIZE)
    {
        if ((AVRCP_DATA_MD_PACKET_TYPE_GET(rxData) == AVRCP_DATA_MD_PACKET_TYPE_SINGLE) ||
            (AVRCP_DATA_MD_PACKET_TYPE_GET(rxData) == AVRCP_DATA_MD_PACKET_TYPE_END))
        {/* Procedure is complete - send confirmation */
            CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
                &cfmMsg->resultCode, &cfmMsg->resultSupplier);

            if (cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)
            {
                cfmMsg->pasDataLen = rxDataLen;
                cfmMsg->pasData = CsrMemDup(rxData, rxDataLen);
            }

            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
        else
        {/* Start or continue packet */
            CsrUint8 *pData = CsrMemDup(rxData, rxDataLen);
            connInst->ctLocal->pendingMsgInfo = pendingMsgInfo;
            CsrBtAvrcpCtPasValTxtIndSend(connInst->appConnId, cfmMsg->attId, rxDataLen, pData, pendingMsgInfo->phandle);
        }
    }
    else
    {/* Message incorrectly formatted */
        cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
        cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;

        CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    }
}

static void csrBtAvrcpCtDispCharSetRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtInformDispCharsetCfm *cfmMsg = (CsrBtAvrcpCtInformDispCharsetCfm *)pendingMsgInfo->cfmMsg;

    CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
        &cfmMsg->resultCode, &cfmMsg->resultSupplier);
    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
    CSR_UNUSED(rxDataLen);
}

static void csrBtAvrcpCtInformBatteryStatusOfCtRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtInformBatteryStatusCfm *cfmMsg = (CsrBtAvrcpCtInformBatteryStatusCfm *)pendingMsgInfo->cfmMsg;

    CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(rxData),
        &cfmMsg->resultCode, &cfmMsg->resultSupplier);
    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
    CSR_UNUSED(rxDataLen);
}
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
CsrBool CsrBtAvrcpCtSetBrowsedPlayerCmdSend(AvrcpConnInstance_t *connInst,
                                            AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                            CsrUint16 playerId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE
                        + AVRCP_DATA_PDU_SET_BP_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                           pendingMsgInfo->tLabel,
                                           AVRCP_DATA_AVCTP_CR_CMD,
                                           AVRCP_DATA_PDU_ID_SET_BROWSED_PLAYER,
                                           AVRCP_DATA_PDU_SET_BP_CMD_HEADER_SIZE);

        CSR_COPY_UINT16_TO_BIG_ENDIAN(playerId,
                                      &txData[AVRCP_DATA_PDU_SET_BP_CMD_PID_INDEX]);

        CsrBtAvrcpBrowsingDataSend(connInst,
                                   AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_SET_BP_CMD_HEADER_SIZE,
                                   txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtGetFolderItemsCmdSend(AvrcpConnInstance_t *connInst,
                                          AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                          CsrBtAvrcpScope scope,
                                          CsrUint32 startItem,
                                          CsrUint32 endItem,
                                          CsrBtAvrcpItemAttMask attributeMask)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING))
    {
        CsrUint8 attributeCount, attributeIndex, *txData;
        CsrUintFast8 i;

        if (scope == CSR_BT_AVRCP_SCOPE_MP_LIST)
        {
            /* no media attributes apply for scope media player list */
            attributeMask = CSR_BT_AVRCP_ITEM_ATT_MASK_NONE;
        }

        if ((attributeMask == CSR_BT_AVRCP_ITEM_ATT_MASK_NONE)
            || (attributeMask == CSR_BT_AVRCP_ITEM_ATT_MASK_ALL))
        {
            /* if no or all attributes are requested, no list is included */
            attributeCount = 0;
        }
        else
        {
            attributeCount = CsrBitCountSparse(attributeMask)
                            * sizeof(CsrBtAvrcpItemAttMask); /* Each attribute Id fills 4 bytes */
        }

        txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE
                        + AVRCP_DATA_PDU_GFI_CMD_MIN_HEADER_SIZE
                        + attributeCount  - 1);

        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                           pendingMsgInfo->tLabel,
                                           AVRCP_DATA_AVCTP_CR_CMD,
                                           AVRCP_DATA_PDU_ID_GET_FOLDER_ITEMS,
                                           (CsrUint16) (AVRCP_DATA_PDU_GFI_CMD_MIN_HEADER_SIZE + attributeCount - 1));

        txData[AVRCP_DATA_PDU_GFI_CMD_SCOPE_INDEX] = scope;
        txData[AVRCP_DATA_PDU_GFI_CMD_ATT_COUNT_INDEX] = (attributeMask == CSR_BT_AVRCP_ITEM_ATT_MASK_NONE) ?
                                        0xFF : attributeCount / sizeof(CsrBtAvrcpItemAttMask);
        CSR_COPY_UINT32_TO_BIG_ENDIAN(startItem,
                                      &txData[AVRCP_DATA_PDU_GFI_CMD_START_INDEX]);
        CSR_COPY_UINT32_TO_BIG_ENDIAN(endItem,
                                      &txData[AVRCP_DATA_PDU_GFI_CMD_END_INDEX]);

        /* Insert the attribute IDs */
        for (i = 0, attributeIndex = 0; attributeIndex < attributeCount; i++)
        {
            if (CSR_BIT_IS_SET(attributeMask, i))
            {
                CSR_COPY_UINT32_TO_BIG_ENDIAN(i + CSR_BT_AVRCP_ITEM_ATT_OFFSET_FROM_MASK,
                                              &txData[AVRCP_DATA_PDU_GFI_CMD_ATT_LIST_INDEX + attributeIndex]);
                attributeIndex += sizeof(CsrBtAvrcpItemAttMask);
            }
        }

        CsrBtAvrcpBrowsingDataSend(connInst,
                                   (CsrUint16) (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GFI_CMD_MIN_HEADER_SIZE
                                                + attributeCount - 1),
                                   txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtChangePathCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrUint16 uidCount,
                                      CsrBtAvrcpFolderDirection direction,
                                      CsrBtAvrcpUid *folderUid)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE
                        + AVRCP_DATA_PDU_CHANGE_PATH_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                           pendingMsgInfo->tLabel,
                                           AVRCP_DATA_AVCTP_CR_CMD,
                                           AVRCP_DATA_PDU_ID_CHANGE_PATH,
                                           AVRCP_DATA_PDU_CHANGE_PATH_CMD_HEADER_SIZE);

        txData[AVRCP_DATA_PDU_CHANGE_PATH_CMD_DIRECTION_INDEX] = direction;
        CSR_COPY_UINT16_TO_BIG_ENDIAN(uidCount,
                                      &txData[AVRCP_DATA_PDU_CHANGE_PATH_CMD_UID_COUNT_INDEX]);
        CSR_BT_AVRCP_UID_COPY(&txData[AVRCP_DATA_PDU_CHANGE_PATH_CMD_FOLDER_UID_INDEX],
                              *folderUid);

        CsrBtAvrcpBrowsingDataSend(connInst,
                                   AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_CHANGE_PATH_CMD_HEADER_SIZE,
                                   txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}



CsrBool CsrBtAvrcpCtGetItemAttributesCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrBtAvrcpScope scope,
                                             CsrUint16 uidCount,
                                             CsrBtAvrcpUid *itemUid,
                                             CsrBtAvrcpItemAttMask attributeMask)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING))
    {
        CsrUint8 attributeCount = CsrBitCountSparse(attributeMask);
        CsrUint16 paramLen = AVRCP_DATA_PDU_GIA_CMD_MIN_HEADER_SIZE
                        + attributeCount * AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ATT_SIZE;
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE
                        + paramLen);
        CsrUintFast8 i;
        CsrUint8 attributeIndex;

        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                           pendingMsgInfo->tLabel,
                                           AVRCP_DATA_AVCTP_CR_CMD,
                                           AVRCP_DATA_PDU_ID_GET_ITEM_ATTRIBUTES,
                                           paramLen);

        txData[AVRCP_DATA_PDU_GIA_CMD_SCOPE_INDEX] = scope;
        txData[AVRCP_DATA_PDU_GIA_CMD_ATTRIB_COUNT_INDEX] = attributeCount;
        CSR_COPY_UINT16_TO_BIG_ENDIAN(uidCount,
                                      &txData[AVRCP_DATA_PDU_GIA_CMD_UID_COUNT_INDEX]);
        CSR_BT_AVRCP_UID_COPY(&txData[AVRCP_DATA_PDU_GIA_CMD_UID_INDEX],
                              *itemUid);

        /* Insert the attribute IDs */
        for (i = 0, attributeIndex = 0; attributeIndex < attributeCount; i++)
        {
            if (CSR_BIT_IS_SET(attributeMask, i))
            {
                CSR_COPY_UINT32_TO_BIG_ENDIAN(i + CSR_BT_AVRCP_ITEM_ATT_OFFSET_FROM_MASK,
                                              &txData[AVRCP_DATA_PDU_GIA_CMD_ATTRIB_INDEX + attributeIndex * AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ATT_SIZE]);
                attributeIndex++;
            }
        }

        CsrBtAvrcpBrowsingDataSend(connInst,
                                   (CsrUint16) (AVRCP_DATA_BROWSING_HEADER_SIZE + paramLen),
                                   txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtSearchCmdSend(AvrcpConnInstance_t *connInst,
                                  AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                  CsrBtAvrcpCharSet charset,
                                  CsrUtf8String *text)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING_SEARCH))
    {
        CsrUint16 textLen = (CsrUint16) CsrStrLen((char*) text);
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE
                        + AVRCP_DATA_PDU_SEARCH_CMD_MIN_HEADER_SIZE + textLen);

        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                           pendingMsgInfo->tLabel,
                                           AVRCP_DATA_AVCTP_CR_CMD,
                                           AVRCP_DATA_PDU_ID_SEARCH,
                                           (CsrUint16) (AVRCP_DATA_PDU_SEARCH_CMD_MIN_HEADER_SIZE + textLen));

        CSR_COPY_UINT16_TO_BIG_ENDIAN(charset,
                                      &txData[AVRCP_DATA_PDU_SEARCH_CMD_CHARSET_INDEX]);
        CSR_COPY_UINT16_TO_BIG_ENDIAN(textLen,
                                      &txData[AVRCP_DATA_PDU_SEARCH_CMD_LEN_INDEX]);
        SynMemCpyS(&txData[AVRCP_DATA_PDU_SEARCH_CMD_SEARCH_TXT_INDEX],
                  textLen,
                  text,
                  textLen);

        CsrBtAvrcpBrowsingDataSend(connInst,
                                   (CsrUint16) (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_SEARCH_CMD_MIN_HEADER_SIZE
                                                + textLen),
                                   txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtGetTotalNumberOfItemsCmdSend(AvrcpConnInstance_t *connInst,
                                                           AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                           CsrBtAvrcpScope scope)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->browsing,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_BROWSING))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GTNI_CMD_HEADER_SIZE);


        CsrBtAvrcpDataInsertBrowsingHeader(txData,
                                pendingMsgInfo->tLabel,
                                AVRCP_DATA_AVCTP_CR_CMD,
                                AVRCP_DATA_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS,
                                AVRCP_DATA_PDU_GTNI_CMD_HEADER_SIZE);
        txData[AVRCP_DATA_PDU_GTNI_CMD_SCOPE_INDEX] = scope;

        CsrBtAvrcpBrowsingDataSend(connInst,
                                  AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GTNI_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void csrBtAvrcpCtSetBrowsedPlayerRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtSetBrowsedPlayerCfm *cfmMsg = (CsrBtAvrcpCtSetBrowsedPlayerCfm *)pendingMsgInfo->cfmMsg;

    if ((rxDataLen >= (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_SET_BP_RES_MIN_HEADER_SIZE)))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_SET_BP_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
        cfmMsg->itemsCount      = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_SET_BP_RES_NUMBER_OF_ITEMS_INDEX]);
        cfmMsg->uidCounter      = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_SET_BP_RES_UID_COUNT_INDEX]);
        cfmMsg->charsetId       = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_SET_BP_RES_CHARSET_INDEX]);
        cfmMsg->folderDepth     = rxData[AVRCP_DATA_PDU_SET_BP_RES_FOLDER_DEPTH_INDEX];
        if (cfmMsg->folderDepth > 0)
        {/* Convert to single directory path */
            cfmMsg->folderNamesLen  = CsrBtAvrcpCtSBPFolderNamesGet(&cfmMsg->folderNames,&rxData[AVRCP_DATA_PDU_SET_BP_RES_FOLDER_PAIRS_INDEX],cfmMsg->folderDepth);
        }
        else
        {
            cfmMsg->folderNamesLen = 0;
            cfmMsg->folderNames    = NULL;
        }
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_STATUS_HEADER_SIZE))
        {/* This is a normal rejection from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_SET_BP_RES_STATUS_INDEX],
                                                   &cfmMsg->resultCode,
                                                   &cfmMsg->resultSupplier);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtGetFolderItemsRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 **rxData)
{
    CsrBtAvrcpCtGetFolderItemsCfm *cfmMsg = (CsrBtAvrcpCtGetFolderItemsCfm *)pendingMsgInfo->cfmMsg;

    if ((rxDataLen >= (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GFI_RES_MIN_HEADER_SIZE)))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus((*rxData)[AVRCP_DATA_PDU_GFI_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
        cfmMsg->uidCounter      = CSR_GET_UINT16_FROM_BIG_ENDIAN(&(*rxData)[AVRCP_DATA_PDU_GFI_RES_UID_COUNT_INDEX]);
        cfmMsg->itemsCount      = CSR_GET_UINT16_FROM_BIG_ENDIAN(&(*rxData)[AVRCP_DATA_PDU_GFI_RES_NUM_OF_ITEMS_INDEX]);
        cfmMsg->itemsDataLen    = rxDataLen;
        cfmMsg->itemsData       = *rxData;
        *rxData = NULL;
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_STATUS_HEADER_SIZE))
        {/* This is a normal rejection from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus((*rxData)[AVRCP_DATA_PDU_GFI_RES_STATUS_INDEX],
                                                   &cfmMsg->resultCode,
                                                   &cfmMsg->resultSupplier);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtChangePathRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtChangePathCfm *cfmMsg = (CsrBtAvrcpCtChangePathCfm *)pendingMsgInfo->cfmMsg;
    
    cfmMsg->itemsCount = 0;
    
    if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_CHANGE_PATH_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_CHANGE_PATH_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
        cfmMsg->itemsCount      = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_CHANGE_PATH_RES_NUM_OF_ITEMS_INDEX]);
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_STATUS_HEADER_SIZE))
        {/* This is a normal rejection from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_CHANGE_PATH_RES_STATUS_INDEX],
                                                   &cfmMsg->resultCode,
                                                   &cfmMsg->resultSupplier);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
    /* For the Media player which is database unware player 
        disconnect/connect back the OBEX service connection so that
        image handles could be re-generated by remote device. */
    if ((!pendingMsgInfo->uidCounter) && 
        (cfmMsg->resultSupplier == CSR_BT_SUPPLIER_AVRCP) && 
        (cfmMsg->resultCode == CSR_BT_RESULT_CODE_AVRCP_SUCCESS)) 
    {
        /* Only disconnect the service level connection 
            only if not already service disconnected */
        if (connInst->ctLocal->ctObexState == AVRCP_CT_OBEX_SERVICE_CONNECTED)
        {
            CsrBtAvrcpImagingClientDisconnectReqSend(connInst->appConnId, TRUE);
        }
    }
#else
    CSR_UNUSED(connInst);
#endif

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
}

static void csrBtAvrcpCtGetItemAttributeRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 **rxData)
{
    CsrBtAvrcpCtGetAttributesCfm *cfmMsg = (CsrBtAvrcpCtGetAttributesCfm *)pendingMsgInfo->cfmMsg;

    if ((rxDataLen >= (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GIA_RES_MIN_HEADER_SIZE)))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus((*rxData)[AVRCP_DATA_PDU_GIA_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
        cfmMsg->attributeCount  = (*rxData)[AVRCP_DATA_PDU_GIA_RES_NOA_INDEX];
        cfmMsg->attribDataLen   = rxDataLen;
        cfmMsg->attribData      = *rxData;
        *rxData = NULL;
        cfmMsg->attribDataPayloadOffset = CSR_BT_AVRCP_LIB_GIA_HEADER_OFFSET;
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_STATUS_HEADER_SIZE))
        {/* This is a normal rejection from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus((*rxData)[AVRCP_DATA_PDU_GIA_RES_STATUS_INDEX],
                                                   &cfmMsg->resultCode,
                                                   &cfmMsg->resultSupplier);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtSearchRspHandler(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 rxDataLen, CsrUint8 *rxData)
{
    CsrBtAvrcpCtSearchCfm *cfmMsg = (CsrBtAvrcpCtSearchCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_SEARCH_RES_HEADER_SIZE))
    {
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_SEARCH_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode,
                                               &cfmMsg->resultSupplier);
        cfmMsg->numberOfItems   = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_SEARCH_RES_ITEM_COUNT_INDEX]);
        cfmMsg->uidCounter      = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_SEARCH_RES_UID_COUNT_INDEX]);
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_STATUS_HEADER_SIZE))
        {/* This is a normal rejection from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_SEARCH_RES_STATUS_INDEX],
                                                   &cfmMsg->resultCode,
                                                   &cfmMsg->resultSupplier);
        }
        else
        {
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

static void csrBtAvrcpCtGetTotalNumberOfItemsRspHandler(AvrcpConnInstance_t *connInst,
                                                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                                    CsrUint16 rxDataLen,
                                                                    CsrUint8 *rxData)
{
    CsrBtAvrcpCtGetTotalNumberOfItemsCfm *cfmMsg = (CsrBtAvrcpCtGetTotalNumberOfItemsCfm *)pendingMsgInfo->cfmMsg;

    if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GTNI_RES_HEADER_SIZE))
    { /* Success */
        CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_GTNI_RES_STATUS_INDEX],
                                               &cfmMsg->resultCode, 
                                               &cfmMsg->resultSupplier);
        cfmMsg->uidCounter = CSR_GET_UINT16_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_GTNI_RES_UID_COUNT_INDEX]);
        cfmMsg->noOfItems  = CSR_GET_UINT32_FROM_BIG_ENDIAN(&rxData[AVRCP_DATA_PDU_GTNI_RES_NUM_OF_ITEMS_INDEX]);
    }
    else
    {
        if (rxDataLen == (AVRCP_DATA_BROWSING_HEADER_SIZE + AVRCP_DATA_PDU_GTNI_RES_MIN_HEADER_SIZE))
        {/* Error response from the TG */
            CsrBtAvrcpCtUtilConvertOperationStatus(rxData[AVRCP_DATA_PDU_GTNI_RES_STATUS_INDEX],
                                                    &cfmMsg->resultCode,
                                                    &cfmMsg->resultSupplier);
        }
        else
        {/* incorrect sized pdu */
            cfmMsg->resultCode = CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE;
            cfmMsg->resultSupplier = CSR_BT_SUPPLIER_AVRCP;
        }
    }

    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
    CSR_UNUSED(connInst);
}

void CsrBtAvrcpCtRxBrowsingHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo;

    pendingMsgInfo = CsrBtAvrcpCtUtilMsgGetTLabel((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList,
                                                  AVRCP_TLABEL_GET(*rxData),
                                                  CSR_BT_AVCTP_BROWSING_PSM);

    if (pendingMsgInfo &&
        csrBtAvrcpCheckRspAgainstPendingMsg(pendingMsgInfo, AVRCP_DATA_BROWSING_PDU_ID_GET(*rxData), *rxData))
    {
        if (CsrBtAvrcpUtilDataCheckBrowsing(rxDataLen, *rxData))
        {
            switch (AVRCP_DATA_BROWSING_PDU_ID_GET(*rxData))
            {
                case AVRCP_DATA_PDU_ID_SET_ABSOLUTE_VOLUME:
                {
                    csrBtAvrcpCtSetAbsoluteVolumeRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_SET_BROWSED_PLAYER:
                {
                    csrBtAvrcpCtSetBrowsedPlayerRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS:
                {
                    csrBtAvrcpCtGetTotalNumberOfItemsRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_GET_FOLDER_ITEMS:
                {
                    csrBtAvrcpCtGetFolderItemsRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_CHANGE_PATH:
                {
                    csrBtAvrcpCtChangePathRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_GET_ITEM_ATTRIBUTES:
                {
                    csrBtAvrcpCtGetItemAttributeRspHandler(connInst, pendingMsgInfo, rxDataLen, rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_SEARCH:
                {
                    csrBtAvrcpCtSearchRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }

                case AVRCP_DATA_PDU_ID_GENERAL_REJECT:
                {/* Find out what message is pending and what message should be sent to the application */
                    CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
                        (*rxData)[AVRCP_DATA_PDU_GENERAL_REJECT_RES_REASON_INDEX],
                        CSR_BT_SUPPLIER_AVCTP);
                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                    break;
                }

                default:
                {
                    CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
                        CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER,
                        CSR_BT_SUPPLIER_AVRCP);
                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                    break;
                }
            }
        }
        else
        {
            CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
                CSR_BT_RESULT_CODE_AVRCP_RSP_INCORRECT_SIZE,
                CSR_BT_SUPPLIER_AVRCP);
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
CsrBool CsrBtAvrcpCtGetPlayStatusCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_GET_PLAY_STATUS_CMD_HEADER_SIZE);

        CsrBtAvrcpDataVendorDataInsert(txData, /* Data to transmit */
                                       connInst->control.ctTLabel, /* Transaction label */
                                       AVRCP_DATA_AVCTP_CR_CMD, /* Command or response */
                                       AVRCP_DATA_AVC_CTYPE_STATUS, /* Command/response type */
                                       AVRCP_DATA_PDU_ID_GET_PLAY_STATUS, /* PDU ID */
                                       AVRCP_DATA_PDU_GET_PLAY_STATUS_CMD_HEADER_SIZE); /* MD parameter length */

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_GET_PLAY_STATUS_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
CsrBool CsrBtAvrcpCtSetAddressedPlayerCmdSend(AvrcpConnInstance_t *connInst,
                                              AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                              CsrUint16 playerId)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_SET_AP_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertControlHeader(txData,
                                          pendingMsgInfo->tLabel,
                                          AVRCP_DATA_AVCTP_CR_CMD,
                                          AVRCP_DATA_AVC_CTYPE_CONTROL,
                                          AVRCP_DATA_AVC_SUBUNIT_TYPE_PANEL,
                                          0x00, /* Subunit ID*/
                                          AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT,
                                          AVRCP_DATA_PDU_ID_SET_ADDRESSED_PLAYER,
                                          AVRCP_DATA_PDU_SET_AP_CMD_HEADER_SIZE);

        CSR_COPY_UINT16_TO_BIG_ENDIAN(playerId,
                                      &txData[AVRCP_DATA_PDU_SET_AP_CMD_PID_INDEX]);

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_SET_AP_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
CsrBool CsrBtAvrcpCtGetElementAttributesCmdSend(AvrcpConnInstance_t *connInst,
                                                AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                CsrUint32 attributeMask)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTP))
    {
        CsrUint8 attributeCount = CsrBitCountSparse(attributeMask);
        CsrUint16 additionalSize = AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_HEADER_SIZE
                        + attributeCount * AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ATT_SIZE;
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + additionalSize);
        CsrUintFast8 i;

        CsrBtAvrcpDataInsertAvctpHeader(txData,
                                        connInst->control.ctTLabel, /* Transaction label */
                                        AVRCP_DATA_AVCTP_CR_CMD); /* Command or response */

        CsrBtAvrcpDataInsertAvcCommonHeader(txData,
                                            AVRCP_DATA_AVC_CTYPE_STATUS);

        CsrBtAvrcpDataInsertAvcVendorHeader(txData,
        AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES,
                                            AVRCP_DATA_MD_PACKET_TYPE_SINGLE,
                                            additionalSize);

        CsrMemSet(&txData[AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ID_INDEX],
                  0,
                  AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ID_SIZE);
        txData[AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_NUM_INDEX] = attributeCount;

        /* Insert the attribute IDs */
        for (i = 0; i < CSR_BT_AVRCP_ITEM_ATT_COUNT; i++)
        {
            if (CSR_BIT_IS_SET(attributeMask, i))
            {
                attributeCount--;
                CSR_COPY_UINT32_TO_BIG_ENDIAN(i + CSR_BT_AVRCP_ITEM_ATT_OFFSET_FROM_MASK,
                                              &txData[AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ATT_INDEX + attributeCount * AVRCP_DATA_PDU_GET_ELEM_ATT_CMD_ATT_SIZE]);
            }
        }

        CsrBtAvrcpControlDataSend(connInst,
                                  (CsrUint16) (AVRCP_DATA_MD_HEADER_SIZE + additionalSize),
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
CsrBool CsrBtAvrcpCtPlayItemCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrBtAvrcpScope scope,
                                    CsrUint16 uidCount,
                                    CsrBtAvrcpUid *itemUid)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {

        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_PLAY_ITEM_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertControlHeader(txData,
                                          pendingMsgInfo->tLabel,
                                          AVRCP_DATA_AVCTP_CR_CMD,
                                          AVRCP_DATA_AVC_CTYPE_CONTROL,
                                          AVRCP_DATA_AVC_SUBUNIT_TYPE_PANEL,
                                          0x00, /* Subunit ID*/
                                          AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT,
                                          AVRCP_DATA_PDU_ID_PLAY_ITEM,
                                          AVRCP_DATA_PDU_PLAY_ITEM_CMD_HEADER_SIZE);

        txData[AVRCP_DATA_PDU_PLAY_ITEM_CMD_SCOPE_INDEX] = scope;
        CSR_COPY_UINT16_TO_BIG_ENDIAN(uidCount,
                                      &txData[AVRCP_DATA_PDU_PLAY_ITEM_CMD_UID_COUNT_INDEX]);
        SynMemCpyS(&txData[AVRCP_DATA_PDU_PLAY_ITEM_CMD_ITEM_UID_INDEX],
                  sizeof(CsrBtAvrcpUid),
                  *itemUid,
                  sizeof(CsrBtAvrcpUid));

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_PLAY_ITEM_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

CsrBool CsrBtAvrcpCtAddToNPLCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrBtAvrcpScope scope,
                                    CsrUint16 uidCount,
                                    CsrBtAvrcpUid *itemUid)
{
    if (CsrBtAvrcpCtResetPendingMsg(&connInst->control,
                                    pendingMsgInfo,
                                    CSR_BT_AVRCP_TIMER_MTC))
    {
        CsrUint8 *txData = CsrPmemAlloc(AVRCP_DATA_MD_HEADER_SIZE
                        + AVRCP_DATA_PDU_ADD_TO_NPL_CMD_HEADER_SIZE);

        CsrBtAvrcpDataInsertControlHeader(txData,
                                          pendingMsgInfo->tLabel,
                                          AVRCP_DATA_AVCTP_CR_CMD,
                                          AVRCP_DATA_AVC_CTYPE_CONTROL,
                                          AVRCP_DATA_AVC_SUBUNIT_TYPE_PANEL,
                                          0x00, /* Subunit ID*/
                                          AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT,
                                          AVRCP_DATA_PDU_ID_ADD_TO_NOW_PLAYING,
                                          AVRCP_DATA_PDU_ADD_TO_NPL_CMD_HEADER_SIZE);

        txData[AVRCP_DATA_PDU_ADD_TO_NPL_CMD_SCOPE_INDEX] = scope;
        CSR_COPY_UINT16_TO_BIG_ENDIAN(uidCount,
                                      &txData[AVRCP_DATA_PDU_ADD_TO_NPL_CMD_UID_COUNT_INDEX]);
        SynMemCpyS(&txData[AVRCP_DATA_PDU_ADD_TO_NPL_CMD_ITEM_UID_INDEX],
                  sizeof(CsrBtAvrcpUid),
                  *itemUid,
                  sizeof(CsrBtAvrcpUid));

        CsrBtAvrcpControlDataSend(connInst,
                                  AVRCP_DATA_MD_HEADER_SIZE + AVRCP_DATA_PDU_ADD_TO_NPL_CMD_HEADER_SIZE,
                                  txData);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
#endif /* CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER */

void CsrBtAvrcpCtRxControlHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData)
{
    AvrcpCtPendingMsgInfo_t *pendingMsgInfo;

    pendingMsgInfo = CsrBtAvrcpCtUtilMsgGetTLabel((CsrCmnList_t *)&connInst->ctLocal->pendingMsgList,
                                                  AVRCP_TLABEL_GET(*rxData),
                                                  CSR_BT_AVCTP_PSM);

    if (pendingMsgInfo  &&
        csrBtAvrcpCheckRspAgainstPendingMsg(pendingMsgInfo, AVRCP_DATA_AVC_OPCODE_GET(*rxData), *rxData))
    {
        if (CsrBtAvrcpUtilDataCheckAVC(rxDataLen, *rxData))
        {
#ifdef CSR_TARGET_PRODUCT_VM
            CSR_LOG_TEXT_INFO((CsrBtAvrcpLto, 0, "CsrBtAvrcpCtRxControlHandler TLable:0x%0x, Cmd/Res:0x%0x, OpCode:0x%0x, PduId:0x%0x, BtConnId:0x%08x", 
                               AVRCP_TLABEL_GET(*rxData),
                               AVRCP_DATA_AVCTP_CR_GET(*rxData), 
                               AVRCP_DATA_AVC_OPCODE_GET(*rxData),
                               AvrcpUtilGetPduIdFromPacket(*rxData),
                               connInst->control.btConnId));
#endif
            switch (AVRCP_DATA_AVC_OPCODE_GET(*rxData))
            {
                case AVRCP_DATA_AVC_OPCODE_PASS_THROUGH:
                {
                    csrBtAvrcpCtPassThroughRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }
#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
                case AVRCP_DATA_AVC_OPCODE_VENDOR_DEPENDENT:
                {
                    csrBtAvrcpCtVendorRspHandler(connInst, pendingMsgInfo, rxDataLen, *rxData);
                    break;
                }
#endif

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
                case AVRCP_DATA_AVC_OPCODE_UNIT_INFO:
                {
                    CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(*rxData),
                        &(((CsrBtAvrcpCtUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->resultCode),
                        &(((CsrBtAvrcpCtUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->resultSupplier));

                    if (rxDataLen > AVRCP_DATA_AVC_OPCODE_INDEX)
                    {
                        CsrUint8 *pData = CsrPmemAlloc((CsrUint16)(rxDataLen-AVRCP_DATA_AVC_UNIT_RES_07_INDEX));
                        ((CsrBtAvrcpCtUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->pDataLen = (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_UNIT_RES_07_INDEX);
                        SynMemCpyS(pData,
                                   (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_UNIT_RES_07_INDEX),
                                   &(*rxData)[AVRCP_DATA_AVC_UNIT_RES_07_INDEX],
                                   (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_UNIT_RES_07_INDEX));
                        ((CsrBtAvrcpCtUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->pData    = pData;
                    }

                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                    break;
                }

                case AVRCP_DATA_AVC_OPCODE_SUBUNIT_INFO:
                {
                    CsrBtAvrcpCtUtilConvertAVCRspType(AVRCP_DATA_AVC_CRTYPE_GET(*rxData),
                        &(((CsrBtAvrcpCtSubUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->resultCode),
                        &(((CsrBtAvrcpCtSubUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->resultSupplier));

                    if (rxDataLen > AVRCP_DATA_AVC_OPCODE_INDEX)
                    {
                        CsrUint8 *pData = CsrPmemAlloc((CsrUint16)(rxDataLen-AVRCP_DATA_AVC_SUBUNIT_RES_PAGE_INDEX));
                        ((CsrBtAvrcpCtSubUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->pDataLen = (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_SUBUNIT_RES_PAGE_INDEX);
                        SynMemCpyS(pData,
                                   (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_SUBUNIT_RES_PAGE_INDEX),
                                   &(*rxData)[AVRCP_DATA_AVC_SUBUNIT_RES_PAGE_INDEX],
                                   (CsrUint16)(rxDataLen-AVRCP_DATA_AVC_SUBUNIT_RES_PAGE_INDEX));
                        ((CsrBtAvrcpCtSubUnitInfoCmdCfm *)(pendingMsgInfo->cfmMsg))->pData    = pData;
                    }

                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                    break;
                }
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */

                default:
                {/* Unknown response - ignore */
                    CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
                        CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER,
                        CSR_BT_SUPPLIER_AVRCP);
                    CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
                    break;
                }
            }
        }
        else
        {/* AV/C frame is incorrectly formatted - ignore */
            CsrBtAvrcpCtPendingMsgUpdateResult(pendingMsgInfo,
                CSR_BT_RESULT_CODE_AVRCP_INVALID_PARAMETER,
                CSR_BT_SUPPLIER_AVRCP);
            CsrBtAvrcpCtCompletePendingMsg(pendingMsgInfo);
        }
    }
#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
    else if ((CsrBtAvrcpUtilDataCheckVendor(rxDataLen, *rxData)) &&
             ((*rxData)[AVRCP_DATA_MD_PDU_ID_INDEX] == AVRCP_DATA_PDU_ID_REG_NOTI))
    {/* This is a notification */
        csrBtAvrcpCtRegisterNotificationRspHandler(connInst, NULL, rxDataLen, *rxData);
    }
#endif
}

#endif /* ! EXCLUDE_CSR_BT_AVRCP_CT_MODULE */
#endif /* ! EXCLUDE_CSR_BT_AVRCP_MODULE */

