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
#include "csr_bt_cm_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"

#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"


/* Update service records and connections */
CsrBool CsrBtHfgUpdateService(HfgMainInstance_t *inst)
{
    if(CsrBtHfgRecordUpdate(inst))
    {
        return TRUE;
    }
    if(CsrBtHfgConnectionUpdate(inst))
    {
        return TRUE;
    }
    return FALSE;
}

/* Link connection established, perform standard actions */
void CsrBtHfgLinkConnectSuccess(HfgInstance_t *inst)
{
    HfgMainInstance_t *mainInst;
    mainInst = CsrBtHfgGetMainInstance(inst);

    /* Setup a few "minor" settings */
    mainInst->activeConnection = CSR_BT_HFG_NO_CONID;

    /* HFG must accept incoming SCO */
    if(inst->linkType == CSR_BT_HFG_CONNECTION_HFG)
    {
        CsrBtCmScoAcceptConnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                                  inst->hfgConnId,
                                  CSR_BT_COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY,
                                  CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH,
                                  CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH,
                                  CSR_BT_SCO_DEFAULT_ACCEPT_MAX_LATENCY,
                                  CSR_BT_SCO_DEFAULT_ACCEPT_VOICE_SETTINGS,
                                  CSR_BT_SCO_DEFAULT_ACCEPT_RE_TX_EFFORT);
    }

    /* Start record and connection update */
    CsrBtHfgUpdateService(mainInst);
}

/* Link connection failed, perform standard actions */
void CsrBtHfgLinkConnectFailed(HfgInstance_t *inst)
{
    HfgMainInstance_t *mainInst;
    mainInst = CsrBtHfgGetMainInstance(inst);

    /* Mark this channel as not-accepting */
    CsrBtHfgConnectionConfirm(CsrBtHfgGetMainInstance(inst),
                         inst->serverChannel);

    /* Active setup connection is current */
    mainInst->activeConnection = inst->index;

    /* Clear connection instance */
    CsrBtHfgInitializeConnInstance(mainInst, inst->index);
    HFG_CHANGE_STATE(inst->state, Activate_s);

    /* Start record and connection update */
    if (mainInst->state != MainDeactivate_s)
    {
        CsrBtHfgUpdateService(mainInst);
    }
}

/* Link disconnected, perform standard actions */
void CsrBtHfgLinkDisconnect(HfgInstance_t *inst, CsrBool localTerminated, CsrBtResultCode reasonCode, CsrBtSupplier reasonSupplier)
{
    HfgMainInstance_t *mainInst;
    mainInst = CsrBtHfgGetMainInstance(inst);

    /* Notify application and then clean instance */
    CsrBtHfgSendHfgDisconnectInd(inst, localTerminated, reasonCode, reasonSupplier);
    CsrBtHfgInitializeConnInstance(mainInst, inst->index);
    HFG_CHANGE_STATE(inst->state, Activate_s);

    /* Start record and connection update */
    CsrBtHfgUpdateService(mainInst);
}

/* De Register timeout function*/
void CsrBtHfgDeRegisterTimeout(CsrUint16 mi, void *mv)
{
    HfgMainInstance_t *inst = (HfgMainInstance_t *)mv;
    CsrUintFast8 i;
    CsrUint8 sc = (CsrUint8)mi;
    HfgInstance_t *link;
    CsrBool deRegister = FALSE;

    /* First find out whether the link is still connected or not */
    for(i=0; i<CSR_BT_HFG_NUM_SERVERS; i++)
    {
        link = &(inst->linkData[i]);
        if((link->serverChannel == sc) && (link->state == Connected_s))
        {
            link->deRegisterTimer = 0;
            deRegister = TRUE;
            break;
        }
    }
    /* If still connected, and the service record is still registered, 
       unregister it */
    if (deRegister)
    {
        for(i=0; (i<CSR_BT_HFG_NUM_CHANNELS && deRegister); i++)
        {
            if (inst->service[i].chan == sc)
            {
                CsrBtCmSdsUnRegisterReqSend(CSR_BT_HFG_IFACEQUEUE,inst->service[i].recHandle, CSR_BT_CM_CONTEXT_UNUSED);
                deRegister = FALSE;
            }
        }
    }

}

/* Connection guard function */
void CsrBtHfgConnectionTimeout(CsrUint16 mi, void *mv)
{   
    HfgInstance_t *inst = (HfgInstance_t *)mv;
    CSR_UNUSED(mi);

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgConnectionTimeout")); 
    
    /* Disconnect and let the app know */
    CsrBtCmDisconnectReqSend(inst->hfgConnId);
}

/* Ring timeout function */
void CsrBtHfgRingTimeout(CsrUint16 mi, void *mv)
{
    HfgInstance_t *inst;
    inst = (HfgInstance_t*)mv;

    /* Timer was fired */
    inst->ringTimer = 0;

    CsrBtHfgHandleRingEvent(inst);
    CSR_UNUSED(mi);
}

/* Handle ring event */
void CsrBtHfgHandleRingEvent(HfgInstance_t *inst)
{
    /* Don't decrease if ringing forever and don't underflow */
    if((inst->ringMsg->numOfRings > 0) &&
       (inst->ringMsg->numOfRings != 255))
    {
        inst->ringMsg->numOfRings--;
    }

    /* Make sure ring-count has not expired */
    if(inst->ringMsg->numOfRings != 0)
    {
        if ((inst->linkType != CSR_BT_HFG_CONNECTION_AG) ||
            (inst->ind.other[CSR_BT_HFG_INBAND_RINGING] == 0))
        {/* Update for HSP BT 2.1: do not send ring if in-band ringing is enabled and it is a HSP connection*/
         /* Send RING before sending CLIP!!*/
            CsrBtHfgSendAtRing(inst);
        }
        /* Send CLIP? */
        if((inst->ind.other[CSR_BT_HFG_SET_CLIP]) && (inst->ringMsg->number != NULL))
        {
            CsrBtHfgSendAtClip(inst,
                                            inst->ringMsg->number,
                                            inst->ringMsg->numType);
        }

    }

    if(inst->ringMsg->numOfRings == 0)
    {
        /* Stop ringing? */
        CsrBtHfgSendHfgRingCfm(inst);
        CsrBtHfgRingStop(inst);
    }
    else
    {
        /* Reschedule ring */
        inst->ringTimer = CsrSchedTimerSet((CsrTime)(inst->ringMsg->repetitionRate * 1000000),
                                         CsrBtHfgRingTimeout,
                                         0,
                                         (void*)inst);
    }
}

/* Stop ring timer */
void CsrBtHfgRingStop(HfgInstance_t *inst)
{
    /* Kill timer */
    if(inst->ringTimer != 0)
    {
        CsrUint16 mi;
        void *mv;
        CsrSchedTimerCancel(inst->ringTimer,
                           &mi,
                           &mv);
        inst->ringTimer = 0;
    }

    /* Kill message */
    if(inst->ringMsg)
    {
        CsrPmemFree(inst->ringMsg->number);
        CsrPmemFree(inst->ringMsg->name);
        CsrPmemFree(inst->ringMsg);
        inst->ringMsg = NULL;
    }
}

/* Save message on global save queue */
void CsrBtHfgMainSaveMessage(HfgMainInstance_t *inst)
{
    CsrMessageQueuePush(&inst->saveQueue, inst->msgClass, inst->msg);
    inst->msg = NULL;
    inst->msgClass = 0;
    inst->restoreFlag = FALSE;
}

/* Save message on connection save queue */
void CsrBtHfgSaveMessage(HfgInstance_t *inst)
{
    CsrMessageQueuePush(&inst->saveQueue, inst->msgClass, inst->msg);
    inst->msg = NULL;
    inst->msgClass = 0;
    inst->restoreFlag = FALSE;
    ((HfgMainInstance_t *)inst->main)->msg = NULL;
    ((HfgMainInstance_t *)inst->main)->msgClass = 0;
    ((HfgMainInstance_t *)inst->main)->restoreFlag = FALSE;
}

/* Empty local save queue */
void CsrBtHfgEmptySaveQueue(HfgInstance_t *inst)
{
    CsrUint16 mi;
    void *mv;

    if(inst->restoreFlag)
    {
        while(CsrMessageQueuePop(&(inst->saveQueue),
                         &mi,
                         &mv))
        {
            CsrBtHfgFreeMessage(mi, mv);
        }
        inst->restoreFlag = FALSE;
    }
}

/* Initialize status indicators */
void CsrBtHfgInitializeIndicators(HfgInstance_t *inst)
{
    HfgMainInstance_t *mainInst;
    CsrUintFast8 i;

    /* Copy all indicators from global */
    mainInst = CsrBtHfgGetMainInstance(inst);

    for(i=0; i<CSR_BT_CIEV_NUMBER_OF_INDICATORS; i++)
    {
        inst->ind.ciev[i] = mainInst->ind.ciev[i];
        inst->ind.bia[i] = 1;
    }
    for(i=0; i<CSR_BT_HFG_NUM_OF_SETTINGS; i++)
    {
        inst->ind.other[i] = mainInst->ind.other[i];
    }
}

/* Initialize connection instance */
void CsrBtHfgInitializeConnInstance(HfgMainInstance_t *inst, CsrUint8 index)
{
    HfgInstance_t *link;
    link = &(inst->linkData[index]);

    /* Empty the save-queue. Pending messages must *NOT* be freed here
     * as they are the same pointer as in the main instance! */
    CsrBtHfgEmptySaveQueue(link);

    /* Stop timers */
    CsrBtHfgRingStop(link);

    if (link->deRegisterTimer != 0)
    {
        void *mv;
        CsrUint16 mi;
        CsrSchedTimerCancel(link->deRegisterTimer,&mi,&mv);
        link->deRegisterTimer = 0;
    }
    /* Stop AT-sequence CHLD and CKPD timers */
    if(link->atSlcTimer != 0)
    {
        void *mv;
        CsrUint16 mi;
        CsrSchedTimerCancel(link->atSlcTimer,
                           &mi,
                           &mv);
        link->atSlcTimer = 0;
    }
    CsrPmemFree(link->atCmd);
    /* Empty the TX list */ 
    if (link->cmTxQueue)
    {
        CsrUint16 class;
        void *msg;

        while(CsrMessageQueuePop(&link->cmTxQueue, &class, &msg))
        {
            CsrPmemFree(((CsrBtCmDataReq*)msg)->payload);
            CsrPmemFree(msg);
        }
    }
    if (link->conGuardTimer != 0)
    {
        void *mv;
        CsrUint16 mi;
        CsrSchedTimerCancel(link->conGuardTimer ,&mi, &mv);
        link->conGuardTimer = 0;
    }
    if (link->atResponseTimer != 0)
    {
        CsrSchedTimerCancel(link->atResponseTimer, NULL, NULL);
        link->atResponseTimer = 0;
    }
    if(link->remoteHfIndicatorList.first != NULL)
    {
        CsrCmnListDeinit(&link->remoteHfIndicatorList);
    }
    CsrCmnListInit(&(link->remoteHfIndicatorList), 0, NULL, NULL);
    /* Zero-initialize everything */
    CsrMemSet(link, 0, sizeof(HfgInstance_t));

    /* Setup the few special members */
    link->main                     = inst;
    link->index                    = index;
    HFG_CHANGE_STATE(link->state, Unused_s);
    HFG_CHANGE_STATE(link->oldState, Unused_s);
    link->serverChannel            = CSR_BT_NO_SERVER;
    link->scoHandle                = CSR_SCHED_QID_INVALID;
    link->cmTxReady                = TRUE;
    link->scoWantedState           = TRUE;

    /* Setup default eSCO settings */
    link->scoParams.txBandwidth    = CSR_BT_ESCO_DEFAULT_CONNECT_TX_BANDWIDTH;
    link->scoParams.rxBandwidth    = CSR_BT_ESCO_DEFAULT_CONNECT_RX_BANDWIDTH;
    link->scoParams.maxLatency     = CSR_BT_ESCO_DEFAULT_CONNECT_MAX_LATENCY;
    link->scoParams.voiceSettings  = CSR_BT_ESCO_DEFAULT_CONNECT_VOICE_SETTINGS;
    link->scoParams.audioQuality   = CSR_BT_ESCO_DEFAULT_CONNECT_AUDIO_QUALITY;
    link->scoParams.reTxEffort     = CSR_BT_ESCO_DEFAULT_CONNECT_RE_TX_EFFORT;

    /* Reset indicators */
    CsrBtHfgInitializeIndicators(link);

    link->selectedQceCodecId = CSR_BT_HFG_QCE_UNSUPPORTED;
}

/* Reset profile for idle state */
void CsrBtHfgResetProfile(HfgMainInstance_t *inst)
{
    CsrUint16 mi;
    CsrUintFast8 i;
    void *mv;
    HfgService_t service[CSR_BT_HFG_NUM_CHANNELS];
    void *links[CSR_BT_HFG_NUM_SERVERS];
    CsrUint8 tmpAtCmdSettings[CSR_BT_HFG_NUMBER_AT_CMD];
    void *sdcTemp1 = inst->sdpHfgSearchConData;
    void *sdcTemp2 = inst->sdpHfgSearchData;
    CsrBool tmpInitFlag = inst->initSequenceDone;
#ifdef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
    CsrUint16 secIncoming;
    CsrUint16 secOutgoing;
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS */

    /* Pop everything off global queue */
    if(inst->restoreFlag)
    {
        while(CsrMessageQueuePop(&(inst->saveQueue),
                         &mi,
                         &mv))
        {
            CsrBtHfgFreeMessage(mi, mv);
        }
    }
    /* Free local HF indicator list memory */
    if(inst->localHfIndicatorList != NULL)
    {
        CsrPmemFree(inst->localHfIndicatorList);
        inst->localHfIndicatorList = NULL;
        inst->indCount = 0;
    }
    /* Free pending message */
    if(inst->msg != NULL)
    {
        CsrBtHfgFreeMessage(inst->msgClass, inst->msg);
    }

    /* Reset connection instances */
    for(i=0; i<CSR_BT_HFG_NUM_SERVERS; i++)
    {
        CsrBtHfgInitializeConnInstance(inst,(CsrUint8) i);
    }

    /* Backup server channels and links */
    SynMemCpyS(service, sizeof(HfgService_t)*CSR_BT_HFG_NUM_CHANNELS, inst->service, sizeof(HfgService_t)*CSR_BT_HFG_NUM_CHANNELS);
    SynMemCpyS(links, sizeof(void*)*CSR_BT_HFG_NUM_SERVERS, inst->linkData, sizeof(void*)*CSR_BT_HFG_NUM_SERVERS);
    
#ifdef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
    secIncoming = inst->secIncoming;
    secOutgoing = inst->secOutgoing;
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS*/

    SynMemCpyS(tmpAtCmdSettings, CSR_BT_HFG_NUMBER_AT_CMD, inst->HfgSendAtToApp,CSR_BT_HFG_NUMBER_AT_CMD);

    /* Zero-initialize everything */
    CsrMemSet(inst, 0, sizeof(HfgMainInstance_t));

    /* Make sure not to remove some useful pointers and create heap leaks....*/
    inst->sdpHfgSearchConData = sdcTemp1;
    inst->sdpHfgSearchData = sdcTemp2;
    
#ifdef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
    inst->secIncoming = secIncoming;
    inst->secOutgoing = secOutgoing;
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS */

    inst->initSequenceDone = tmpInitFlag;
    SynMemCpyS(inst->HfgSendAtToApp, CSR_BT_HFG_NUMBER_AT_CMD, tmpAtCmdSettings,CSR_BT_HFG_NUMBER_AT_CMD);

    /* Setup special members */
    inst->appHandle                = CSR_SCHED_QID_INVALID;
    inst->maxConnections           = CSR_BT_HFG_NUM_SERVERS;
    inst->activeService            = CSR_BT_HFG_NO_CONID;
    inst->activeConnection         = CSR_BT_HFG_NO_CONID;

    /* Clear special arrays */
    CsrMemSet(&(inst->ind), 0, sizeof(HfgIndicators_t));

    /* Restore backup server channels */
    SynMemCpyS(inst->service, sizeof(HfgService_t)*CSR_BT_HFG_NUM_CHANNELS, service, sizeof(HfgService_t)*CSR_BT_HFG_NUM_CHANNELS);
    SynMemCpyS(inst->linkData, sizeof(void*)*CSR_BT_HFG_NUM_SERVERS, links, sizeof(void*)*CSR_BT_HFG_NUM_SERVERS);

    /* Setup service record/server channel placeholder */
    for(i=0; i<CSR_BT_HFG_NUM_CHANNELS; i++)
    {
        inst->service[i].registered = 0;
        inst->service[i].accepting = 0;
        inst->service[i].recHandle = 0;
    }

    /* Set default service name */
    CsrStrLCpy((CsrCharString*)(inst->localName),
                CSR_BT_HFG_SERVICE_NAME,
                CSR_BT_MAX_FRIENDLY_NAME_LEN + 1);
    CsrUtf8StrTruncate(inst->localName, CSR_BT_MAX_FRIENDLY_NAME_LEN);
}

/* Free message and member-data if any */
void CsrBtHfgFreeMessage(CsrUint16 class, void *msg)
{
    switch(class)
    {
        case CSR_BT_CM_PRIM:
        {
            CsrBtCmFreeUpstreamMessageContents(class, msg);
            break;
        }

        case CSR_BT_HFG_PRIM:
        {
            CsrBtHfgFreeDownstreamMessageContents(class, msg);
            break;
        }
        case CSR_SCHED_PRIM:
            {
                CsrEnvPrim *primType;

                primType = (CsrEnvPrim*)msg;
                switch (*primType)
                {
                    case CSR_CLEANUP_IND:
                        {
                            /* Yet nothing to be done for cleanup when deinit */
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
                break;
            }
        default:
            {
                CsrGeneralException(CsrBtHfgLto,
                                    0,
                                    class,
                                    0,
                                    0,
                                    "Unhandled PRIM in CsrBtHfgFreeMessage");
                break;
            }
    }
    SynergyMessageFree(class, msg);
}

