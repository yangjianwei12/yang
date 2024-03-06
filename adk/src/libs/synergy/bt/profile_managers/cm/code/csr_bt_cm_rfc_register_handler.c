/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_util.h"

static void csrBtCmRfcRegisterCfmMsgSend(CsrSchedQid theAppHandle, CsrUint8 theServer,
                            CsrBtResultCode resultCode, CsrBtSupplier resultSupplier,
                            CsrUint16 context)
{/* Send a CSR_BT_CM_REGISTER_CFM signal to the application */
    CsrBtCmRegisterCfm *cmPrim;

    cmPrim    = (CsrBtCmRegisterCfm *)CsrPmemAlloc(sizeof(CsrBtCmRegisterCfm));
    cmPrim->type                = CSR_BT_CM_REGISTER_CFM;
    cmPrim->resultCode          = resultCode;
    cmPrim->resultSupplier      = resultSupplier;
    cmPrim->serverChannel       = theServer;
    cmPrim->context             = context;
    CsrBtCmPutMessage(theAppHandle, cmPrim);
}

void CsrBtCmRfcRegisterReqHandler(cmInstanceData_t *cmData)
{/* This event allows the above profile to register a protocol handle with
    RFCOMM. RFCOMM will return a RFC_REGISTER_CFM primitive with an
    assigned server channel number. The registered protocol handle will be
    used for notifying the profile of the arrival of events for the given server
    channel. */
    CsrBtCmRegisterReq *cmPrim = (CsrBtCmRegisterReq *) cmData->recvMsgP;
    CsrUint16 regContext;
    CsrUint8 serverChannel;

    /* If connection handling is enabled from application, the phandle shall be used as context. */
    regContext = (cmPrim->optionsMask & CM_REGISTER_OPTION_APP_CONNECTION_HANDLING) ? cmPrim->phandle : 0;

    cmData->smVar.appHandle                     = cmPrim->phandle;
    cmData->smVar.arg.reg.context               = cmPrim->context;
    cmData->smVar.arg.reg.registeringSrvChannel = (cmPrim->optionsMask & CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL) ?
                            CSR_BT_CM_SERVER_CHANNEL_DONT_CARE : cmPrim->serverChannel;

    serverChannel = (cmPrim->serverChannel != CSR_BT_CM_SERVER_CHANNEL_DONT_CARE) ?
                            cmPrim->serverChannel : RFC_INVALID_SERV_CHANNEL;

    RfcRegisterReqSend(CSR_BT_CM_IFACEQUEUE, 0, regContext, serverChannel);
}

void CsrBtCmRfcUnRegisterReqMsgSend(cmInstanceData_t *cmData)
{
    CsrBtCmUnregisterReq *cmPrim = (CsrBtCmUnregisterReq *) cmData->recvMsgP;

    RfcUnregisterReqSend(cmPrim->serverChannel);
}

void CsrBtCmRfcRegisterCfmHandler(cmInstanceData_t *cmData)
{ /* This event will be used to indicate to the higher layer that its
     previous registration of a protocol handle with an RFC_REGISTER_REQ
     event had been accepted (if accept is set to True) or denied (if accept
     is set to false). */
    RFC_REGISTER_CFM_T *rfcPrim = (RFC_REGISTER_CFM_T *) cmData->recvMsgP;

    if (rfcPrim->accept)
    { /* RFC_REGISTER_REQ event had been accepted. The received
         serverchannel number is sent to the profile. */
        if (cmData->smVar.arg.reg.registeringSrvChannel == CSR_BT_CM_SERVER_CHANNEL_DONT_CARE ||
            cmData->smVar.arg.reg.registeringSrvChannel == rfcPrim->loc_serv_chan)
        {/* Either the desired server channel is used, or the application did not request a particular server channel */
            csrBtCmRfcRegisterCfmMsgSend(cmData->smVar.appHandle,
                                         rfcPrim->loc_serv_chan,
                                         CSR_BT_RESULT_CODE_CM_SUCCESS,
                                         CSR_BT_SUPPLIER_CM,
                                         cmData->smVar.arg.reg.context);
        }
        else
        {/* Requested server channel is not assigned and CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL
            was not set in request. Un-register assigned server channel and indicate the error to the application */
            RfcUnregisterReqSend(rfcPrim->loc_serv_chan);
            csrBtCmRfcRegisterCfmMsgSend(cmData->smVar.appHandle,
                                         CSR_BT_NO_SERVER,
                                         CSR_BT_RESULT_CODE_CM_SERVER_CHANNEL_ALREADY_USED,
                                         CSR_BT_SUPPLIER_CM,
                                         cmData->smVar.arg.reg.context);
        }
    }
    else
    { /* RFC_REGISTER_REQ event had been denied */
        csrBtCmRfcRegisterCfmMsgSend(cmData->smVar.appHandle,
                                     CSR_BT_NO_SERVER,
                                     (CsrBtResultCode) HCI_ERROR_SCM_INSUFFICIENT_RESOURCES,
                                     CSR_BT_SUPPLIER_RFCOMM,
                                     cmData->smVar.arg.reg.context);
    }
    cmData->smVar.arg.reg.registeringSrvChannel = CSR_BT_CM_SERVER_CHANNEL_DONT_CARE;
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}

#endif /* #ifndef EXCLUDE_CSR_BT_RFC_MODULE */
