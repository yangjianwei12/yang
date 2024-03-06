/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_callback_q.h"

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
static void csrBtCmDmSyncDisconnectHandler(cmInstanceData_t *cmData,
                                           hci_connection_handle_t handle,
                                           CsrUint8 status,
                                           CsrBtReasonCode reason)
{
    cmRfcConnElement *theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle,
                                                       &handle);

    if (theElement)
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        CsrBtCmDmSyncDisconnectRfcHandler(cmData, theElement->cmRfcConnInst, status, reason);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
    }
}

void CsrBtCmDmSyncClearPcmSlotFromTable(cmInstanceData_t *cmData,
                                        eScoParmVars *eScoParms)
{
    if (eScoParms)
    {
        CsrUint8 pcmSlot = eScoParms->pcmSlot;

        eScoParms->pcmSlot = 0;

        if (pcmSlot > 0 && pcmSlot <= MAX_PCM_SLOTS)
        {
            cmData->dmVar.pcmAllocationTable[pcmSlot-1] = NO_SCO;
        }
    }
}

CsrUint8 returnNumberOfScoConnection(cmInstanceData_t *cmData)
{
    CsrUint8 numOfSco = 0;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmRfcConnElement *currentRfcElem;

    for (currentRfcElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentRfcElem; currentRfcElem = currentRfcElem->next)
    { /* Search through the RFC connection table                                */
        if (currentRfcElem->cmRfcConnInst)
        {
            if (currentRfcElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED)
            {
                if (currentRfcElem->cmRfcConnInst->eScoParms &&
                    currentRfcElem->cmRfcConnInst->eScoParms->handle < SCOBUSY_ACCEPT)
                {
                    numOfSco++;
                }
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

    return numOfSco;
}
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

void CsrBtCmDmSyncRegisterCfmHandler(cmInstanceData_t *cmData)
{
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    /* Always require unauthenticated link key on RFCOMM PSM */
    dm_sm_service_register_req(CSR_BT_CM_IFACEQUEUE,
                               0, /* context */
                               SEC_PROTOCOL_L2CAP,
                               RFCOMM_PSM,
                               TRUE,
                               SECL4_IN_SSP | SECL4_OUT_SSP,
                               CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                               NULL);

    /* Start adding device records from persistent store to Bluestack */
    CsrBtCmSmDbAddDeviceIndex(cmData, 0);
#else /* EXCLUDE_CSR_BT_SC_MODULE */
    CsrBtCmInitCompleted(cmData);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */
}

#ifndef EXCLUDE_CSR_BT_SCO_MODULE

#ifndef EXCLUDE_CSR_BT_CHIP_SUPPORT_MAP_SCO_PCM
void CsrBtCmBccmdMapScoPcmReqSend(CsrUint8 pcmSlot, CsrUint16 seqNo)
{
    CsrUint8 *payload    = (CsrUint8 *) CsrPmemAlloc(sizeof(CsrUint16));
    CsrUint8 *tmpPayload = payload;
    CSR_ADD_UINT8_TO_XAP(tmpPayload, pcmSlot);
    CsrBccmdWriteReqSend(CSR_BT_CM_IFACEQUEUE, BCCMD_VARID_MAP_SCO_PCM, seqNo, sizeof(CsrUint16), payload);
}
#endif

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
void CsrBtCmBccmdGetScoParametersReqSend(CsrUint16 scoHandle, CsrUint16 seqNo)
{
    CsrUint8 *payload    = (CsrUint8 *) CsrPmemAlloc(sizeof(CsrUint16));
    CsrUint8 *tmpPayload = payload;
    CSR_ADD_UINT16_TO_XAP(tmpPayload, scoHandle);
    CsrBccmdReadReqSend(CSR_BT_CM_IFACEQUEUE, BCCMD_VARID_GET_SCO_PARAMETERS, seqNo, sizeof(CsrUint16), payload);
}
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */

CsrBool CsrBtCmDmIsEdrEsco(hci_pkt_type_t packetType)
{
    if (packetType & 0x03C0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CsrBool CsrBtCmDmIsBrEsco(hci_pkt_type_t packetType)
{
    if (packetType & 0x0038)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CsrBool CsrBtCmDmIsSco(hci_pkt_type_t packetType)
{
    if (packetType & 0x007)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static const cmHciPacketFeatureType_t packetTypeRating[] =
{
    { HCI_ESCO_PKT_3EV5,    LMP_FEATURES_3SLOT_ESCO_MR_BIT  },
    { HCI_ESCO_PKT_2EV5,    LMP_FEATURES_3SLOT_ESCO_MR_BIT  },
    { HCI_ESCO_PKT_3EV3,    LMP_FEATURES_MR_ESCO_3MBPS_BIT  },
    { HCI_ESCO_PKT_2EV3,    LMP_FEATURES_MR_ESCO_2MBPS_BIT  },
    { HCI_ESCO_PKT_EV5,     LMP_FEATURES_EV5_PACKETS_BIT    },
    { HCI_ESCO_PKT_EV4,     LMP_FEATURES_EV4_PACKETS_BIT    },
    { HCI_ESCO_PKT_EV3,     LMP_FEATURES_EV3_PACKETS_BIT    },
    { HCI_ESCO_PKT_HV3,     LMP_FEATURES_HV3_PACKETS_BIT    },
    { HCI_ESCO_PKT_HV2,     LMP_FEATURES_HV2_PACKETS_BIT    },
    { HCI_ESCO_PKT_HV1,     LMP_FEATURES_SCO_LINK_BIT       },
};

static CsrBool csrBtCmScoPacketTypeSupported(cmInstanceData_t *cmData, aclTable *aclConnectionElement, CsrUint8 featureBit)
{
    if (aclConnectionElement)
    {
        CsrUint8 featIndex     = featureBit/8;
        CsrUint8 featOffsetBit = featureBit%8;

        return (CSR_BIT_IS_SET(cmData->dmVar.lmpSuppFeatures[featIndex], featOffsetBit) &&
                CSR_BIT_IS_SET(aclConnectionElement->remoteFeatures[featIndex], featOffsetBit));
    }
    else
    {
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                cmData->dmVar.lockMsg,
                                cmData->globalState,
                                "");
        return FALSE;
    }
}

void CsrBtCmScoFreePacketTypeArray(cmSyncNegotiationCntType_t **negotiateCnt)
{
    if (*negotiateCnt)
    {
        CsrPmemFree((*negotiateCnt)->negotiateTypes);
        (*negotiateCnt)->negotiateTypes = NULL;

        CsrPmemFree(*negotiateCnt);
        *negotiateCnt = NULL;
    }
}

CsrBool CsrBtCmScoCurrentNegotiateParmsIsSco(CsrBtCmScoCommonParms *parms)
{
    return CsrBtCmDmIsSco(parms->audioQuality);
}

CsrBool CsrBtCmScoGetCurrentNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt, CsrBtCmScoCommonParms *parms)
{
    cmSyncNegotiationType_t *negotiate = &negotiateCnt->negotiateTypes[negotiateCnt->index];

    CsrBtCmLogTextErrorRetVal(negotiateCnt->index < negotiateCnt->count
                              && negotiate->index < negotiate->count,
                              FALSE);

    *parms = negotiate->parms;
    parms->audioQuality = negotiate->packetTypes[negotiate->index];
    return TRUE;
}

static void csrBtCmScoGetCurrentIndices(cmSyncNegotiationCntType_t *negotiateCnt,
                                        cmSyncNegotiationType_t **negotiate,
                                        CsrUint16 **cntIdx, CsrUint16 **typeIdx,
                                        CsrUint16 *cntCount, CsrUint16 *typeCount)
{
    *negotiate  = &negotiateCnt->negotiateTypes[negotiateCnt->index];
    *cntIdx     = &negotiateCnt->index;
    *cntCount   = negotiateCnt->count;
    *typeIdx    = &(*negotiate)->index;
    *typeCount  = (*negotiate)->count;
}

CsrBool CsrBtCmScoGetNextNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt, CsrBtCmScoCommonParms *parms)
{
    cmSyncNegotiationType_t *negotiate;
    CsrUint16 *cntIdx;
    CsrUint16 *typeIdx;
    CsrUint16 cntCount;
    CsrUint16 typeCount;

    csrBtCmScoGetCurrentIndices(negotiateCnt, &negotiate, &cntIdx, &typeIdx, &cntCount, &typeCount);

    if (++*typeIdx < typeCount)
    {
        /* We still have a valid packettype to try */
        *parms = negotiate->parms;
        parms->audioQuality = negotiate->packetTypes[*typeIdx];
        return TRUE;
    }
    else if (++*cntIdx < cntCount)
    {
        /* All packet types exhaused for this set of eSCO parms.
         * Move on to the next set of parameters */
        negotiate = &negotiateCnt->negotiateTypes[*cntIdx];
        *parms = negotiate->parms;
        /* Take first element. We can safely do this since we are guarenteed
         * that negotiateTypes contains at least one valid packettype */
        parms->audioQuality = negotiate->packetTypes[0];
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CsrBool CsrBtCmScoSeekToNextScoOnlyNegotiateParms(cmSyncNegotiationCntType_t *negotiateCnt)
{
    CsrBtCmScoCommonParms parms;

    while (CsrBtCmScoGetNextNegotiateParms(negotiateCnt, &parms))
    {
        if (CsrBtCmDmIsSco(parms.audioQuality))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void CsrBtCmScoSetEScoParms(eScoParmVars *escoParms, CsrBtCmScoCommonParms *parms, hci_connection_handle_t handle)
{
    if (escoParms)
    {
    escoParms->handle        = handle;
    escoParms->packetType    = parms->audioQuality;
    escoParms->txBdw         = parms->txBandwidth;
    escoParms->rxBdw         = parms->rxBandwidth;
    escoParms->maxLatency    = parms->maxLatency;
    escoParms->voiceSettings = parms->voiceSettings;
    escoParms->reTxEffort    = parms->reTxEffort;
}
}

static CsrUint16 csrBtCmScoCreatePacketType(cmInstanceData_t               *cmData,
                                            aclTable                   *aclConnectionElement,
                                            CsrBtCmScoCommonParms       parms,
                                            cmSyncNegotiationType_t    *negotiateType)
{
    CsrUintFast8 i;

    negotiateType->count = 0;
    negotiateType->index = 0;

    for (i = 0; i < CSR_ARRAY_SIZE(packetTypeRating); i++)
    {
        if (CsrBtCmDmIsEdrEsco(packetTypeRating[i].packetType))
        {/* EDR eSCO Packet types 2EV3, 3EV3, 2EV5 and 3EV5 */
            if (!(parms.audioQuality & packetTypeRating[i].packetType))
            {
                if (csrBtCmScoPacketTypeSupported(cmData, aclConnectionElement, packetTypeRating[i].featureBit))
                {
                    negotiateType->packetTypes[negotiateType->count] = ((~packetTypeRating[i].packetType) & HCI_ALL_MR_ESCO);
                    ++negotiateType->count;
                }
            }
        }
        else if (CsrBtCmDmIsBrEsco(packetTypeRating[i].packetType))
        {/* BR eSCO Packet types EV3, EV4 and EV5 */
            if (parms.audioQuality & packetTypeRating[i].packetType)
            {
                if (csrBtCmScoPacketTypeSupported(cmData, aclConnectionElement, packetTypeRating[i].featureBit))
                {
                    negotiateType->packetTypes[negotiateType->count] = packetTypeRating[i].packetType | HCI_ALL_MR_ESCO;
                    ++negotiateType->count;
                }
            }
        }
        else
        { /* SCO packet types (HV1, HV2 and HV3)*/
            if (aclConnectionElement->encryptType != DM_SM_ENCR_ON_BREDR_AES_CCM)
            { /* Check if the link is SC then forbid SCO packet types */
                if (parms.audioQuality & packetTypeRating[i].packetType)
                {
                    if (csrBtCmScoPacketTypeSupported(cmData, aclConnectionElement, packetTypeRating[i].featureBit))
                    {
                        negotiateType->packetTypes[negotiateType->count] = packetTypeRating[i].packetType | HCI_ALL_MR_ESCO;
                        ++negotiateType->count;
                    }
                }
            }
        }
    }

    return negotiateType->count;
}

CsrUint16 CsrBtCmScoCreatePacketTypeArray(cmInstanceData_t                 *cmData,
                                          CsrBtCmScoCommonParms     **primParms,
                                          CsrUint16                   primParmsLen,
                                          CsrBtDeviceAddr               deviceAddr,
                                          cmSyncNegotiationCntType_t **negotiateCnt)
{
    CsrUintFast8 i;
    aclTable *aclConnectionElement;
    cmSyncNegotiationType_t *negotiateType;

    CsrBtCmScoCommonParms *parms = *primParms;

    /* Free previous parms if any */
    CsrBtCmScoFreePacketTypeArray(negotiateCnt);
    /* Zero out index and count parameters */
    *negotiateCnt                   = CsrPmemZalloc(sizeof(cmSyncNegotiationCntType_t));
    (*negotiateCnt)->negotiateTypes = CsrPmemZalloc(sizeof(cmSyncNegotiationType_t) * primParmsLen);

    returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        for (i = 0; i < primParmsLen; ++i)
        {
            negotiateType = &(*negotiateCnt)->negotiateTypes[(*negotiateCnt)->count];

            if (csrBtCmScoCreatePacketType(cmData, aclConnectionElement, parms[i], negotiateType))
            {
                negotiateType->parms = parms[i];
                ++(*negotiateCnt)->count;
            }
        }
    }

    CsrPmemFree(*primParms);
    *primParms = NULL;

    return (*negotiateCnt)->count;
}

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
void CsrBtCmBccmdMapScoPcmCfmHandler(cmInstanceData_t *cmData)
{
    CsrBccmdCfm * prim =  (CsrBccmdCfm *) cmData->recvMsgP;

    if (prim->status == CSR_RESULT_SUCCESS)
    {
        switch (prim->seqNo & 0xFF00)
        {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case RFC_SCO_CONNECT:
                {
                    CsrBtCmRfcScoConnectReqHandler(cmData, cmData->dmVar.scoConnectAddr);
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

            case INCOMING_SCO_CON:
                {
                    aclTable    * aclConnectionElement;
                    returnAclConnectionFromIndex(cmData, (CsrUint8)(prim->seqNo & 0x00FF), &aclConnectionElement);

                    if (!CsrBtBdAddrEqZero(&aclConnectionElement->deviceAddr))
                    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
                        cmRfcConnElement *theElement = returnReserveScoIndexToThisAddress(cmData, aclConnectionElement->deviceAddr);

                        if(theElement)
                        { /* One of the RFC profiles has accepted a eSCO connection. */
                            cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

                            if (theLogicalLink->eScoParms &&
                                theLogicalLink->eScoParms->handle == SCOBUSY_ACCEPT)
                            {
                                dm_sync_connect_rsp(&(aclConnectionElement->deviceAddr),
                                                    HCI_SUCCESS,
                                                    theLogicalLink->eScoParms->txBdw,
                                                    theLogicalLink->eScoParms->rxBdw,
                                                    theLogicalLink->eScoParms->maxLatency,
                                                    theLogicalLink->eScoParms->voiceSettings,
                                                    theLogicalLink->eScoParms->reTxEffort,
                                                    theLogicalLink->eScoParms->packetType);
                            }
                            else
                            {
                                /* sco accept has been cancelled during setup - reject the connection */
                                dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                                    HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                                    0, /* tx bandwidth */
                                                    0, /* rx bandwidth */
                                                    0, /* max latency */
                                                    0, /* voice settings */
                                                    0, /* retx effort */
                                                    0); /* packet type */
                                CsrBtCmDmLocalQueueHandler();
                            }
                        }
                        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
                        { /* The search criterion did not match in the rfc connection table. Reject the eSCO connection */
                            dm_sync_connect_rsp(&(aclConnectionElement->deviceAddr),
                                                HCI_ERROR_REJ_BY_REMOTE_PERS,
                                                0, /* tx bandwidth */
                                                0, /* rx bandwidth */
                                                0, /* max latency */
                                                0, /* voice settings */
                                                0, /* retx effort */
                                                0); /* packet type */
                            CsrBtCmDmLocalQueueHandler();
                        }
                    }
                    else
                    {
                        /* No ACL link exist here, so do not send a connection reject command */
                        CsrBtCmDmLocalQueueHandler();
                    }
                    break;
                }

            default:
                {
                    CsrBtCmGeneralException(CSR_BCCMD_PRIM,
                                            prim->type,
                                            0,
                                            "Unknown seqno");
                    break;
                }
        }
    }
    else
    {
        switch (prim->seqNo & 0xFF00)
        {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case RFC_SCO_CONNECT:
                {
                    if (cmData->dmVar.rfcConnIndex != CM_ERROR)
                    {
                        cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromIndex, &(cmData->dmVar.rfcConnIndex));

                        if (theElement)
                        {
                            cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;

                            CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                               theLogicalLink->eScoParms);
                            CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                          theLogicalLink->btConnId,
                                                          NULL,
                                                          0,
                                                          (CsrBtResultCode) prim->status,
                                                          CSR_BT_SUPPLIER_BCCMD);
                        }
                        CsrBtCmDmLocalQueueHandler();
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

            case INCOMING_SCO_CON:
                {
                    aclTable    * aclConnectionElement;
                    returnAclConnectionFromIndex(cmData, (CsrUint8)(prim->seqNo & 0x00FF), &aclConnectionElement);
                    dm_sync_connect_rsp(&(aclConnectionElement->deviceAddr),
                                        HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                        0, /* tx bandwidth */
                                        0, /* rx bandwidth */
                                        0, /* max latency */
                                        0, /* voice settings */
                                        0, /* retx effort */
                                        0); /* packet type */
                    CsrBtCmDmLocalQueueHandler();
                    break;
                }

            default:
                {
                    CsrBtCmGeneralException(CSR_BCCMD_PRIM,
                                            prim->type,
                                            0,
                                            "Unknown seqno");
                    break;
                }
        }
    }
    CsrPmemFree(prim->payload);
    prim->payload = NULL;
}
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */

CsrUint8 CsrBtCmDmFindFreePcmSlot(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;

    for (i = 0; i < MAX_PCM_SLOTS; i++)
    {
        if (cmData->dmVar.pcmAllocationTable[i] == NO_SCO)
        {
            return i + 1;
        }
    }
    return CSR_BT_PCM_DONT_CARE;
}

static void csrBtCmRfcMapScoIndSend(CsrSchedQid appHandle, CsrUint32 btConnId, CsrUint8 linkType)
{
    CsrBtCmMapScoPcmInd *msg;

    msg = (CsrBtCmMapScoPcmInd *)CsrPmemAlloc(sizeof(CsrBtCmMapScoPcmInd));
    msg->type = CSR_BT_CM_MAP_SCO_PCM_IND;
    msg->btConnId = btConnId;
    msg->linkType = linkType;
    CsrBtCmPutMessage(appHandle, msg);
}

static void csrBtCmIncomingScoReqSend(cmInstanceData_t *cmData,
                                      CsrSchedQid appHandle, 
                                      CsrBtConnId btConnId, 
                                      CsrUint8    linkType, 
                                      dm_protocol_id_t protocolId)
{
    CsrBtCmIncomingScoReq *msg = (CsrBtCmIncomingScoReq *)CsrPmemAlloc(sizeof(CsrBtCmIncomingScoReq));
    msg->type       = CSR_BT_CM_INCOMING_SCO_REQ;
    msg->appHandle  = appHandle;
    msg->btConnId   = btConnId;
    msg->linkType   = linkType;
    msg->protocolId = protocolId;
    dm_free_upstream_primitive(cmData->recvMsgP);
    cmData->recvMsgP = msg;
    CsrBtCmDmProvider(cmData);
    SynergyMessageFree(CSR_BT_CM_PRIM, cmData->recvMsgP);
    cmData->recvMsgP = NULL;
}

void CsrBtCmIncomingScoReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmIncomingScoReq *prim = (CsrBtCmIncomingScoReq *)cmData->recvMsgP;

    /* The protocol can only be RFCOMM as the SCO over L2CAP (TCS) is not supported any more;
       it is deprecated */
    csrBtCmRfcMapScoIndSend(prim->appHandle, prim->btConnId, prim->linkType);
}

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmMapScoPcmResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmMapScoPcmRes *cmPrim = (CsrBtCmMapScoPcmRes *)cmData->recvMsgP;

    cmRfcConnElement * theElement    = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(cmPrim->btConnId));

    if(theElement)
    { /* Request to create SCO connection to remote Bluetooth device */
        aclTable    * aclConnectionElement;
        CsrUint8     aclIndex;

        cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;
        aclIndex = returnAclConnectionElement(cmData, theLogicalLink->deviceAddr, &aclConnectionElement);

        if (theLogicalLink->eScoParms &&
            theLogicalLink->eScoParms->handle == SCOBUSY_ACCEPT)
        {
            if (cmPrim->acceptResponse != HCI_SUCCESS)
            {
                dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                    cmPrim->acceptResponse,
                                    0, /* tx bandwidth */
                                    0, /* rx bandwidth */
                                    0, /* max latency */
                                    0, /* voice settings */
                                    0, /* retx effort */
                                    0); /* packet type */
                CsrBtCmDmLocalQueueHandler();
                return;
            }
            else if(cmPrim->parms)
            {
                theLogicalLink->eScoParms->txBdw = cmPrim->parms->txBandwidth;
                theLogicalLink->eScoParms->rxBdw = cmPrim->parms->rxBandwidth;
                theLogicalLink->eScoParms->maxLatency = cmPrim->parms->maxLatency;
                theLogicalLink->eScoParms->voiceSettings = cmPrim->parms->voiceSettings;
                theLogicalLink->eScoParms->reTxEffort = cmPrim->parms->reTxEffort;
                theLogicalLink->eScoParms->packetType = SCO_PACKET_RESERVED_BITS | cmPrim->parms->audioQuality;
            }

            /* 'theElement' (RFC) is set, so should be an Acl Connection.
               but guard against de-ref of a NULL */
            if (    aclConnectionElement
                && (aclConnectionElement->encryptType == DM_SM_ENCR_ON_BREDR_AES_CCM))
            {
                /* SCO packet types were not allowed on AES-CCM link, Mask SCO packet type bits */
                theLogicalLink->eScoParms->packetType = theLogicalLink->eScoParms->packetType & CSR_BT_CM_SCO_MASK_BITS;
                if (theLogicalLink->eScoParms->packetType == 0xFFC0)
                {/* Only reserved bits are left seems Application has restricted the packet types to only SCO packet types,
                    so returning with error code 0x0E (Rejected Due to Security Reasons) and
                    SCO transport will not be established*/
                    dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                        HCI_ERROR_REJ_BY_REMOTE_SEC,
                                        0, /* tx bandwidth */
                                        0, /* rx bandwidth */
                                        0, /* max latency */
                                        0, /* voice settings */
                                        0, /* retx effort */
                                        0); /* packet type */
                    CsrBtCmDmLocalQueueHandler();
                    return;
                }
            }

            if (cmPrim->pcmSlot != CSR_BT_PCM_DONT_MAP)
            {
                if ((cmPrim->pcmSlot == 0) || ((cmPrim->pcmSlot <= MAX_PCM_SLOTS) &&
                                               (cmData->dmVar.pcmAllocationTable[cmPrim->pcmSlot - 1] == NO_SCO)))
                {
                    theLogicalLink->eScoParms->pcmSlot = cmPrim->pcmSlot;
                }
                else
                {
                    if ((cmPrim->pcmReassign) || (cmPrim->pcmSlot == CSR_BT_PCM_DONT_CARE))
                    {
                        theLogicalLink->eScoParms->pcmSlot = CsrBtCmDmFindFreePcmSlot(cmData);
                        if (theLogicalLink->eScoParms->pcmSlot > MAX_PCM_SLOTS)
                        {
                            dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                                HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                                0, /* tx bandwidth */
                                                0, /* rx bandwidth */
                                                0, /* max latency */
                                                0, /* voice settings */
                                                0, /* retx effort */
                                                0); /* packet type */
                            CsrBtCmDmLocalQueueHandler();
                            return;
                        }
                    }
                    else
                    {
                        dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                            HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                            0, /* tx bandwidth */
                                            0, /* rx bandwidth */
                                            0, /* max latency */
                                            0, /* voice settings */
                                            0, /* retx effort */
                                            0); /* packet type */
                        CsrBtCmDmLocalQueueHandler();
                        return;
                    }
                }

                if (theLogicalLink->eScoParms->pcmSlot != 0)
                {
                    cmData->dmVar.pcmAllocationTable[theLogicalLink->eScoParms->pcmSlot - 1] = SCOBUSY_ACCEPT;
                }
#ifndef EXCLUDE_CSR_BT_CHIP_SUPPORT_MAP_SCO_PCM
                /* Do not issue map_sco_pcm bccmd if DSPM or QCA chips are used! */
                CsrBtCmBccmdMapScoPcmReqSend(theLogicalLink->eScoParms->pcmSlot,
                                             (CsrUint16) (INCOMING_SCO_CON | aclIndex));
#else
                CSR_UNUSED(aclIndex);
                cmData->dmVar.scoConnectAddr = theLogicalLink->deviceAddr;
                dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                    HCI_SUCCESS,
                                    theLogicalLink->eScoParms->txBdw,
                                    theLogicalLink->eScoParms->rxBdw,
                                    theLogicalLink->eScoParms->maxLatency,
                                    theLogicalLink->eScoParms->voiceSettings,
                                    theLogicalLink->eScoParms->reTxEffort,
                                    theLogicalLink->eScoParms->packetType);
#endif                
            }
            else
            { /* Skip map SCO over PCM and go directly for the SCO setup procedure */
                theLogicalLink->eScoParms->pcmSlot = 0;
                cmData->dmVar.scoConnectAddr = theLogicalLink->deviceAddr;
                dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                    HCI_SUCCESS,
                                    theLogicalLink->eScoParms->txBdw,
                                    theLogicalLink->eScoParms->rxBdw,
                                    theLogicalLink->eScoParms->maxLatency,
                                    theLogicalLink->eScoParms->voiceSettings,
                                    theLogicalLink->eScoParms->reTxEffort,
                                    theLogicalLink->eScoParms->packetType);
            }
        }
        else
        {
            /* sco accept has been cancelled during setup - reject the connection */
            dm_sync_connect_rsp(&(theLogicalLink->deviceAddr),
                                HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                0, /* tx bandwidth */
                                0, /* rx bandwidth */
                                0, /* max latency */
                                0, /* voice settings */
                                0, /* retx effort */
                                0); /* packet type */
            CsrBtCmDmLocalQueueHandler();
        }
    }
    else
    {
        /* We have no way of knowing which ACL link should have the reject command,
           so we rely on the connection accept timeout on the chip */
        CsrBtCmDmLocalQueueHandler();
    }
}
#endif

void CsrBtCmDmSyncConnectIndHandler(cmInstanceData_t *cmData)
{
    DM_SYNC_CONNECT_IND_T   *dmPrim = (DM_SYNC_CONNECT_IND_T *) cmData->recvMsgP;

    CsrUint8 numOfSco    = returnNumberOfScoConnection(cmData);

    if (numOfSco < CSR_BT_MAX_NUM_SCO_CONNS)
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        cmRfcConnElement *theElement = returnReserveScoIndexToThisAddress(cmData, dmPrim->bd_addr);

        if(theElement)
        { /* One of the RFC profiles has accepted a eSCO connection. */
            cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;
            csrBtCmIncomingScoReqSend(cmData, 
                                      theLogicalLink->appHandle, 
                                      theLogicalLink->btConnId, 
                                      dmPrim->link_type, 
                                      SEC_PROTOCOL_RFCOMM);
        }
        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
        { /* The search criterion did not match in the rfc connection table. Reject the eSCO connection */
            dm_sync_connect_rsp(&(dmPrim->bd_addr),
                                HCI_ERROR_REJ_BY_REMOTE_PERS,
                                0, /* tx bandwidth */
                                0, /* rx bandwidth */
                                0, /* max latency */
                                0, /* voice settings */
                                0, /* retx effort */
                                0); /* packet type */
        }
    }
    else
    {
        dm_sync_connect_rsp(&(dmPrim->bd_addr),
                            HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                            0, /* tx bandwidth */
                            0, /* rx bandwidth */
                            0, /* max latency */
                            0, /* voice settings */
                            0, /* retx effort */
                            0); /* packet type */
    }
}

void CsrBtCmDmSyncConnectCfmHandler(cmInstanceData_t *cmData)
{ /* This event is the confirmation of the opening (or otherwise) of a eSCO
     connection following a eSCO connect request. Restore the DM queue and
     inform the application */
    DM_SYNC_CONNECT_CFM_T *dmPrim = (DM_SYNC_CONNECT_CFM_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
    CsrBool incoming = FALSE;

    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateSyncConnectEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION,
                         dmPrim->status,
                         dmPrim,
                         &incoming);
#endif

    if(cmData->dmVar.rfcConnIndex != CM_ERROR)
    {/* The saved index did match. Decide which player requested the sco
        connection */
        switch(cmData->dmVar.activePlayer)
        {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case RFC_PLAYER :
                {
                    cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromIndex, &(cmData->dmVar.rfcConnIndex));

                    if (theElement &&
                        theElement->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED)
                    {
                        CsrBtCmDmSyncConnectRfcCfmHandler(theElement->cmRfcConnInst, cmData);
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
                        if (dmPrim->status == HCI_SUCCESS
                            && CsrBtCmEventSubscribed(cmData,
                                                      CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION))
                        {
                            /* WiFi Coex: ask for eSCO timing parameters */
                            CsrBtCmBccmdGetScoParametersReqSend(dmPrim->handle,RFC_SCO_CONNECT);
                        }
#endif
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
                    }
                    else
                    {
                        if(dmPrim->status == HCI_SUCCESS)
                        {
                            dm_sync_disconnect_req(dmPrim->handle,
                                                   HCI_ERROR_OETC_USER);
                            cmData->dmVar.rfcConnIndex = CM_ERROR;

                            if (theElement && theElement->cmRfcConnInst->eScoParms)
                            {
                                theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
                            }
                        }
                        CsrBtCmDmLocalQueueHandler();
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

            default :
                {
                    /* If contableIndex is valid, so is activePlayer */
                    break;
                }
        }
    }
    else
    {/* The saved index did not match. If the result is with success the SCO connection
        is release */

        if (dmPrim->status == HCI_SUCCESS)
        {
            cmRfcConnElement *theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle,
                                                               &dmPrim->handle);
            dm_sync_disconnect_req(dmPrim->handle,
                                   HCI_ERROR_OETC_USER);

            if (theElement && theElement->cmRfcConnInst->eScoParms)
            {
                theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
            }
        }
        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmSyncConnectCompleteIndHandler(cmInstanceData_t *cmData)
{
    DM_SYNC_CONNECT_COMPLETE_IND_T *dmPrim = (DM_SYNC_CONNECT_COMPLETE_IND_T*) cmData->recvMsgP;
    CsrUint8 numOfSco = returnNumberOfScoConnection(cmData);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
    CsrBool incoming = TRUE;

    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateSyncConnectEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION,
                         dmPrim->status,
                         dmPrim,
                         &incoming);
#endif

    if (numOfSco < CSR_BT_MAX_NUM_SCO_CONNS)
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        cmRfcConnElement *theElement = returnReserveScoIndexToThisAddress(cmData, dmPrim->bd_addr);

        if(theElement)
        { /* One of the RFC profiles has accepted a eSCO connection. Store the eSco
             handle in the rfc connection table and notify the profile */
            cmRfcConnInstType *theLogicalLink   = theElement->cmRfcConnInst;

            if (theLogicalLink->eScoParms &&
                theLogicalLink->eScoParms->pcmSlot != 0)
            {
                if (dmPrim->status == HCI_SUCCESS)
                {
                    cmData->dmVar.pcmAllocationTable[theLogicalLink->eScoParms->pcmSlot - 1] = dmPrim->handle;
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
                    /* WiFi Coex: ask for eSCO timing parameters */
                    if (CsrBtCmEventSubscribed(cmData,
                                               CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION))
                    {
                        CsrBtCmBccmdGetScoParametersReqSend(dmPrim->handle,RFC_SCO_CONNECT);
                    }
#endif
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
                }
                else
                {
                    CsrBtCmDmSyncClearPcmSlotFromTable(cmData,
                                                       theLogicalLink->eScoParms);
                }
            }
            CsrBtCmDmSyncConnectCompleteRfcHandler(theLogicalLink, dmPrim);
        }
        else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
        { /* The search criterion did not match in the rfc connection table. Reject the SCO connection */
#ifdef CSR_BT_INSTALL_CM_DUT_MODE
            if (!cmData->dmVar.deviceUnderTest)
            {
                if (dmPrim->status == HCI_SUCCESS)
                {
#ifdef EXCLUDE_CSR_BT_RFC_MODULE
                    cmRfcConnElement *theElement;
#endif
                    theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle,
                                                     &dmPrim->handle);

                    dm_sync_disconnect_req(dmPrim->handle, HCI_ERROR_OETC_USER);

                    if (theElement && theElement->cmRfcConnInst->eScoParms)
                    {
                        theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
                    }
                }
            }
            else
#endif
            { /* Just accept it, the local device is in DUT mode    */
                ;
            }
        }
    }
    else
    {
#ifdef CSR_BT_INSTALL_CM_DUT_MODE
        if (!cmData->dmVar.deviceUnderTest)
        {
            if (dmPrim->status == HCI_SUCCESS)
            {
                cmRfcConnElement *theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle,
                                                                   &dmPrim->handle);
                dm_sync_disconnect_req(dmPrim->handle, HCI_ERROR_OETC_USER);

                if (theElement && theElement->cmRfcConnInst->eScoParms)
                {
                    theElement->cmRfcConnInst->eScoParms->closeSco = TRUE;
                }
            }
        }
        else
#endif
        { /* Just accept it, the local device is in DUT mode    */
            ;
        }
    }

    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmSyncRenegotiateIndHandler(cmInstanceData_t *cmData)
{
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmRfcConnElement * theElement;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
    DM_SYNC_RENEGOTIATE_IND_T *dmPrim = (DM_SYNC_RENEGOTIATE_IND_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
        resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle, &(dmPrim->handle));

    if(theElement)
    {
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
        if (dmPrim->status == HCI_SUCCESS
            && CsrBtCmEventSubscribed(cmData,
                                      CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION))
        {
            /* WiFi Coex: ask for eSCO timing parameters */
            CsrBtCmBccmdGetScoParametersReqSend(dmPrim->handle,RFC_SCO_CONNECT);
        }
#endif
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
        CsrBtCmDmScoRenegotiateIndMsgSend(theElement->cmRfcConnInst, resultCode, resultSupplier);
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
}

#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
void CsrBtCmDmSyncRenegotiateCfmHandler(cmInstanceData_t *cmData)
{
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmRfcConnElement        * theElement;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

    DM_SYNC_RENEGOTIATE_CFM_T   * dmPrim = (DM_SYNC_RENEGOTIATE_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode      = (CsrBtResultCode) dmPrim->status;
        resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromScoHandle, &(dmPrim->handle));

    if(theElement)
    {
#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION
        if (dmPrim->status == HCI_SUCCESS
            && CsrBtCmEventSubscribed(cmData,
                                      CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION))
        {
            /* WiFi Coex: ask for eSCO timing parameters */
            CsrBtCmBccmdGetScoParametersReqSend(dmPrim->handle,RFC_SCO_CONNECT);
        }
#endif
#endif /* !EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
        CsrBtCmDmScoRenegotiateCfmMsgSend(theElement->cmRfcConnInst, resultCode, resultSupplier);
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
    CsrBtCmDmLocalQueueHandler();
}
#endif

void CsrBtCmDmSyncDisconnectIndHandler(cmInstanceData_t *cmData)
{
    DM_SYNC_DISCONNECT_IND_T *dmPrim = (DM_SYNC_DISCONNECT_IND_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateSyncDiscIndEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION,
                         dmPrim->reason,
                         dmPrim,
                         NULL);
#endif

    csrBtCmDmSyncDisconnectHandler(cmData,
                                   dmPrim->handle,
                                   HCI_SUCCESS, /*FIXME_DM_SM - the 'status' member isn't there anymore */
                                   (CsrBtReasonCode) dmPrim->reason);
}

void CsrBtCmDmSyncDisconnectCfmHandler(cmInstanceData_t *cmData)
{
    DM_SYNC_DISCONNECT_CFM_T *dmPrim = (DM_SYNC_DISCONNECT_CFM_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateSyncDiscCfmEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION,
                         dmPrim->status,
                         dmPrim,
                         NULL);
#endif

    csrBtCmDmSyncDisconnectHandler(cmData,
                                   dmPrim->handle,
                                   dmPrim->status,
                                   (CsrBtReasonCode) HCI_ERROR_CONN_TERM_LOCAL_HOST);
}
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

