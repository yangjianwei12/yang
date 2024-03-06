/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
TODOs
1. file_list.txt should be updated
2. update x2p files
******************************************************************************/
#include "csr_bt_handover_if.h"
#include "csr_bt_hidd_main.h"
#include "csr_bt_marshal_util.h"
#include "csr_streams.h"
#include "csr_bt_tasks.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_hidd_prim.h"

#define PanicIfFalse(cond) if(!(cond)) CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, #cond " Failed!!")
/*
 * HIDD state analysis for handover.
 * ---------------------------------
 * HIDD_INIT_STATE
 *      Ignore. Boot state.
 * HIDD_IDLE_STATE - Post Synergy Init state. Ready for activation.
 *      Commit to this as default state. No need to marshal.
 * HIDD_REGISTERING_SDP - Processing Activate Request from app. Registering SDP.
 *      Veto. Short lived intermittent state.
 * HIDD_CONNECTING_STATE - Connecting to host.
 *      Veto. Short lived intermittent state.
 * HIDD_UNREGISTERING_SD - State not getting used.
 *      Veto. (future use)
 * HIDD_CONNECTED_STATE - connected to hid host.
 *      Marshal - Stream handles, remote info, state
 * HIDD_ACCEPT_CONNECTING_STATE - Waiting for connection.
 *      Marshal - state
 * HIDD_NOT_CONNECTED_STATE - disconnected.
 *      Marshal - state
 *
 * SDP Record is too big for marshalling. It has to be stored locally in profile as constant.
 *
 */

static CsrBtMarshalUtilInst *hiddConvInst;

static void HiddCleanUpAfterHandoverFailure(HiddInstanceDataType *instData)
{
    CsrBtBdAddrZero(&(instData->bdAddr));
    HiddResetControlAndInterruptChannelData(instData, FALSE);
    HiddResetGenericInstanceData(instData);
}

static bool csrBtHiddVeto (void)
{
    bool veto = FALSE;
    if (csrBtHiddInstData.state == HIDD_CONNECTING_STATE ||
        csrBtHiddInstData.state == HIDD_REGISTERING_SDP ||
        csrBtHiddInstData.state == HIDD_UNREGISTERING_SDP)
    {
        veto = TRUE;
    }

    return veto;
}


static void convHiddMainInstanceData(CsrBtMarshalUtilInst *conv, HiddInstanceDataType *hiddMainInst)
{
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->state);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->ctrlCh.hostMtu);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->ctrlCh.localMtu);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->ctrlCh.cid);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->intrCh.hostMtu);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->intrCh.localMtu);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->intrCh.cid);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->prevError);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->prevErrorSupplier);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->disconnectReason);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->active);
    CsrBtMarshalUtilConvertObj(conv, hiddMainInst->appRequestedDisconnect);
}

static void serHiddMainInstanceData(CsrBtMarshalUtilInst *conv,
                                    HiddInstanceDataType *hiddMainInst,
                                    const CsrBtDeviceAddr *addr)
{
    CSR_UNUSED(addr); //only one instance is supported
    convHiddMainInstanceData(conv, hiddMainInst);
}

static void deserHiddMainInstanceData(CsrBtMarshalUtilInst *conv,
                                      HiddInstanceDataType *hiddMainInst,
                                      const CsrBtDeviceAddr *addr)
{
    hiddMainInst->bdAddr = *addr;
    convHiddMainInstanceData(conv, hiddMainInst);
}


static void registerStreams(CsrBtConnId connId)
{
    CsrStreamsRegister(CM_GET_UINT16ID_FROM_BTCONN_ID(connId),
                       L2CAP_ID,
                       CSR_BT_HIDD_IFACEQUEUE);

    CsrStreamsSourceHandoverPolicyConfigure(CM_GET_UINT16ID_FROM_BTCONN_ID(connId),
                                            L2CAP_ID,
                                            SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);
}

static void csrBtHiddConnectAccept(HiddInstanceDataType *instData)
{
    CsrUint16 flush_to_ctrl = L2CA_FLUSH_TO_DEFAULT, flush_to_intr = L2CA_FLUSH_TO_DEFAULT;
    L2CA_QOS_T *qos_ctrl = NULL, *qos_intr = NULL;
    dm_security_level_t     secIncoming;        /* incoming security level */

    if(instData->ctrlCh.qos != NULL)
    {
        qos_ctrl = (L2CA_QOS_T *)CsrPmemAlloc(sizeof(L2CA_QOS_T));
        SynMemCpyS(qos_ctrl, sizeof(L2CA_QOS_T), instData->ctrlCh.qos,sizeof(L2CA_QOS_T));
        flush_to_ctrl = instData->flushTimeout;
    }

    if(instData->intrCh.qos != NULL)
    {
        qos_intr = (L2CA_QOS_T *)CsrPmemAlloc(sizeof(L2CA_QOS_T));
        SynMemCpyS(qos_intr, sizeof(L2CA_QOS_T), instData->intrCh.qos,sizeof(L2CA_QOS_T));
        flush_to_intr = instData->flushTimeout;
    }

#ifndef INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
        CSR_BT_HIDD_MANDATORY_SECURITY_INCOMING,
        CSR_BT_HIDD_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_HIDD_SUCCESS,
        CSR_BT_RESULT_CODE_HIDD_UNACCEPTABLE_PARAMETER);
#else
    secIncoming = instData->secIncoming;
#endif /* INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS */

    CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_CTRL_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    flush_to_ctrl,
                                    qos_ctrl,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));

    CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_INTR_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    flush_to_intr,
                                    qos_intr,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));

}

static bool csrBtHiddMarshal(const tp_bdaddr *vmTpAddrt,
                             CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "csrBtHiddMarshal"));
    CsrBtTpdAddrT tpAddrt = { 0 };
    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if(CsrBtBdAddrEq(&(csrBtHiddInstData.bdAddr), &tpAddrt.addrt.addr))
    {
        if (!hiddConvInst)
        {
            hiddConvInst = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(hiddConvInst, length, buf, TRUE);
        serHiddMainInstanceData(hiddConvInst, &csrBtHiddInstData, &tpAddrt.addrt.addr);
        *written = length - CsrBtMarshalUtilRemainingLengthGet(hiddConvInst);
        return CsrBtMarshalUtilStatus(hiddConvInst);
    }
    else
    {
        return TRUE;
    }
}

static bool csrBtHiddUnmarshal(const tp_bdaddr *vmTpAddrt,
                               const CsrUint8 *buf,
                               CsrUint16 length,
                               CsrUint16 *written)
{
    CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "csrBtHiddUnmarshal"));
    CsrBtTpdAddrT tpAddrt = { 0 };
    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);
    if (!hiddConvInst)
    {
        hiddConvInst = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(hiddConvInst, length, (void *) buf, TRUE);
    deserHiddMainInstanceData(hiddConvInst, &csrBtHiddInstData, &tpAddrt.addrt.addr);
    *written = length - CsrBtMarshalUtilRemainingLengthGet(hiddConvInst);

    return CsrBtMarshalUtilStatus(hiddConvInst);
}

static void csrBtHiddHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                    bool newPrimary)
{
    CSR_UNUSED(vmTpAddrt);
    CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "csrBtHiddHandoverCommit"));

    if(newPrimary)
    {
        if (csrBtHiddInstData.state == HIDD_ACCEPT_CONNECTING_STATE)
        {
            csrBtHiddConnectAccept(&csrBtHiddInstData);
        }

        if (csrBtHiddInstData.ctrlCh.cid)
        {
            registerStreams(csrBtHiddInstData.ctrlCh.cid);
        }
        if (csrBtHiddInstData.intrCh.cid)
        {
            registerStreams(csrBtHiddInstData.intrCh.cid);
        }
    }
    else
    {
        /* Remove connection instance from new secondary */
    }
}

static void csrBtHiddHandoverComplete(bool newPrimary)
{
    CSR_UNUSED(newPrimary);
    CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "csrBtHiddHandoverComplete"));
    if (hiddConvInst)
    {
        CsrBtMarshalUtilDestroy(hiddConvInst);
        hiddConvInst = NULL;
    }
}

static void csrBtHiddHandoverAbort(void)
{
    CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "csrBtHiddHandoverAbort"));
    if (hiddConvInst && CsrBtMarshalUtilTypeGet(hiddConvInst) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        HiddCleanUpAfterHandoverFailure(&csrBtHiddInstData);
    }

    csrBtHiddHandoverComplete(FALSE);

}

const handover_interface csr_bt_hidd_handover_if =
    MAKE_BREDR_HANDOVER_IF(
    &csrBtHiddVeto,
    &csrBtHiddMarshal,
    &csrBtHiddUnmarshal,
    &csrBtHiddHandoverCommit,
    &csrBtHiddHandoverComplete,
    &csrBtHiddHandoverAbort);

