/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_pmem.h"

#include "csr_bt_hidd_sef.h"
#include "csr_bt_hidd_prim.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_hidd_local_lib.h"
#include "csr_bt_cm_private_lib.h"
#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_sdc_support.h"
#include "csr_bt_hidd_local_lib.h"
#include "csr_bt_util.h"
#include "csr_bt_hidd_local_lib.h"
#include "csr_log_text_2.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_streams.h"
#include "csr_bt_hidd_stream.h"
#endif
/* to determine which service record is being registered */
#define HIDD_SDS_REQUEST_NONE       0
#define HIDD_SDS_REQUEST_HID        1
#define HIDD_SDS_REQUEST_DEVICE     2

/* HID specific service record attributes */
#define HID_PARSER_VERSION          0x0201
#define HID_DEVICE_SUBCLASS         0x0202
#define HID_COUNTRY_CODE            0x0203
#define HID_VIRTUAL_CABLE           0x0204
#define HID_RECONNECT_INITIATE      0x0205
#define HID_DESCRIPTOR_LIST         0x0206
#define HID_LANGID_BASE_LIST        0x0207
#define HID_SDP_DISABLE             0x0208
#define HID_BATTERY_POWER           0x0209
#define HID_REMOTE_WAKE             0x020A
#define HID_PROFILE_VERSION         0x020B
#define HID_SUPERVISION_TIMEOUT     0x020C
#define HID_NORMALLY_CONNECTABLE    0x020D
#define HID_BOOT_DEVICE             0x020E

/* Reason for disconnecting when connected */
#define HIDD_DISCONNECT_REASON_UNSET  0
#define HIDD_CONNECTED_UNPLUG_IND     1
#define HIDD_CONNECTED_UNPLUG_REQ     2
#define HIDD_CONNECTED_CHANGE_MODE    3
#define HIDD_CONNECTED_DISCONNECT_REQ 4

#define HIDD_ACCEPT_CONNECT_TIMEOUT   60000000 /* 60 seconds */

#ifdef INSTALL_APP_TRIGGERED_HIDD_RECONNECT
#define HIDD_NUM_OF_RECONNECTS          0
#define HIDD_CONNECT_ATTEMPT_TIMEOUT    0
#else
#define HIDD_NUM_OF_RECONNECTS        HIDD_MAX_NUM_OF_RECONNECT_ATTEMPTS
#define HIDD_CONNECT_ATTEMPT_TIMEOUT  HIDD_RECONNECT_DELAY
#endif

/* local functions */
static void reconnectTimeOut(CsrUint16 not_used, void *data)
{
    HiddInstanceDataType    *instData;
    CSR_UNUSED(not_used);

    instData = data;
    if(instData->reconnect == TRUE)
    {
        if(instData->state == HIDD_ACCEPT_CONNECTING_STATE)
        {
            CsrBtCml2caCancelConnectAcceptReqSend(instData->myAppHandle, CSR_BT_HID_CTRL_PSM);
            CsrBtCml2caCancelConnectAcceptReqSend(instData->myAppHandle, CSR_BT_HID_INTR_PSM);
        }

        instData->timerId = 0;
    }
}


static void csrBtHiddGetHidFlags(HiddInstanceDataType *instData)
{
    CsrUint8 tmpFlag = 0;

    if(CsrBtSdcGetBoolDirect(instData->hidSdp.recordLen, instData->hidSdp.record, HID_VIRTUAL_CABLE, &tmpFlag))
    {
        if(tmpFlag)
        {
            instData->hidFlags |= CSR_BT_HIDD_FLAGS_VIRTUAL_CABLE_BIT;
            tmpFlag = 0;
        }
    }

    if(CsrBtSdcGetBoolDirect(instData->hidSdp.recordLen, instData->hidSdp.record, HID_RECONNECT_INITIATE, &tmpFlag))
    {
        if(tmpFlag)
        {
            instData->hidFlags |= CSR_BT_HIDD_FLAGS_RECONNECT_INIT_BIT;
            tmpFlag = 0;
        }
    }
}

static void csrBtHiddConnect(HiddInstanceDataType *instData, CsrUint8 psm)
{
    dm_security_level_t     secOutgoing;        /* incoming security level */
        
#ifndef INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoing, CSR_BT_SEC_DEFAULT,
            CSR_BT_HIDD_MANDATORY_SECURITY_OUTGOING,
            CSR_BT_HIDD_DEFAULT_SECURITY_OUTGOING,
            CSR_BT_RESULT_CODE_HIDD_SUCCESS,
            CSR_BT_RESULT_CODE_HIDD_UNACCEPTABLE_PARAMETER);
#else
        secOutgoing= instData->secOutgoing;
#endif /* INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS */

    /* We don't support QOS yet in L2CAP */
    CsrBtCml2caConnectReqSend(instData->myAppHandle,
                        instData->bdAddr,
                        psm,
                        psm,
                        CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                        secOutgoing,
                        CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                               CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));
}

static void csrBtHiddAcceptConnect(HiddInstanceDataType *instData)
{
    dm_security_level_t     secIncoming;        /* incoming security level */
    
#ifndef INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
        CSR_BT_HIDD_MANDATORY_SECURITY_INCOMING,
        CSR_BT_HIDD_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_HIDD_SUCCESS,
        CSR_BT_RESULT_CODE_HIDD_UNACCEPTABLE_PARAMETER);
#else
    secIncoming = instData->secIncoming;
#endif /* INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS */

    if(instData->reconnect)
    {
        instData->timerId = CsrSchedTimerSet( HIDD_ACCEPT_CONNECT_TIMEOUT,(void (*) (CsrUint16, void *)) reconnectTimeOut, 0, (void *) instData);
    }
    if(instData->ctrlCh.qos == NULL)
    {
        CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_CTRL_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    L2CA_FLUSH_TO_DEFAULT,
                                    NULL,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));
    }
    else
    {
        L2CA_QOS_T *qos = (L2CA_QOS_T *)CsrPmemAlloc(sizeof(L2CA_QOS_T));
        SynMemCpyS(qos,sizeof(L2CA_QOS_T),instData->ctrlCh.qos,sizeof(L2CA_QOS_T));
        CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_CTRL_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    instData->flushTimeout, /* recommended L2CA_FLUSH_TO_DEFAULT */
                                    qos,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));
    }
    if(instData->intrCh.qos == NULL)
    {
        CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_INTR_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    L2CA_FLUSH_TO_DEFAULT,
                                    NULL,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));
    }
    else
    {
        L2CA_QOS_T *qos = (L2CA_QOS_T *)CsrPmemAlloc(sizeof(L2CA_QOS_T));
        SynMemCpyS(qos,sizeof(L2CA_QOS_T),instData->intrCh.qos,sizeof(L2CA_QOS_T));
        CsrBtCml2caConnectAcceptReqSend(instData->myAppHandle,
                                    CSR_BT_HID_INTR_PSM,
                                    0, /* Class of Device */
                                    secIncoming,
                                    CSR_BT_HIDD_PROFILE_DEFAULT_MTU_SIZE,
                                    instData->flushTimeout, /* recommended L2CA_FLUSH_TO_DEFAULT */
                                    qos,
                                    CSR_BT_HID_PROFILE_UUID,
                                    CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                           CSR_BT_HID_DEFAULT_ENC_KEY_SIZE_VAL));
    }
}

static void connectTimeOut(CsrUint16 psm, void *data)
{
    HiddInstanceDataType    *instData;

    instData = data;

    if(instData->reconnect == TRUE)
    {
        if(instData->state == HIDD_CONNECTING_STATE)
        {
            csrBtHiddConnect(instData, (CsrUint8)psm);
        }
        instData->timerId = 0;
    }

}

static void csrBtHiddRestoreSavedMessages(HiddInstanceDataType * instData)
{
    CsrBtHiddRestoreInd* prim;
    if (instData->saveQueue != NULL)
    {
        prim = CsrPmemAlloc(sizeof(CsrBtHiddRestoreInd));
        prim->type = CSR_BT_HIDD_RESTORE_IND;
        CsrBtHiddMessagePut(instData->myAppHandle, prim);

        instData->restoreHiddFlag = TRUE;
    }
    else if(instData->saveCmQueue != NULL)
    {
        prim = CsrPmemAlloc(sizeof(CsrBtHiddRestoreInd));
        prim->type = CSR_BT_HIDD_RESTORE_IND;
        CsrBtHiddMessagePut(instData->myAppHandle, prim);

        instData->restoreCmFlag = TRUE;
    }
}

static bool csrBtHiddIsValidRecordHandle(HiddInstanceDataType *instData)
{
    if (instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_NONE ||
        instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_HID ||
        instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_DEVICE)
    {
        return FALSE;
    }

    return TRUE;
}

/* HIDD handler functions */
void CsrBtHiddActivateIdleHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddActivateReq* prim;
    BD_ADDR_T zeroBdAddr;

    prim = (CsrBtHiddActivateReq *) instData->pRecvMsg;
    CsrBtBdAddrZero(&zeroBdAddr);

    if ((csrBtHiddIsValidRecordHandle(instData) && prim->hidSdp) ||
        (instData->deviceIdSdp.record && prim->deviceIdSdp))
    {
        /* if a service record is provided in the prim and a service record is already registered */
        instData->registeringNewRec = TRUE;

        if (prim->hidSdp)
        {
            if (instData->hidSdp.record)
            {
                CsrPmemFree(instData->hidSdp.record);
                instData->hidSdp.record = NULL;
            }

            instData->hidSdp.recordLen = prim->hidSdpLen;
            instData->hidSdp.record = prim->hidSdp;
        }
        if (prim->deviceIdSdp)
        {
            if (instData->deviceIdSdp.record)
            {
                CsrPmemFree(instData->deviceIdSdp.record);
                instData->deviceIdSdp.record = NULL;
            }
     
            instData->deviceIdSdp.recordLen = prim->deviceIdSdpLen;
            instData->deviceIdSdp.record = prim->deviceIdSdp;
        }
        /* unregister old records (both records will be registered again before sending CSR_BT_HIDD_ACTIVATE_CFM) */
        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_REGISTERING_SDP); /*HIDD_UNREGISTERING_SDP);*/
        if (instData->hidSdp.recordHandle != 0)
        {/* Do not try to unregister unless there is something to unregister */
            CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
        }
        if (instData->deviceIdSdp.recordHandle != 0)
        {/* Do not try to unregister unless there is something to unregister */
            CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->deviceIdSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
        }
        instData->newRecUnregCfmRec = FALSE;
        if (prim->hidSdp)
        {
            instData->hidSdp.recordHandle = HIDD_SDS_REQUEST_HID;
        }
        if (prim->deviceIdSdp)
        {
            instData->deviceIdSdp.recordHandle = HIDD_SDS_REQUEST_DEVICE;
        }
        /* Remember to keep the data received from the application */
        instData->active = TRUE;
        if( !CsrBtBdAddrEq(&(instData->bdAddr), &(prim->deviceAddr)) )
        {
            CsrBtBdAddrCopy(&(instData->bdAddr), &(prim->deviceAddr));
        }
        instData->appHandle = prim->appHandle;
        if((prim->qosCtrl != NULL) && (prim->qosCtrlCount == 1))
        {
            if (instData->ctrlCh.qos)
            {
                CsrPmemFree(instData->ctrlCh.qos);
                instData->ctrlCh.qos = NULL;
            }

            instData->ctrlCh.qos = prim->qosCtrl;
        }
        if((prim->qosIntr) && (prim->qosIntrCount == 1))
        {
            if (instData->intrCh.qos)
            {
                CsrPmemFree(instData->intrCh.qos);
                instData->intrCh.qos = NULL;
            }

            instData->intrCh.qos = prim->qosIntr;
        }
        instData->flushTimeout = prim->flushTimeout;
        instData->disconnectReason = HIDD_DISCONNECT_REASON_UNSET;
    }
    else
    {
        if(instData->active && (instData->disconnectReason == HIDD_DISCONNECT_REASON_UNSET))
        {
            /* if already active and not unplugged */
            instData->reconnect = TRUE;
            if(instData->appHandle == prim->appHandle)
            {
                if(!CsrBtBdAddrEq(&(instData->bdAddr),&(prim->deviceAddr)))
                {
                    CsrGeneralException(CsrBtHiddLto,
                                        0,
                                        CSR_BT_HIDD_PRIM,
                                        prim->type,
                                        (CsrUint16) instData->state,
                                        "Wrong bdAddr handle");
                }
                CsrPmemFree(prim->deviceIdSdp);
                CsrPmemFree(prim->hidSdp);
                CsrPmemFree(prim->qosCtrl);
                CsrPmemFree(prim->qosIntr);
            }
            else
            {
                CsrGeneralException(CsrBtHiddLto,
                                    0,
                                    CSR_BT_HIDD_PRIM,
                                    prim->type,
                                    (CsrUint16) instData->state,
                                    "Wrong app handle");
            }
        }
        else
        {
            instData->active = TRUE;
            if( !CsrBtBdAddrEq(&(instData->bdAddr), &(prim->deviceAddr)) )
            {
                CsrBtBdAddrCopy(&(instData->bdAddr), &(prim->deviceAddr));
            }

            instData->appHandle = prim->appHandle;
            instData->deviceIdSdp.recordLen = prim->deviceIdSdpLen;
            if (instData->deviceIdSdp.record)
            {
                CsrPmemFree(instData->deviceIdSdp.record);
                instData->deviceIdSdp.record = NULL;
            }
            instData->deviceIdSdp.record = prim->deviceIdSdp;
            instData->hidSdp.recordLen = prim->hidSdpLen;
            if (instData->hidSdp.record)
            {
                CsrPmemFree(instData->hidSdp.record);
                instData->hidSdp.record = NULL;
            }
            instData->hidSdp.record = prim->hidSdp;
            if((prim->qosCtrl != NULL) && (prim->qosCtrlCount == 1))
            {
                instData->ctrlCh.qos = prim->qosCtrl;
            }
            if((prim->qosIntr) && (prim->qosIntrCount == 1))
            {
                instData->intrCh.qos = prim->qosIntr;
            }
            instData->flushTimeout = prim->flushTimeout;
            instData->disconnectReason = HIDD_DISCONNECT_REASON_UNSET;
        }

        instData->deactivating = FALSE;

        /* Bluetooth address present or not */
        if( CsrBtBdAddrEq(&zeroBdAddr, &(prim->deviceAddr)) )
        {
            /* no address - register sdp */
            CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_REGISTERING_SDP);
            instData->hidSdp.recordHandle = HIDD_SDS_REQUEST_HID;
            if(instData->hidSdp.record)
            {
                CsrUint8        *record;
                record = (CsrUint8 *) CsrPmemAlloc(instData->hidSdp.recordLen);
                SynMemCpyS(record, instData->hidSdp.recordLen, instData->hidSdp.record, instData->hidSdp.recordLen);

                CsrBtCmSdsRegisterReqSend(instData->myAppHandle, record, instData->hidSdp.recordLen, CSR_BT_CM_CONTEXT_UNUSED);
            }
            else
            {
                CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED, CSR_BT_SUPPLIER_HIDD);
            }
        }
        else
        {
            if(instData->hidFlags & CSR_BT_HIDD_FLAGS_RECONNECT_INIT_BIT)
            {
                /* connect request */
                instData->reconnect = TRUE;
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTING_STATE);
                CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                csrBtHiddConnect(instData, CSR_BT_HID_CTRL_PSM);
            }
            else
            {
                /* register sdp */
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_REGISTERING_SDP);
                instData->hidSdp.recordHandle = HIDD_SDS_REQUEST_HID;
                if(instData->hidSdp.record)
                {
                    CsrUint8        *record;
                    record = (CsrUint8 *) CsrPmemAlloc(instData->hidSdp.recordLen);
                    SynMemCpyS(record, instData->hidSdp.recordLen, instData->hidSdp.record, instData->hidSdp.recordLen);

                    CsrBtCmSdsRegisterReqSend(instData->myAppHandle, record, instData->hidSdp.recordLen, CSR_BT_CM_CONTEXT_UNUSED);
                }
                else
                {
                    CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED, CSR_BT_SUPPLIER_HIDD);
                }
            }
        }
    }
}

void CsrBtHiddReactivateIdleHandler(HiddInstanceDataType *instData)
{
    if(instData->hidFlags & CSR_BT_HIDD_FLAGS_RECONNECT_INIT_BIT)
    {
        /* connect request */
        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTING_STATE);
        instData->numOfRetries = 0;
        csrBtHiddConnect(instData, CSR_BT_HID_CTRL_PSM);
    }
    else
    {

        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_ACCEPT_CONNECTING_STATE);
        csrBtHiddAcceptConnect(instData);
    }
}

void CsrBtHiddDeactivateHandler(HiddInstanceDataType *instData)
{
    instData->deactivating = TRUE;
    instData->active = FALSE;

    if(instData->reconnect)
    {
        instData->reconnect = FALSE;
    }
    if(instData->ctrlCh.cid)
    {
        /* Already connecting - await connect cfm to be able to disconnect */
        if(HIDD_CONNECTED_STATE == instData->state)
        {
            if(instData->intrCh.cid)
            {
                CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
            }
        }
    }
    else
    {
        switch(instData->state)
        {
        case HIDD_ACCEPT_CONNECTING_STATE:
            {
                /* Cancel accept connect */
                CsrBtCml2caCancelConnectAcceptReqSend(instData->myAppHandle, CSR_BT_HID_CTRL_PSM);
                CsrBtCml2caCancelConnectAcceptReqSend(instData->myAppHandle, CSR_BT_HID_INTR_PSM);
            }
            break;
        case HIDD_CONNECTING_STATE:
            {
                /* Cancel connect request */
                CsrBtCml2caCancelConnectReqSend(instData->myAppHandle,instData->bdAddr, CSR_BT_HID_CTRL_PSM);
            }
            break;
        case HIDD_IDLE_STATE:
            /* Fall through */
        case HIDD_NOT_CONNECTED_STATE:
            {
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                instData->prevError = CSR_BT_RESULT_CODE_HIDD_CONNECTION_TERM_BY_LOCAL_HOST;
                instData->prevErrorSupplier = CSR_BT_SUPPLIER_HIDD;
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
                instData->deactUnregCfmRec = FALSE;
            }
            break;
        default:
            /* REGISTER and UNREGISTER */
            break;
        }
    }
}

void CsrBtHiddControlRespConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddControlRes *prim;

    prim = (CsrBtHiddControlRes *) instData->pRecvMsg;

    if(!(prim->data))
    {
        prim->data = CsrPmemAlloc(1);
        prim->dataLen = 1;
    }
    prim->data[0] = prim->transactionType;
    prim->data[0] |= prim->parameter;


    if(prim->dataLen >= instData->ctrlCh.hostMtu)
    {
        CsrUint8* tmp;

        instData->ctrlCh.sendMsg = prim->data;
        instData->ctrlCh.sendMsgLength = prim->dataLen;

        instData->ctrlCh.sendMsgOffset = instData->intrCh.hostMtu;
        tmp = CsrPmemAlloc(instData->ctrlCh.sendMsgOffset);

        SynMemCpyS(tmp, instData->ctrlCh.sendMsgOffset, prim->data, instData->ctrlCh.sendMsgOffset);

        CsrBtCml2caDataReqSend(instData->ctrlCh.cid, instData->ctrlCh.sendMsgOffset, tmp, CSR_BT_CM_CONTEXT_UNUSED);
    }
    else
    {
        CsrBtCml2caDataReqSend(instData->ctrlCh.cid, prim->dataLen, prim->data, CSR_BT_CM_CONTEXT_UNUSED);
    }
}

void CsrBtHiddDataConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddDataReq *prim;

    prim = (CsrBtHiddDataReq *) instData->pRecvMsg;

    prim->report[0] = CSR_BT_HIDD_DATA;
    prim->report[0] |= CSR_BT_HIDD_INPUT_REPORT;

    if(prim->reportLen >= instData->intrCh.hostMtu)
    {
        CsrUint8* tmp;

        instData->intrCh.sendMsg = prim->report;
        instData->intrCh.sendMsgLength = prim->reportLen;

        instData->intrCh.sendMsgOffset = instData->intrCh.hostMtu;
        tmp = CsrPmemAlloc(instData->intrCh.sendMsgOffset);

        SynMemCpyS(tmp, instData->intrCh.sendMsgOffset, prim->report, instData->intrCh.sendMsgOffset);

        CsrBtCml2caDataReqSend(instData->intrCh.cid, instData->intrCh.sendMsgOffset, tmp, CSR_BT_CM_CONTEXT_UNUSED);
    }
    else
    {
        CsrBtCml2caDataReqSend(instData->intrCh.cid, prim->reportLen, prim->report, CSR_BT_CM_CONTEXT_UNUSED);
    }
}

void CsrBtHiddUnplugConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddUnplugReq *prim;

    prim = (CsrBtHiddUnplugReq *) instData->pRecvMsg;

    if(CsrBtBdAddrEq(&(instData->bdAddr), &(prim->deviceAddr)))
    {
        /* Unplug request */
        CsrUint8* tmpData = CsrPmemAlloc(1);
        tmpData[0] = CSR_BT_HIDD_CONTROL;
        tmpData[0] |= CSR_BT_HIDD_VC_UNPLUG;
        instData->disconnectReason = HIDD_CONNECTED_UNPLUG_REQ;
        CsrBtCml2caDataReqSend(instData->ctrlCh.cid, 1, tmpData, CSR_BT_CM_CONTEXT_UNUSED);
    }
    else
    {
        /* Unknown device */
    }
}

void CsrBtHiddChangeModeConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddModeChangeReq *prim;

    prim = (CsrBtHiddModeChangeReq *)instData->pRecvMsg;

    if (prim->mode == CSR_BT_HIDD_DISCONNECT_MODE)
    {
        instData->disconnectReason = HIDD_CONNECTED_CHANGE_MODE;
        CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
    }
    else
    {
        /* nothing to change */
        CsrBtHiddModeChangeIndSend(instData, prim->mode, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
    }
}

void CsrBtHiddChangeModeNotConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtHiddModeChangeReq *prim;

    prim = (CsrBtHiddModeChangeReq *)instData->pRecvMsg;

    /* Setting the mode from the application is treated as connection request, since the connection doesn't exists.*/
    if (prim->mode == CSR_BT_HIDD_SNIFF_MODE || prim->mode == CSR_BT_HIDD_ACTIVE_MODE)
    {
        /* connect request */
        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTING_STATE);
        instData->numOfRetries = 0;
        instData->reconnect = TRUE;
        csrBtHiddConnect(instData, CSR_BT_HID_CTRL_PSM);
    }
    else
    {
        /* nothing to change */
        CsrBtHiddModeChangeIndSend(instData, prim->mode, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
    }
}

/* CM handler functions */
void CsrBtHiddCmRegisterInitHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caRegisterCfm *prim;
    prim = (CsrBtCmL2caRegisterCfm *) instData->pRecvMsg;

    if( prim->localPsm == CSR_BT_HID_CTRL_PSM )
    {
        CsrBtCml2caRegisterReqSend(instData->myAppHandle, CSR_BT_HID_INTR_PSM, L2CA_MODE_MASK_BASIC, 0);
    }
    else if( prim->localPsm == CSR_BT_HID_INTR_PSM )
    {
        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_IDLE_STATE);
        csrBtHiddRestoreSavedMessages(instData);
    }
    else
    {
        CsrGeneralException(CsrBtHiddLto,
                            0,
                            CSR_BT_CM_PRIM,
                            prim->type,
                            (CsrUint16) instData->state,
                            "Unknown PSM");
    }
}

static void csrBtHiddFreeRecord(HiddInstanceDataType *instData)
{
    /* Get HID flags before removing the data.*/
    csrBtHiddGetHidFlags(instData);
    if (instData->hidSdp.record)
    {
        CsrPmemFree(instData->hidSdp.record);
        instData->hidSdp.record = NULL;
        instData->hidSdp.recordLen = 0;        
    }
}

void CsrBtHiddCmRegisterSdpRegisteringHandler(HiddInstanceDataType *instData)
{
    CsrBtCmSdsRegisterCfm *prim;
    prim = (CsrBtCmSdsRegisterCfm *) instData->pRecvMsg;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if(instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_HID)
        {
             /* Hid service record OK */
            instData->hidSdp.recordHandle = prim->serviceRecHandle;

            if(instData->deactivating)
            {
                /* Hid service record already succeeded therefore need to be unregistered */
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                instData->prevError = CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED;
                instData->prevErrorSupplier = CSR_BT_SUPPLIER_HIDD;
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
            }
            else
            {
                if(instData->deviceIdSdp.record)
                {
                    CsrUint8 *record;
                    record = (CsrUint8 *) CsrPmemAlloc(instData->deviceIdSdp.recordLen);
                    SynMemCpyS(record, instData->deviceIdSdp.recordLen, instData->deviceIdSdp.record, instData->deviceIdSdp.recordLen);

                    CsrBtCmSdsRegisterReqSend(instData->myAppHandle, record, instData->deviceIdSdp.recordLen, CSR_BT_CM_CONTEXT_UNUSED);
                }
                else
                {
                    /* Accept connect request */
                    if(!instData->reconnect)
                    {
                        CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                    }
                    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_ACCEPT_CONNECTING_STATE);
                    csrBtHiddAcceptConnect(instData);
                    csrBtHiddFreeRecord(instData);
                }
            }
        }
        else
        {
            /* Device Id service record OK */
            instData->deviceIdSdp.recordHandle = prim->serviceRecHandle;

            if(instData->deactivating)
            {
                /* Hid service record already succeeded therefore need to be unregistered */
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                instData->prevError = CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED;
                instData->prevErrorSupplier = CSR_BT_SUPPLIER_HIDD;
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
            }
            else
            {
                /* Accept connect request */
                if(!instData->reconnect)
                {
                    CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                }
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_ACCEPT_CONNECTING_STATE);
                csrBtHiddAcceptConnect(instData);
            }
        }
    }
    else
    {
        if(instData->deactivating)
        {
            instData->numOfRetries = 0;
            if(instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_HID)
            {
                CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                   CsrBtHiddCleanUpToIdle(instData);
            }
            else
            {
                /* Hid service record already succeeded therefore need to be unregistered */
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                instData->prevError = prim->resultCode;
                instData->prevErrorSupplier = prim->resultSupplier;
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
            }
        }
        else
        {
            if (instData->hidSdp.recordHandle == HIDD_SDS_REQUEST_HID)
            {
                /*  Respond, cleanup and return to idle */
                if (!instData->reconnect)
                {
                    CsrBtHiddActivateCfmSend(instData,
                        CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED,
                        CSR_BT_SUPPLIER_HIDD);
                }
                else
                {
                    CsrBtHiddStatusIndSend(instData,
                        CSR_BT_HIDD_CONNECT_FAILED,
                        CSR_BT_CONN_ID_INVALID);
                }
                CsrBtHiddCleanUpToIdle(instData);
            }
            else
            {
                /* Hid service record already succeeded therefore need to be unregistered */
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
            }
        }
    }
}

void CsrBtHiddCmUnregisterSdpRegisteringHandler(HiddInstanceDataType *instData)
{
    CsrBtCmSdsUnregisterCfm *prim = (CsrBtCmSdsUnregisterCfm *) instData->pRecvMsg;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if (instData->registeringNewRec)
        {
            /* Old service record is unregistered successfully. Now register the new service record */
            if(!instData->newRecUnregCfmRec)
            {
                CsrUint8        *record;
                record = (CsrUint8 *) CsrPmemAlloc(instData->hidSdp.recordLen);
                SynMemCpyS(record, instData->hidSdp.recordLen, instData->hidSdp.record, instData->hidSdp.recordLen);
                
                CsrBtCmSdsRegisterReqSend(instData->myAppHandle, record, instData->hidSdp.recordLen, CSR_BT_CM_CONTEXT_UNUSED);
                instData->newRecUnregCfmRec = TRUE;
            }
            else
            {
                instData->newRecUnregCfmRec = FALSE;
            }
        }
        else if (instData->deactivating)
        {
            if(!instData->deactUnregCfmRec)
            {
                if ((instData->deviceIdSdp.recordHandle != HIDD_SDS_REQUEST_DEVICE) && (instData->deviceIdSdp.recordHandle) && (instData->deviceIdSdp.record))
                {
                    CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->deviceIdSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
                    instData->deactUnregCfmRec = TRUE;
                }
                else
                {
                    CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                    CsrBtHiddCleanUpToIdle(instData);
                }
            }
            else
            {
                instData->deactUnregCfmRec = FALSE;
                CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                CsrBtHiddCleanUpToIdle(instData);
            }
        }
        else
        {
            if(!instData->reconnect)
            {
                CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SDS_REGISTER_FAILED, CSR_BT_SUPPLIER_HIDD);
            }
            else
            {
                CsrBtHiddStatusIndSend(instData,
                                       CSR_BT_HIDD_CONNECT_FAILED,
                                       CSR_BT_CONN_ID_INVALID);
            }
            CsrBtHiddCleanUpToIdle(instData);
        }
    }
    else
    {
        /* just cleanup and return to idle */
        if(!instData->reconnect)
        {
            CsrBtHiddActivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SDS_UNREGISTER_FAILED, CSR_BT_SUPPLIER_HIDD);
        }
        else
        {
            CsrBtHiddStatusIndSend(instData,
                                   CSR_BT_HIDD_UNREGISTER_FAILED,
                                   CSR_BT_CONN_ID_INVALID);
        }
        CsrBtHiddCleanUpToIdle(instData);
    }
}


void CsrBtHiddCmConnectAcceptConnectingHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caConnectAcceptCfm *prim;
    prim = (CsrBtCmL2caConnectAcceptCfm *)instData->pRecvMsg;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, prim->deviceAddr, prim->btConnId);

        if(!CsrBtBdAddrEq(&(instData->bdAddr), &(prim->deviceAddr)))
        {
            CsrBtBdAddrCopy(&(instData->bdAddr), &(prim->deviceAddr));
        }
        if(CSR_BT_HID_CTRL_PSM == prim->localPsm)
        {
            instData->ctrlCh.cid = prim->btConnId;
            instData->ctrlCh.hostMtu = prim->mtu;
            instData->ctrlCh.localMtu = prim->localMtu;
            /* Wait for interrupt channel */
        }
        else
        {
            if(instData->deactivating || instData->cleanUp)
            {
                CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
            }
            else
            {
                instData->intrCh.cid = prim->btConnId;
                instData->intrCh.hostMtu = prim->mtu;
                instData->intrCh.localMtu = prim->localMtu;
                if(instData->ctrlCh.cid)
                {
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECTED,
                                           (CsrBtConnId)instData->ctrlCh.cid);
                    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTED_STATE);
#ifdef CSR_STREAMS_ENABLE
                    CsrBtHiddStreamsRegister(instData, instData->intrCh.cid);
                    CsrBtHiddStreamsRegister(instData, instData->ctrlCh.cid);
#endif
                    csrBtHiddRestoreSavedMessages(instData);
                }
                if(instData->reconnect)
                {
                    CsrSchedTimerCancel(instData->timerId, NULL, NULL);
                    instData->reconnect = FALSE;
                }
            }
        }
    }
    else
    {
        if(!instData->deactivating)
        {
            instData->prevError = prim->resultCode;
            instData->prevErrorSupplier = prim->resultSupplier;
            if(instData->reconnect)
            {
                CsrSchedTimerCancel(instData->timerId, NULL, NULL);
                instData->reconnect = FALSE;
            }
        }
        if(instData->ctrlCh.cid)/* if control channel is already connected disconnect */
        {
            CsrBtCml2caDisconnectReqSend(instData->ctrlCh.cid);
            if(instData->reconnect)
            {
                CsrSchedTimerCancel(instData->timerId, NULL, NULL);
                instData->reconnect = FALSE;
            }
        }
        else
        {
            /* do nothing */
        }
    }
}

void CsrBtHiddCmCancelConnectAcceptConnectingHandler(HiddInstanceDataType *instData)
{
    CsrUint8 flags;
    instData->cancelChCount++;

    if(2 == instData->cancelChCount)
    {/* Keep the sdp record info (if any) so it can be used at a later stage if needed, or un-registered at a later activation */
        HiddSdpInfo tmpSdpInfo = instData->hidSdp;

        if(!instData->cleanUp)
        {
            if(instData->deactivating)
            {
                CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
            }
            else if (instData->reconnect)
            {
                /* Connection cancelled */
                instData->prevError = CSR_BT_RESULT_CODE_HIDD_TIMEOUT;
                instData->prevErrorSupplier = CSR_BT_SUPPLIER_HIDD;
                CsrBtHiddStatusIndSend(instData,
                                       CSR_BT_HIDD_DISCONNECTED,
                                       (CsrBtConnId)instData->ctrlCh.cid);
                csrBtHiddRestoreSavedMessages(instData);
            }
        }
        
        if (instData->hidSdp.record)
        {
            instData->hidSdp.record = NULL;
        }

        flags = instData->hidFlags;
        CsrBtHiddCleanUpToIdle(instData);
        /* Restore the SDP and HID Flags so that they are available at activation */
        instData->hidSdp = tmpSdpInfo;
        instData->hidFlags = flags;
        CSR_LOG_TEXT_INFO((CsrBtHiddLto, 0, "CsrBtHiddCmCancelConnectAcceptConnectingHandler FLAGS:%d",
                           instData->hidFlags));
    }
}

void CsrBtHiddCmConnectConnectingHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caConnectCfm *prim;
    prim = (CsrBtCmL2caConnectCfm *)instData->pRecvMsg;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, prim->deviceAddr, prim->btConnId);

        if(CSR_BT_HID_CTRL_PSM == prim->localPsm)
        {
            instData->ctrlCh.cid = prim->btConnId;
            instData->ctrlCh.hostMtu = prim->mtu;
            instData->ctrlCh.localMtu = prim->localMtu;
            if(instData->deactivating || instData->cleanUp)
            {
                CsrBtCml2caDisconnectReqSend(instData->ctrlCh.cid);
            }
            else
            {
                instData->numOfRetries = 0;
                csrBtHiddConnect(instData, CSR_BT_HID_INTR_PSM);
            }
        }
        else if(CSR_BT_HID_INTR_PSM == prim->localPsm)
        {
            if(instData->deactivating || instData->cleanUp)
            {
                if(instData->ctrlCh.cid)
                {
                    CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
                }
            }
            else
            {
                instData->intrCh.cid = prim->btConnId;
                instData->intrCh.hostMtu = prim->mtu;
                instData->intrCh.localMtu = prim->localMtu;
                if(instData->ctrlCh.cid)
                {
                    if (instData->appRequestedDisconnect)
                    {
                        /* Disconnect was requested by app because of which there was no connection,
                         * inform application of the initial mode change.*/
                        CsrBtHiddModeChangeIndSend(instData, CSR_BT_HIDD_ACTIVE_MODE,
                                                   CSR_BT_RESULT_CODE_HIDD_SUCCESS,
                                                   CSR_BT_SUPPLIER_HIDD);
                    }

                    /* Connection is completed, clear appRequestedDisconnect flag.*/
                    instData->appRequestedDisconnect = FALSE;
                    instData->reconnect = FALSE;
                    instData->numOfRetries = 0;
                    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTED_STATE);
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECTED,
                                           (CsrBtConnId)instData->ctrlCh.cid);
#ifdef CSR_STREAMS_ENABLE
                    CsrBtHiddStreamsRegister(instData, instData->intrCh.cid);
                    CsrBtHiddStreamsRegister(instData, instData->ctrlCh.cid);
#endif
                }
                else
                {
                    /* Control channel not connected, it should have been!!! */
                }
            }
        }
        else
        {
            /* Unknown channel */
        }

    }
    else
    {
        /* if cancel connect request as success */
        if (prim->resultCode == CSR_BT_RESULT_CODE_CM_CANCELLED && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            if(instData->deactivating && (CSR_BT_HID_CTRL_PSM == prim->localPsm))
            {
                CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                CsrBtHiddCleanUpToIdle(instData);
            }
        }
        else
        {
            if(instData->reconnect && (instData->numOfRetries < HIDD_NUM_OF_RECONNECTS) )
            {
                if(CSR_BT_HID_CTRL_PSM == prim->localPsm)
                {
                    if(instData->deactivating || instData->cleanUp)
                    {
                        CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                        CsrBtHiddCleanUpToIdle(instData);
                    }
                    else
                    {
                        instData->timerId = CsrSchedTimerSet( HIDD_CONNECT_ATTEMPT_TIMEOUT,(void (*) (CsrUint16, void *)) connectTimeOut, CSR_BT_HID_CTRL_PSM, (void *) instData);
                    }
                    instData->numOfRetries++;
                }
                else if((CSR_BT_HID_INTR_PSM == prim->localPsm) && (instData->ctrlCh.cid != 0)) 
                    /* If we do not have a ctrl channel, just ignore this cfm, since it will be from before the reconnect was initiated. */
                {
                    if(instData->deactivating || instData->cleanUp)
                    {
                        CsrBtCml2caDisconnectReqSend(instData->ctrlCh.cid);
                    }
                    else
                    {
                        instData->timerId = CsrSchedTimerSet( HIDD_CONNECT_ATTEMPT_TIMEOUT,(void (*) (CsrUint16, void *)) connectTimeOut, CSR_BT_HID_INTR_PSM, (void *) instData);
                    }
                    instData->numOfRetries++;
                }
            }
            else /* !reconnect */
            {
                if(instData->ctrlCh.cid)
                {
                    instData->prevError = prim->resultCode;
                    instData->prevErrorSupplier = prim->resultSupplier;
                    CsrBtCml2caDisconnectReqSend(instData->ctrlCh.cid);
                }
                else if(instData->intrCh.cid)
                {
                    CsrGeneralException(CsrBtHiddLto,
                                        0,
                                        CSR_BT_CM_PRIM,
                                        prim->type,
                                        (CsrUint16) instData->state,
                                        "Only interrupt channel connected");
                }
                else
                {
                    if(!(instData->cleanUp))
                    {
                        if (instData->appRequestedDisconnect)
                        {
                            /* Application had requested for disconnection, send initial mode change to app. */
                            CsrBtHiddModeChangeIndSend(instData, CSR_BT_HIDD_DISCONNECT_MODE, prim->resultCode, prim->resultSupplier);
                            CsrBtHiddCleanUpToIdle(instData);
                        }
                        else
                        {
                            CsrBtHiddStatusIndSend(instData,
                                                   CSR_BT_HIDD_CONNECT_FAILED,
                                                   CSR_BT_CONN_ID_INVALID);
                            CsrBtHiddCleanUpToIdle(instData);
                        }
                    }
                }
            }
        }
    }
}

void CsrBtHiddCmUnregisterSdpUnregisteringHandler(HiddInstanceDataType *instData)
{
    CsrBtCmSdsUnregisterCfm *prim = (CsrBtCmSdsUnregisterCfm *) instData->pRecvMsg;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if(instData->hidSdp.recordHandle == prim->serviceRecHandle)
        {
            instData->hidSdp.recordHandle = 0;
            if ((instData->deviceIdSdp.recordHandle) && (instData->deviceIdSdp.record))
            {
                CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->deviceIdSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
            }
            else if(!(instData->deviceIdSdp.record))
            {
                if(instData->deactivating || instData->cleanUp)
                {
                    if(instData->prevError)
                    {
                        if(instData->deactivating)
                        {
                            CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                        }
                           CsrBtHiddCleanUpToIdle(instData);
                    }
                    else if(instData->intrCh.cid)
                    {
                        CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
                    }
                }
                else if(instData->reconnect && instData->prevError)
                {
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECT_FAILED,
                                           CSR_BT_CONN_ID_INVALID);
                       CsrBtHiddCleanUpToIdle(instData);
                }
                else
                {
                    /* proceed to connected */
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECTED,
                                           (CsrBtConnId)instData->ctrlCh.cid);
                    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTED_STATE);
                    csrBtHiddRestoreSavedMessages(instData);
                }
            }
            else
            {
                /* Abnormal flow */
            }
        }
        else
        {
            instData->deviceIdSdp.recordHandle = 0;
            if(instData->deactivating || instData->cleanUp)
            {
                if(instData->prevError)
                {
                    if(!(instData->cleanUp))
                    {
                        CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                    }
                    CsrBtHiddCleanUpToIdle(instData);
                }
                else if(instData->intrCh.cid)
                {
                    CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
                }
                else
                {
                    /* Abnormal flow */
                }
            }
            else
            {
                if(instData->prevError) /* not success */
                {
                    /* cleanup and return to idle */
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECT_FAILED,
                                           CSR_BT_CONN_ID_INVALID);
                    CsrBtHiddCleanUpToIdle(instData);
                }
                else
                {
                    /* proceed to connected */
                    CsrBtHiddStatusIndSend(instData,
                                           CSR_BT_HIDD_CONNECTED,
                                           (CsrBtConnId)instData->ctrlCh.cid);
                    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTED_STATE);
                    csrBtHiddRestoreSavedMessages(instData);
                }
            }
        }
    }
    else
    {
        /* Send status and disconnect connection */
        instData->prevError = prim->resultCode;
        instData->prevErrorSupplier = prim->resultSupplier;
        if(!instData->deactivating)
        {
                CsrBtHiddStatusIndSend(instData,
                                   CSR_BT_HIDD_UNREGISTER_FAILED,
                                   CSR_BT_CONN_ID_INVALID);
        }
        CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
    }
}

void CsrBtHiddCmDataCfmConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caDataCfm *prim;

    prim = (CsrBtCmL2caDataCfm *)instData->pRecvMsg;

    if(instData->intrCh.cid == prim->btConnId)
    {
        if(instData->intrCh.sendMsg)
        {
            CsrUint8* tmp;

            if((instData->intrCh.sendMsgLength - instData->intrCh.sendMsgOffset) >= (instData->intrCh.hostMtu-1))
            {
                tmp = CsrPmemAlloc(instData->intrCh.hostMtu);

                tmp[0] = CSR_BT_HIDD_DATC;
                tmp[0] |= CSR_BT_HIDD_INPUT_REPORT;

                SynMemCpyS(&(tmp[1]), instData->intrCh.hostMtu-1, &(instData->intrCh.sendMsg[instData->intrCh.sendMsgOffset]), instData->intrCh.hostMtu-1);

                instData->intrCh.sendMsgOffset += (instData->intrCh.hostMtu - 1);
                CsrBtCml2caDataReqSend(instData->intrCh.cid, instData->intrCh.hostMtu, tmp, CSR_BT_CM_CONTEXT_UNUSED);

            }
            else
            {
                CsrUint16 size = instData->intrCh.sendMsgLength - instData->intrCh.sendMsgOffset;
                tmp = CsrPmemAlloc(size + 1);

                tmp[0] = CSR_BT_HIDD_DATC;
                tmp[0] |= CSR_BT_HIDD_INPUT_REPORT;

                if(size)
                {
                    SynMemCpyS(&(tmp[1]), size, &(instData->intrCh.sendMsg[instData->intrCh.sendMsgOffset]), size);
                }

                CsrBtCml2caDataReqSend(instData->intrCh.cid, (CsrUint16)(size+1), tmp, CSR_BT_CM_CONTEXT_UNUSED);
                CsrPmemFree(instData->intrCh.sendMsg);
                instData->intrCh.sendMsg = NULL;
            }
        }
        else
        {
            if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
            {
                CsrBtHiddDataCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
            }
            else
            {
                CsrBtHiddDataCfmSend(instData, prim->resultCode, prim->resultSupplier);
            }
        }
    }
    else
    {
        /* Control channel confirm on control response */
        if(instData->ctrlCh.sendMsg)
        {
            CsrUint8* tmp;

            if((instData->ctrlCh.sendMsgLength - instData->ctrlCh.sendMsgOffset) >= (instData->ctrlCh.hostMtu-1))
            {
                tmp = CsrPmemAlloc(instData->ctrlCh.hostMtu);

                tmp[0] = CSR_BT_HIDD_DATC;
                tmp[0] |= (instData->ctrlCh.sendMsg[0] & 0x0F);

                SynMemCpyS(&(tmp[1]), instData->ctrlCh.hostMtu-1, &(instData->ctrlCh.sendMsg[instData->ctrlCh.sendMsgOffset]), instData->ctrlCh.hostMtu-1);

                instData->ctrlCh.sendMsgOffset += (instData->ctrlCh.hostMtu-1);
                CsrBtCml2caDataReqSend(instData->ctrlCh.cid, instData->ctrlCh.hostMtu, tmp, CSR_BT_CM_CONTEXT_UNUSED);

            }
            else
            {
                CsrUint16 size = instData->ctrlCh.sendMsgLength - instData->ctrlCh.sendMsgOffset;
                tmp = CsrPmemAlloc(size + 1);

                tmp[0] = CSR_BT_HIDD_DATC;
                tmp[0] |= (instData->ctrlCh.sendMsg[0] & 0x0F);;

                if(size)
                {
                    SynMemCpyS(&(tmp[1]), size, &(instData->ctrlCh.sendMsg[instData->ctrlCh.sendMsgOffset]), size);
                }

                CsrBtCml2caDataReqSend(instData->ctrlCh.cid, (CsrUint16)(size+1), tmp, CSR_BT_CM_CONTEXT_UNUSED);
                CsrPmemFree(instData->ctrlCh.sendMsg);
                instData->ctrlCh.sendMsg = NULL;
            }
        }
        else
        {
            /* No action */
        }
    }
}

void CsrBtHiddCmDataIndConnectedHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caDataInd *prim;
    HiddChInfo *pCurrentCh = NULL;

    prim = (CsrBtCmL2caDataInd *)instData->pRecvMsg;

    CsrBtCmL2caDataResSend(prim->btConnId);

    if(prim->btConnId == instData->ctrlCh.cid)
    {
        pCurrentCh = &(instData->ctrlCh);
    }
    else /* if(prim->btConnId == instData->intrCh.cid) */
    {
        pCurrentCh = &(instData->intrCh);
    }

    if(pCurrentCh->recvMsg && pCurrentCh->cid)
    {
        if( prim->length == 1)
        {
            CsrPmemFree(prim->payload);
            if(pCurrentCh->cid == instData->ctrlCh.cid)
            {
                CsrBtHiddControlIndSend(instData, (CsrUint8)(pCurrentCh->recvMsg[0] & 0xF0), (CsrUint8)(pCurrentCh->recvMsg[0] & 0x0F),
                                    pCurrentCh->recvMsgLength, pCurrentCh->recvMsg);
            }
            else
            {
                CsrBtHiddDataIndSend(instData, (CsrUint8)(pCurrentCh->recvMsg[0] & 0x0F), pCurrentCh->recvMsgLength, pCurrentCh->recvMsg);
            }
            pCurrentCh->recvMsg = NULL;
        }
        else if( prim->length <= pCurrentCh->localMtu)
        {
            CsrUint8* tmp;
            CsrUint16 size;

            size = pCurrentCh->recvMsgLength + prim->length - 1;
            tmp = CsrPmemAlloc(size);

            SynMemCpyS(tmp, size, pCurrentCh->recvMsg, pCurrentCh->recvMsgLength);
            SynMemCpyS(&(tmp[pCurrentCh->recvMsgLength]), prim->length - 1, prim->payload + 1, prim->length - 1);

            CsrPmemFree(pCurrentCh->recvMsg);
            pCurrentCh->recvMsg = tmp;
            pCurrentCh->recvMsgLength = size;

            CsrPmemFree(prim->payload);
            if( prim->length < pCurrentCh->localMtu)
            {
                if(pCurrentCh->cid ==  instData->ctrlCh.cid)
                {
                    if(pCurrentCh->reportType)
                    {
                        CsrBtHiddControlIndSend(instData, (CsrUint8)CSR_BT_HIDD_SET_REPORT, pCurrentCh->reportType, pCurrentCh->recvMsgLength, pCurrentCh->recvMsg);
                    }
                    else
                    {
                        /* no such data */
                        CsrBtHiddControlIndSend(instData, (CsrUint8)(pCurrentCh->recvMsg[0] & 0xF0), (CsrUint8)(pCurrentCh->recvMsg[0] & 0x0F),
                                        pCurrentCh->recvMsgLength, pCurrentCh->recvMsg);
                    }
                }
                else
                {
                    CsrBtHiddDataIndSend(instData, (CsrUint8)(pCurrentCh->recvMsg[0] & 0x0F), pCurrentCh->recvMsgLength, pCurrentCh->recvMsg);
                }
                pCurrentCh->recvMsg = NULL;
            }
        }
        else
        {
            /* wrong payload size */
            CsrPmemFree(prim->payload);
        }
    }
    else if(pCurrentCh->cid)
    {
        if( prim->length < pCurrentCh->localMtu )
        {
            if(pCurrentCh->cid ==  instData->ctrlCh.cid)
            {
                if( ((prim->payload[0]) & (CSR_BT_HIDD_CONTROL + CSR_BT_HIDD_VC_UNPLUG)) == (CSR_BT_HIDD_CONTROL + CSR_BT_HIDD_VC_UNPLUG) )
                {
                    instData->disconnectReason = HIDD_CONNECTED_UNPLUG_IND;
                    CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
                    CsrPmemFree(prim->payload);
                }
                else if( ((prim->payload[0]) & CSR_BT_HIDD_SET_REPORT) == CSR_BT_HIDD_SET_REPORT )
                {
                    CsrBtHiddControlIndSend(instData, (CsrUint8)(prim->payload[0] & 0xF0), (CsrUint8)(prim->payload[0] & 0x0F), prim->length, prim->payload);
                }
                else if( ((prim->payload[0]) & CSR_BT_HIDD_GET_REPORT) == CSR_BT_HIDD_GET_REPORT )
                {
                    if(prim->length == 1)
                    {
                        CsrBtHiddControlIndSend(instData, (CsrUint8)(prim->payload[0] & 0xF0), (CsrUint8)(prim->payload[0] & 0x0F), 0, NULL);
                        CsrPmemFree(prim->payload);
                    }
                    else
                    {
                        CsrBtHiddControlIndSend(instData, (CsrUint8)(prim->payload[0] & 0xF0), (CsrUint8)(prim->payload[0] & 0x0F), prim->length, prim->payload);
                    }
                }
                else if( ((prim->payload[0]) & CSR_BT_HIDD_SET_IDLE) == CSR_BT_HIDD_SET_IDLE )
                {
                    CsrBtHiddControlIndSend(instData, (CsrUint8)(prim->payload[0] & 0xF0), (CsrUint8)(prim->payload[0] & 0x0F), prim->length, prim->payload);
                }
                else
                {
                    CsrBtHiddControlIndSend(instData, (CsrUint8)(prim->payload[0] & 0xF0), (CsrUint8)(prim->payload[0] & 0x0F), 0, NULL);
                    CsrPmemFree(prim->payload);
                }
            }
            else if( pCurrentCh->cid ==  instData->intrCh.cid)
            {
                CsrBtHiddDataIndSend(instData, (CsrUint8)(prim->payload[0] & 0x0F), prim->length, prim->payload);
            }
            else
            {
                CsrPmemFree(prim->payload);
            }
        }
        else if(prim->length == pCurrentCh->localMtu)
        {
            if( (pCurrentCh->cid == instData->ctrlCh.cid) && (((prim->payload[0]) & CSR_BT_HIDD_SET_REPORT) == CSR_BT_HIDD_SET_REPORT) )
            {
                pCurrentCh->reportType = (CsrUint8)(prim->payload[0] & 0x0F);
            }
            else
            {
                pCurrentCh->reportType = 0;
            }
            pCurrentCh->recvMsgLength = prim->length;
            pCurrentCh->recvMsg = CsrPmemAlloc(pCurrentCh->recvMsgLength);
            SynMemCpyS(pCurrentCh->recvMsg, pCurrentCh->recvMsgLength, prim->payload, prim->length);

            CsrPmemFree(prim->payload);
        }
        else
        {
            /* Something wrong*/
            CsrPmemFree(prim->payload);
        }
    }
    else
    {
        /* No cid */
        CsrPmemFree(prim->payload);
    }

}

void CsrBtHiddCmModeChangeHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caModeChangeInd *prim;

    prim =  (CsrBtCmL2caModeChangeInd *)instData->pRecvMsg;

    if (!(instData->disconnectReason == HIDD_CONNECTED_CHANGE_MODE || instData->disconnectReason == HIDD_CONNECTED_DISCONNECT_REQ))
    {
        /* This means disconnect was not requested by the application, either through new or old APIs.
         * The mode change indication can be processed.*/
        CsrBtHiddModeChangeIndSend(instData, prim->mode, prim->resultCode, prim->resultSupplier);
    }
}

void CsrBtHiddCmDisconnectHandler(HiddInstanceDataType *instData)
{
    CsrBtCmL2caDisconnectInd *prim;
    CsrBool disconnected;

    prim = (CsrBtCmL2caDisconnectInd *)instData->pRecvMsg;
    disconnected = FALSE;

    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL, instData->bdAddr, prim->btConnId);
#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to L2CA_DISCONNECT_IND. */
        CsrBtCmL2caDisconnectRspSend(prim->l2caSignalId, prim->btConnId);
    }
#endif
    if(instData->intrCh.cid == prim->btConnId)
    {
        instData->intrCh.cid = 0;
        if(instData->ctrlCh.cid)
        {
            if(prim->localTerminated)
            {
                CsrBtCml2caDisconnectReqSend(instData->ctrlCh.cid);
            }
            else
            {
                /* Wait for disconnect on control channel */
            }
        }
        else
        {
            /* Normaly this won't happen */
            disconnected = TRUE;
        }
    }
    else if(instData->ctrlCh.cid == prim->btConnId)
    {
        if(instData->intrCh.cid)
        {
            /* Interrupt channel is not disconnected, it should have been, so now disconnect!!! */
            if(prim->localTerminated)
            {
                CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
            }
            else
            {
                /* Wait for disconnect on interrupt channel */
            }
        }
        else
        {
            disconnected = TRUE;
        }
    }

    if(disconnected)
    {
        if(!instData->deactivating && !instData->cleanUp &&
           ((prim->reasonSupplier == CSR_BT_SUPPLIER_L2CAP_DISCONNECT &&
            prim->reasonCode == L2CA_DISCONNECT_LINK_LOSS) ||
           (prim->reasonSupplier == CSR_BT_SUPPLIER_HCI &&
             prim->reasonCode == HCI_ERROR_OETC_USER)))
        {
            if(instData->hidFlags & CSR_BT_HIDD_FLAGS_RECONNECT_INIT_BIT)
            {
                instData->reconnect = TRUE;
            }
            else
            {
                if(instData->hidFlags & CSR_BT_HIDD_FLAGS_VIRTUAL_CABLE_BIT)
                {
                    instData->reconnect = TRUE;
                }
            }
            if(instData->reconnect && (instData->numOfRetries < HIDD_NUM_OF_RECONNECTS))
            {
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_IDLE_STATE);
                CsrBtHiddStatusIndSend(instData,
                                       CSR_BT_HIDD_RECONNECTING,
                                       instData->ctrlCh.cid);
                CsrBtHiddReactivateIndSend(instData);
            }
            else
            {
                CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_IDLE_STATE);
                CsrBtHiddStatusIndSend(instData,
                                       CSR_BT_HIDD_DISCONNECTED,
                                       instData->ctrlCh.cid);
            }
        }
        else
        {
            switch(instData->state)
            {
            case HIDD_ACCEPT_CONNECTING_STATE:
                {
                    if(instData->prevError || instData->deactivating || instData->cleanUp)
                    {
                        /* Case: Failed but one channel was connected.
                                 Now unregister service records */
                        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                        CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
                    }
                    else
                    {
                        /* Unexpected local disconnect */
                        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_IDLE_STATE);
                        CsrBtHiddStatusIndSend(instData,
                                               CSR_BT_HIDD_DISCONNECTED,
                                               instData->ctrlCh.cid);
                    }
                }
                break;
            case HIDD_CONNECTING_STATE:
                /* FALL THROUGH */
            case HIDD_UNREGISTERING_SDP:
                {
                    if(instData->cleanUp)
                    {
                        CsrBtHiddCleanUpToIdle(instData);
                    }
                    else if(instData->deactivating)
                    {
                        CsrBtHiddDeactivateCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                        CsrBtHiddCleanUpToIdle(instData);
                    }
                    else if(instData->prevError)
                    {
                        /* Case: Failed but one channel was connected.
                                 Cleanup and return to idle */
                        CsrBtHiddStatusIndSend(instData,
                                               CSR_BT_HIDD_CONNECT_FAILED,
                                               instData->ctrlCh.cid);
                        CsrBtHiddCleanUpToIdle(instData);
                    }
                    else
                    {
                        /* Case: DisconnectInd while changing mode
                                 Cleanup and return to idle */
                        CsrBtHiddStatusIndSend(instData,
                                               CSR_BT_HIDD_DISCONNECTED,
                                               instData->ctrlCh.cid);
                        CsrBtHiddCleanUpToIdle(instData);

                    }
                }
                break;
            case HIDD_CONNECTED_STATE:
                {
                    if(instData->cleanUp)
                    {
                        CsrBtHiddCleanUpToIdle(instData);
                    }
                    else if(instData->deactivating)
                    {
                        CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_UNREGISTERING_SDP);
                        instData->deactUnregCfmRec = FALSE;
                        if (prim->localTerminated)
                        {
                            instData->prevError = CSR_BT_RESULT_CODE_HIDD_CONNECTION_TERM_BY_LOCAL_HOST;
                        }
                        else
                        {
                            instData->prevError = CSR_BT_RESULT_CODE_HIDD_CONNECTION_TERM_BY_REMOTE_HOST;
                        }
                        CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->hidSdp.recordHandle, CSR_BT_CM_CONTEXT_UNUSED);
                    }
                    else
                    {
                        if(HIDD_CONNECTED_UNPLUG_IND == instData->disconnectReason)
                        {
                            CsrBtHiddUnplugIndSend(instData, instData->bdAddr, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                            instData->cleanUp = TRUE;
                            CsrBtHiddCleanUpToIdle(instData);
                            instData->disconnectReason = HIDD_CONNECTED_UNPLUG_IND;
                        }
                        else if(HIDD_CONNECTED_UNPLUG_REQ == instData->disconnectReason)
                        {
                            CsrBtHiddUnplugCfmSend(instData, CSR_BT_RESULT_CODE_HIDD_SUCCESS, CSR_BT_SUPPLIER_HIDD);
                            instData->cleanUp = TRUE;
                            CsrBtHiddCleanUpToIdle(instData);
                            instData->disconnectReason = HIDD_CONNECTED_UNPLUG_REQ;
                        }
                        else if(HIDD_CONNECTED_CHANGE_MODE == instData->disconnectReason)
                        {
                            CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_NOT_CONNECTED_STATE);
                            /* Note down the application requested disconnection.*/
                            instData->appRequestedDisconnect = TRUE;
                            CsrBtHiddModeChangeIndSend(instData,
                                                       CSR_BT_HIDD_DISCONNECT_MODE,
                                                       CSR_BT_RESULT_CODE_HIDD_SUCCESS,
                                                       CSR_BT_SUPPLIER_HIDD);
                            instData->disconnectReason = HIDD_DISCONNECT_REASON_UNSET;
                        }
                        else
                        {
                            /* Either the disconnection was requested by the application or this is an unexpected
                             * disconnection which we have received, inform this to application.*/
                            CSR_BT_HIDD_STATE_CHANGE(instData->state,
                                                     HIDD_CONNECTED_DISCONNECT_REQ == instData->disconnectReason ?
                                                     HIDD_NOT_CONNECTED_STATE: 
                                                     HIDD_IDLE_STATE);
                            CsrBtHiddStatusIndSend(instData,
                                                   CSR_BT_HIDD_DISCONNECTED,
                                                   instData->ctrlCh.cid);
                            instData->disconnectReason = HIDD_DISCONNECT_REASON_UNSET;
                        }
                    }
                }
                break;
            case HIDD_NOT_CONNECTED_STATE:
                {
                    CsrBtHiddModeChangeIndSend(instData, CSR_BT_HIDD_DISCONNECT_MODE,
                                               instData->prevError, instData->prevErrorSupplier);
                    CsrBtHiddCleanUpToIdle(instData);
                }
                break;
            default:
                /* Unexspected state */
                break;
            }
        }
        instData->ctrlCh.cid = CSR_BT_CONN_ID_INVALID;
    }
}

void CsrBtHiddCmDisconnectIgnoreHandler(HiddInstanceDataType *instData)
{
    CSR_UNUSED(instData);
    /*
     * HIDD_IDLE_STATE
 */
}

#ifdef INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS
void CsrBtHiddSecurityInHandler(HiddInstanceDataType *instData)
{
    CsrBtResultCode rval;
    CsrBtHiddSecurityInReq *prim;

    prim = (CsrBtHiddSecurityInReq*)instData->pRecvMsg;

    rval = CsrBtScSetSecInLevel(&instData->secIncoming, prim->secLevel,
        CSR_BT_HIDD_MANDATORY_SECURITY_INCOMING,
        CSR_BT_HIDD_DEFAULT_SECURITY_INCOMING,
        CSR_BT_RESULT_CODE_HIDD_SUCCESS,
        CSR_BT_RESULT_CODE_HIDD_UNACCEPTABLE_PARAMETER);

    CsrBtHiddSecurityInCfmSend(prim->appHandle, rval, CSR_BT_SUPPLIER_HIDD);
}

void CsrBtHiddSecurityOutHandler(HiddInstanceDataType *instData)
{
    CsrBtResultCode rval;
    CsrBtHiddSecurityOutReq *prim;

    prim = (CsrBtHiddSecurityOutReq*)instData->pRecvMsg;

    rval = CsrBtScSetSecOutLevel(&instData->secOutgoing, prim->secLevel,
        CSR_BT_HIDD_MANDATORY_SECURITY_OUTGOING,
        CSR_BT_HIDD_DEFAULT_SECURITY_OUTGOING,
        CSR_BT_RESULT_CODE_HIDD_SUCCESS,
        CSR_BT_RESULT_CODE_HIDD_UNACCEPTABLE_PARAMETER);

    CsrBtHiddSecurityOutCfmSend(prim->appHandle, rval, CSR_BT_SUPPLIER_HIDD);
}
#endif /* INSTALL_HIDD_CUSTOM_SECURITY_SETTINGS */

void HiddConnectReqHandler(HiddInstanceDataType *instData)
{
    /* Send connect request.*/
    CSR_BT_HIDD_STATE_CHANGE(instData->state, HIDD_CONNECTING_STATE);
    instData->numOfRetries = 0;
    instData->reconnect = TRUE;
    csrBtHiddConnect(instData, CSR_BT_HID_CTRL_PSM);
}

void HiddDisconnectReqHandler(HiddInstanceDataType *instData)
{
    /* Send disconnect request.*/
    instData->disconnectReason = HIDD_CONNECTED_DISCONNECT_REQ;
    CsrBtCml2caDisconnectReqSend(instData->intrCh.cid);
}

