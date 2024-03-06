/******************************************************************************
 Copyright (c) 2013-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_cm_cme.h"

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
#include "csr_hci_lib.h"
#include "csr_transport.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_cme_prim_8.h"

#define CSR_BT_CM_CME_HOST_REPLAY_CFM_MARSHALL(addr, cme_host_replay_cfm_ptr) \
        do { \
            CME_HEADER_MARSHALL((addr), &((cme_host_replay_cfm_ptr)->header)); \
        } while(0)

/*
 * Upstream CME message CSR_CME_SIGNAL_ID_ACL_FLOW_CONTROL_IND handler
 */
static void csrBtCmCmeAclFlowControlIndHandler(cmInstanceData_t *cmData)
{
    CSR_UNUSED(cmData);

    /* TBD:- TBD when chip supports this feature */
    return;
}

/*
 * Upstream CME message CSR_CME_SIGNAL_ID_HOST_REPLAY_REQ handler
 */
static void csrBtCmCmeHostReplayReqHandler(cmInstanceData_t *cmData)
{
    CsrUintFast8 index = 0;

    /* av updates */
    for ( ;index<CSR_BT_AV_MAX_NUM_STREAMS; index++)
    {
        if (cmData->avStreamVar[index].start)
        {
            CsrBtCmCmeProfileA2dpStartIndSend(cmData, index);
        }
    }

    /* other profile updates - place holder for future profile indications */
    /* obex */
    /* ??? */
}

/*
 * Send downstream CME message CSR_CME_SIGNAL_ID_HOST_REPLAY_CFM 
 */
static void csrBtCmCmeHostReplayCfmSend(cmInstanceData_t *cmData)
{
    CsrUint8 *prim = (CsrUint8 *)CsrPmemZalloc(sizeof(CSR_CME_SIGNAL_ID_HOST_REPLAY_CFM));
    CsrMblk *data = NULL;
    CsrCmeHostReplayCfm req;
    
    /* prepare the header */
    req.header.Signal_Id = CSR_CME_SIGNAL_ID_HOST_REPLAY_CFM;
    req.header.Length = CME_HOST_REPLAY_CFM_BYTE_SIZE;
    
    CSR_BT_CM_CME_HOST_REPLAY_CFM_MARSHALL(prim, &req);
    
    data = CsrMblkDataCreate(prim, CME_HOST_REPLAY_CFM_BYTE_SIZE, TRUE);
    CsrHciVendorSpecificCommandReqSend(TRANSPORT_CHANNEL_CME, data);
}

/*
 * Send downstream CME message CSR_CME_SIGNAL_ID_PROFILE_A2DP_START_IND 
 */
void CsrBtCmCmeProfileA2dpStartIndSend(cmInstanceData_t *cmData, CsrUint8 idx)
{
    CsrUint8 *prim = (CsrUint8 *)CsrPmemZalloc(sizeof(CSR_CME_SIGNAL_ID_PROFILE_A2DP_START_IND));
    CsrMblk *data = NULL;
    CsrUint8 streamIdx = idx;
    CsrCmeProfileA2dpStartInd req;

    /* prepare the header */
    req.header.Signal_Id = CSR_CME_SIGNAL_ID_PROFILE_A2DP_START_IND;
    req.header.Length = CME_PROFILE_A2DP_START_IND_BYTE_SIZE;
    req.acl_handle = cmData->avStreamVar[streamIdx].aclHandle;
    req.bit_rate = cmData->avStreamVar[streamIdx].bitRate;
    req.codec_location = cmData->avStreamVar[streamIdx].codecLocation;
    req.codec_type = cmData->avStreamVar[streamIdx].codecType;
    req.l2cap_connection_id = cmData->avStreamVar[streamIdx].l2capConnectionId;
    req.period = cmData->avStreamVar[streamIdx].period;
    req.role = cmData->avStreamVar[streamIdx].role;
    req.sampling_freq = cmData->avStreamVar[streamIdx].samplingFreq;
    req.sdu_size = cmData->avStreamVar[streamIdx].sduSize;

    CME_PROFILE_A2DP_START_IND_MARSHALL(prim, &req);
    
    data = CsrMblkDataCreate(prim, CME_PROFILE_A2DP_START_IND_BYTE_SIZE, TRUE);
    CsrHciVendorSpecificCommandReqSend(TRANSPORT_CHANNEL_CME, data);
}

/*
 * Send downstream CME message CSR_CME_SIGNAL_ID_PROFILE_A2DP_STOP_IND 
 */
void CsrBtCmCmeProfileA2dpStopIndSend(cmInstanceData_t *cmData, CsrUint8 idx)
{
    CsrUint8 *prim = (CsrUint8 *)CsrPmemZalloc(sizeof(CSR_CME_SIGNAL_ID_PROFILE_A2DP_STOP_IND));
    CsrMblk *data = NULL;
    CsrUint8 streamIdx = idx;
    CsrCmeProfileA2dpStopInd req;

    /* prepare the header */
    req.header.Signal_Id = CSR_CME_SIGNAL_ID_PROFILE_A2DP_STOP_IND;
    req.header.Length = CME_PROFILE_A2DP_STOP_IND_BYTE_SIZE;
    req.acl_handle = cmData->avStreamVar[streamIdx].aclHandle;
    req.l2cap_connection_id = cmData->avStreamVar[streamIdx].l2capConnectionId;

    CME_PROFILE_A2DP_STOP_IND_MARSHALL(prim, &req);
    
    data = CsrMblkDataCreate(prim, CSR_CME_SIGNAL_ID_PROFILE_A2DP_STOP_IND, TRUE);
    CsrHciVendorSpecificCommandReqSend(TRANSPORT_CHANNEL_CME, data);
}

/*
 * Coexistence Management Entity for Bluetooth host initialization sequence
 */
void CsrBtCmCmeInit(void)
{
    /* Register with HCI to receive vendor specific events on HCI extension channel no. 24 */
    CsrHciRegisterVendorSpecificEventHandlerReqSend
    (
        CSR_BT_CM_IFACEQUEUE, TRANSPORT_CHANNEL_CME
    );
}

/*
 * HCI vendor specific event handler for CME HCI extension channel no.24
 */
void CsrBtCmCmeHciVendorSpecificEventIndHandler(cmInstanceData_t *cmData)
{
    CsrHciVendorSpecificEventInd *prim = cmData->recvMsgP;

    CsrUint16 dataLen = CsrMblkGetLength(prim->data);

    CsrUint8 *data = CsrMblkMap(prim->data, 0, dataLen);

    CsrCmeSignalId signalId = CME_HEADER_SIGNAL_ID_GET(data);

    /* check on cme service status on chip */
    if (!cmData->cmeServiceRunning && 
            ((signalId == CSR_CME_SIGNAL_ID_HOST_REPLAY_REQ) ||
                (signalId == CSR_CME_SIGNAL_ID_ACL_FLOW_CONTROL_IND)))
    {
        cmData->cmeServiceRunning = TRUE;
    }
    else if (signalId == CSR_CME_SIGNAL_ID_COEX_STOP_IND)
    {

        cmData->cmeServiceRunning = FALSE;
    }

    /* handle cme signal */
    switch(signalId)
    {
        case CSR_CME_SIGNAL_ID_HOST_REPLAY_REQ:
        {
            /* Replay Bluetooth host high context information */
            csrBtCmCmeHostReplayReqHandler(cmData);
            csrBtCmCmeHostReplayCfmSend(cmData);
            break;
        }
        case CSR_CME_SIGNAL_ID_ACL_FLOW_CONTROL_IND:
        {
            /* Low priority ACL flow control on/off */
            csrBtCmCmeAclFlowControlIndHandler(cmData);
            break;
        }
        case CSR_CME_SIGNAL_ID_COEX_STOP_IND:
        {
            /* Coexistence service is stopping on chip */
            break;
        }
        default:
        {
            /* hah! */
            break;
        }
    }
    CsrMblkUnmap(prim->data, data);
    CsrMblkDestroy(prim->data);
}

#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

