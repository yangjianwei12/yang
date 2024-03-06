/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_env_prim.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hfg_streams.h"
#endif /* CSR_STREAMS_ENABLE */

/* Cancelled connection */
void CsrBtHfgConnectCmCancelAcceptConnectCfmHandler(HfgInstance_t *inst)
{
    CsrBtCmCancelAcceptConnectCfm *prim;

    prim = (CsrBtCmCancelAcceptConnectCfm*)inst->msg;
    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if(inst->pendingDisconnect)
        {
            CsrBtHfgSendHfgDisconnectInd(inst, TRUE, prim->resultCode, prim->resultSupplier);
            inst->pendingDisconnect = FALSE;
            CsrBtHfgLinkConnectFailed(inst);
        }
        else if(inst->pendingCancel)
        {
            CsrBtHfgSendHfgServiceConnectInd(inst, CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HFG);
            inst->pendingCancel = FALSE;
            CsrBtHfgLinkConnectFailed(inst);
        }
        else
        {
            CsrBtHfgStartSdcSearch(inst);
        }
    }
    else
    {
        /* If not success, wait for CSR_BT_CM_CONNECT_ACCEPT_CFM */
        ;
    }
}

/* Disconnect indication handler */
void CsrBtHfgXCmDisconnectIndHandler(HfgInstance_t *inst)
{
    CsrBtCmDisconnectInd *prim;
    prim = (CsrBtCmDisconnectInd*)inst->msg;
#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to CSR_BT_CM_DISCONNECT_IND. */
        CsrBtCmRfcDisconnectRspSend(prim->btConnId);
    }
#endif
    CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,inst->address,
                    prim->btConnId);
    /* Stop both ringing and LP changer */
    CsrBtHfgRingStop(inst);

    if (inst->deRegisterTimer != 0)
    {
        void *mv;
        CsrUint16 mi;
        CsrSchedTimerCancel(inst->deRegisterTimer,&mi,&mv);
        inst->deRegisterTimer = 0;
    }
    /* Clean local save queue */
    CsrBtHfgEmptySaveQueue(inst);

    if(inst->pendingDisconnect)
    {
        if(!prim->status)
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
        else
        {
            CsrBtHfgLinkDisconnect(inst, prim->localTerminated, CSR_BT_RESULT_CODE_HFG_SUCCESS, CSR_BT_SUPPLIER_HFG);
        }
    }
    else if(inst->pendingCancel)
    {
        if(!prim->status)
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
        else
        {
            if(inst->state == Connected_s)
            {
                CsrBtHfgLinkDisconnect(inst, prim->localTerminated, CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HFG);
            }
            else
            {

                CsrBtHfgSendHfgServiceConnectInd(inst, CSR_BT_RESULT_CODE_HFG_CANCELLED_CONNECT_ATTEMPT, CSR_BT_SUPPLIER_HFG);
                CsrBtHfgLinkConnectFailed(inst);
            }
        }
    }
    else
    {
        /* No disconnect req received so disconnect must be result
         * of LP fail. If result is 'link loss' the link is
         * unexpected terminated by remote side => start scanning
         * and inform app layer and let it decide how to
         * continue. Peer side has intentionally released the
         * connection => send info to app layer and await further
         * action */
        inst->pendingSco = FALSE;
        CsrBtHfgLinkDisconnect(inst, prim->localTerminated, prim->reasonCode, prim->reasonSupplier);
    }
}

/* AT Command responses are terminated by <CR><LF>, go backwards through the received AT Command
* response and return TRUE if <CR> is found.*/
static CsrBool csrBtHfgCompletePacketReceived(CsrUint8 *atCmd, CsrUint16 len)
{
	CsrUintFast16   i;

	for (i = len - 1; i>0; i--)
	{
		if (atCmd[i] == '\r')
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* RFCOMM data received */
void CsrBtHfgXCmDataIndHandler(HfgInstance_t *inst)
{
	CsrBtCmDataInd *cmPrim;
	cmPrim = (CsrBtCmDataInd*)inst->msg;

#ifndef CSR_STREAMS_ENABLE
	CsrBtCmDataResSend(inst->hfgConnId);
#endif /* CSR_STREAMS_ENABLE */

	/* AT commands may be divided into several packets this may be
	* continuation of a command where the first part is already
	* stored */
	if (inst->atCmd == NULL)
	{
		/* No data is currently stored from a previous command. */
		if (cmPrim->payloadLength <= CSR_BT_AT_COMMAND_MAX_LENGTH)
		{
			inst->atLen = cmPrim->payloadLength;
		}
		else
		{
			inst->atLen = CSR_BT_AT_COMMAND_MAX_LENGTH;
		}
		inst->atCmd = CsrPmemAlloc(inst->atLen);
		SynMemCpyS(inst->atCmd, inst->atLen, cmPrim->payload, inst->atLen);
	}
	else
	{
		/* Data is already received. Allocate space for the new data
		* and append */
		CsrUint16 currentAtCmdSize = inst->atLen;
		CsrUint16 payloadToCopy = cmPrim->payloadLength;
		CsrUint8 *p;

		/* If the command exceeds the MAX Command length, truncate the
		* AT command and process it. Accidentally unterminated AT Commands
		* should be handled gracefully or rejected as an unknown. */

        if(cmPrim->payloadLength > (CSR_BT_AT_COMMAND_MAX_LENGTH - currentAtCmdSize))
        {
            inst->atLen = CSR_BT_AT_COMMAND_MAX_LENGTH;
            payloadToCopy = CSR_BT_AT_COMMAND_MAX_LENGTH - currentAtCmdSize;
        }
        else
        {
            inst->atLen = currentAtCmdSize + cmPrim->payloadLength; 
        }

		p = (CsrUint8*)CsrPmemAlloc(inst->atLen);

		/* copy existing data and append it */
		SynMemCpyS(p, inst->atLen, inst->atCmd, currentAtCmdSize);
		SynMemCpyS((p + currentAtCmdSize), payloadToCopy, cmPrim->payload, payloadToCopy);

		/* Data from inst and signal copied, so free it */
		CsrPmemFree(inst->atCmd);
		inst->atCmd = p;
	}

	if (csrBtHfgCompletePacketReceived(inst->atCmd, inst->atLen)
		|| inst->atLen >= CSR_BT_AT_COMMAND_MAX_LENGTH)
	{
		CsrBtHfgAtInterpret(inst);
	}
}

/* Data was sent to the CM */
void CsrBtHfgXCmDataCfmHandler(HfgInstance_t *inst)
{
    CsrUint16 class;
    void *msg;

    if (inst->waitForDataCfm != 0)
    {
        inst->waitForDataCfm--;
        if (inst->waitForDataCfm == 0)
        {
            HFG_CHANGE_STATE(inst->atState, At10End_s);
            csrBtHfgSlcHfgDone(inst);
        }
    }
    /* Data was sent - pop from queue and send again, otherwise, open
     * up for transmission */
    if(CsrMessageQueuePop(&inst->cmTxQueue, &class, &msg))
    {
#ifdef CSR_STREAMS_ENABLE
        CsrBtCmDataReq *cmPrim = msg;
        CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(cmPrim->btConnId),
                           RFCOMM_ID,
                           cmPrim->payloadLength,
                           cmPrim->payload);
        SynergyMessageFree(class, cmPrim);
#else /* !CSR_STREAMS_ENABLE */
        CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, class, msg);
#endif /* CSR_STREAMS_ENABLE */           
    }
    else
    {
        inst->cmTxReady = TRUE;
    }
}

