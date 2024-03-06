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

/* Local: SLC setup completed for HFG */
void csrBtHfgSlcHfgDone(HfgInstance_t *inst)
{
    inst->linkType = CSR_BT_HFG_CONNECTION_HFG;

    HFG_CHANGE_STATE(inst->state,Connected_s);
    HFG_CHANGE_STATE(inst->oldState,ServiceSearch_s);

    /* The HFG is at this late point actually "connected", so call the
     * common connection-completed handler to sync the connection/sds
     * records etc. */
    CsrBtHfgLinkConnectSuccess(inst);

    CsrBtHfgSendHfgServiceConnectInd(inst, CSR_BT_RESULT_CODE_HFG_SUCCESS, CSR_BT_SUPPLIER_HFG);
    CsrBtHfgMainSendHfgHouseCleaning(CsrBtHfgGetMainInstance(inst));
}

static void csrBtBuildCodecMask(HfgInstance_t *inst, CsrUint8 *codecStr)
{
    switch ( (CsrUint8)CsrStrToInt((char*)codecStr) )
    {
        case CSR_BT_WBS_CVSD_CODEC:
            {
                inst->remSupportedCodecs |= CSR_BT_WBS_CVSD_CODEC_MASK;
                break;
            }
        case CSR_BT_WBS_MSBC_CODEC:
            {
                inst->remSupportedCodecs |= CSR_BT_WBS_MSBC_CODEC_MASK;
                break;
            }
        case CSR_BT_WBS_LC3SWB_CODEC:
            {
                inst->remSupportedCodecs |= CSR_BT_WBS_LC3SWB_CODEC_MASK;
                break;
            }
        default:
            break;
    }
}

/* Local: Scan AT string and return next parameter as a separate,
 * newly allocated entity. Updates index to point to next parameter if
 * possible, otherwise set to end so next call will yield NULL */
static CsrUint8 *csrBtHfgGetArgument(CsrUint8 *atStr, CsrUint16 atLen, CsrUint16 *index)
{
    CsrUint16 begin;
    CsrUint16 length;
    CsrBool instr;
    CsrUint8 *res;

    if((atStr == NULL) ||
       (atLen < 1) ||
       (index == NULL))
    {
        return NULL;
    }

    begin = 0xFFFF;
    length = 0;
    instr = FALSE;
    res = NULL;

    /* Scan until end of string reached */
    while(*index < atLen)
    {
        if(begin == 0xFFFF)
        {
            /* Find beginning */
            if((atStr[*index] == ' ') ||
               (atStr[*index] == '\n') ||
               (atStr[*index] == '\r') ||
               (atStr[*index] == '\t') ||
               (atStr[*index] == '(') )
            {
                /* Skip whitespace, skip */
                (*index)++;
            }
            else
            {
                if (atStr[*index] == '\0')
                {/* Null terminator is unconditional stop */
                    break;
                }
                else
                {
                    begin = *index;
                    length++;
                    (*index)++;
                    if(atStr[*index] == '\"')
                    {
                        instr = TRUE;
                    }
                }
            }
            continue;
        }

        if(atStr[*index] == '\0')
        {
            /* Null terminator is unconditional stop */
            break;
        }
        if(instr)
        {
            /* We're in a string */
            if((atStr[*index] == '\"') &&
               (atStr[*index - 1] != '\\'))
            {
                break;
            }
        }
        else
        {
            /* Look for standard terminators */
            if((atStr[*index] == '\n') ||
               (atStr[*index] == '\r') ||
               (atStr[*index] == ',') ||
               (atStr[*index] == ')'))
            {
                break;
            }
        }


        /* One more character in argument */
        length++;
        (*index)++;
    }

    /* If an argument was found, cut it out and prepare for next
     * run */
    if((begin != 0xFFFF) && (length > 0))
    {
        /* Generate argument container */
        res = CsrPmemAlloc(length + 1);
        SynMemCpyS(res,
               length + 1,
               atStr + begin,
               length);
        res[length] = '\0';

        /* Skip to next argument start */
        while(*index < atLen)
        {
            if((atStr[*index] == ' ') ||
               (atStr[*index] == '\n') ||
               (atStr[*index] == '\r') ||
               (atStr[*index] == '\t') ||
               (atStr[*index] == ')') ||
               (atStr[*index] == '(') ||
               (atStr[*index] == ','))
            {
                /* We found delimiter in current index, so increase
                 * count and bail out. Next run will skip any heading
                 * whitespace */
                (*index)++;
                break;
            }
            (*index)++;
        }
    }

    return res;
}

/* Local: Single-char helper function */
static CsrUint8 csrBtHfgGetSingleChar(CsrUint8 *at, CsrUint16 len, CsrUint16 *idx)
{
    CsrUint8 res;
    CsrUint8 *arg = NULL;

    res = 0xFF;
    arg = csrBtHfgGetArgument(at, len, idx);
    if(arg && (CsrStrLen((char*)arg) > 0))
    {
        if ((CsrStrLen((char*)arg) == 3) && (arg[0]=='"' && arg[2]=='"'))
        {
            res = arg[1];
        }
        else
        {
            res = arg[0];
        }
    }

    CsrPmemFree(arg);

    return res;
}

/* Local: Store CSR_BT_CM_DATA_IND on save queue */
static void csrBtHfgSaveCmDataInd(HfgInstance_t *inst)
{
    CsrBtCmDataInd *prim;
    prim = CsrPmemAlloc(sizeof(CsrBtCmDataInd));

    prim->type = CSR_BT_CM_DATA_IND;
    prim->btConnId = inst->hfgConnId;
    prim->payloadLength = inst->atLen;
    prim->payload = CsrPmemAlloc(inst->atLen);
    SynMemCpyS(prim->payload, inst->atLen, inst->atCmd, inst->atLen);
    prim->context = ((CsrBtCmDataInd *)inst->msg)->context;

    CsrMessageQueuePush(&(inst->saveQueue), CSR_BT_CM_PRIM, prim);
}

/* Local: CHLD missing in AT-sequence */
static void csrBtHfgSlcAtMissingTimeout(CsrUint16 mi, void *mv)
{
    HfgInstance_t *inst;
    inst = (HfgInstance_t*)mv;
    CSR_UNUSED(mi);

    if((inst->state == ServiceSearch_s) &&
       (inst->atState == At5Cmer_s || inst->atState == At6ChldQuery_s))
    {
        /* We're still in waiting for the AT+CHLD or AT+BIND but it didn't show
         * up. Assume connection is completed */
        HFG_CHANGE_STATE(inst->atState, At10End_s);
        csrBtHfgSlcHfgDone(inst);
    }
    inst->atSlcTimer = 0;
}

/* AT exec: ATA */
void CsrBtHfgAtAtaExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    CsrBtHfgRingStop(inst);

    /* Always ok */
    CsrBtHfgSendHfgAnswerInd(inst);
    CsrBtHfgSendAtOk(inst);
}

/* AT set: AT+VGM= */
void CsrBtHfgAtVgmSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint8 *arg;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Set indicator, send to app and respond */
    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(arg)
    {
        CsrUint8 gain;
        gain = (CsrUint8)CsrStrToInt((char*)arg);

        if(gain <= CSR_BT_HFG_MAX_VGM)
        {
            inst->ind.other[CSR_BT_HFG_SET_MIC_VOL] = gain;
            CsrBtHfgSendHfgMicGainInd(inst, gain);

            cme = CSR_BT_CME_SUCCESS;
        }
        CsrPmemFree(arg);
    }

    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT set: AT+VGS= */
void CsrBtHfgAtVgsSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint8 *arg;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Set indicator, send to app and respond */
    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(arg)
    {
        CsrUint8 gain;
        gain = (CsrUint8)CsrStrToInt((char*)arg);

        if(gain <= CSR_BT_HFG_MAX_VGS)
        {
            inst->ind.other[CSR_BT_HFG_SET_SPEAKER_VOL] = gain;
            CsrBtHfgSendHfgSpeakerGainInd(inst, gain);

            cme = CSR_BT_CME_SUCCESS;
        }
        CsrPmemFree(arg);
    }

    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT set: AT+CKPD */
void CsrBtHfgAtCkpdSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrBool skipOk;
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    skipOk = FALSE;

    /* Do various things depending on what's running */
    if(inst->ringTimer != 0)
    {
        /* Ringing, so pick it up */
        CsrBtHfgRingStop(inst);
        CsrBtHfgSendHfgAnswerInd(inst);
    }
    else if(inst->scoHandle != CSR_SCHED_QID_INVALID)
    {
        /* Audio is on */
        if(inst->ind.ciev[CSR_BT_CALL_STATUS_INDICATOR] == CSR_BT_CALL_ACTIVE_VALUE)
        {
            /* Hang up if call is active */
            CsrBtHfgSendHfgRejectInd(inst);
        }
        else
        {
            /* No call, transfer SCO back to gateway */
            inst->scoWantedState = FALSE;
            inst->pendingSco = TRUE;
            inst->pendingScoDisconnect = TRUE;
            CsrBtCmScoDisconnectReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);
        }
    }
    else if(inst->ind.ciev[CSR_BT_CALL_STATUS_INDICATOR] == CSR_BT_CALL_ACTIVE_VALUE)
    {
        /* Audio is off, but call is active - transfer audio to HS */
        csrBtHfgSendCmScoConnectReq(inst, CSR_BT_ESCO_DEFAULT_CONNECT);
    }
    else if(CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_VOICE_RECOGNITION)
    {
        /* Voice dialing is supported, so send that */
        CsrBtHfgSendHfgVoiceRecogInd(inst, TRUE);
        skipOk = TRUE;
    }
    else
    {
        /* Not much else to do than the redial */
        CsrBtHfgSendHfgDialInd(inst, CSR_BT_HFG_DIAL_REDIAL, NULL);
        skipOk = TRUE;
    }

    /* Send OK to command, except on dial indication which is handled
     * by the application */
    if(!skipOk)
    {
        CsrBtHfgSendAtOk(inst);
    }
}


/* AT exec: AT+CHUP */
void CsrBtHfgAtChupExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    CsrBtHfgRingStop(inst);

    /* Always ok */
    CsrBtHfgSendHfgRejectInd(inst);
    CsrBtHfgSendAtOk(inst);
}

/* AT test: AT+CHLD=? */
void CsrBtHfgAtChldTest(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message for now,
           process the message from queue after SDC is done */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    /* Always respond with supported CHLDs and OK. The CHLD sender
     * will check the "callConfig" and deliver the needed
     * parameters */
    CsrBtHfgSendAtChldSupport(inst);
    CsrBtHfgSendAtOk(inst);

    /* This is always the last command in the sequence if both device doesn't support
       HF Indicator feature */
    if(seq)
    {
         /* We have received the AT+CHLD, hence we need to cancel the timer */
        if(inst->atSlcTimer != 0)
        {
            CsrSchedTimerCancel(inst->atSlcTimer,NULL,NULL);
            inst->atSlcTimer = 0;
        }

        if((CsrBtHfgGetMainInstance(inst)->locSupFeatures &
                CSR_BT_HFG_SUPPORT_HF_INDICATORS) && (
                inst->remSupFeatures &
                CSR_BT_HF_SUPPORT_HF_INDICATORS))
        {
            HFG_CHANGE_STATE(inst->atState, At6ChldQuery_s);

            /* Started a timer to indicate the completion of SLC to application when
               carkit doesn't send AT+BIND command */
            inst->atSlcTimer = CsrSchedTimerSet(CSR_BT_HFG_MISSING_SLC_AT_TIMER,
                                               csrBtHfgSlcAtMissingTimeout,
                                               0,
                                               (void*)inst);
        }
        else
        {
            /* Wait for two CSR_BT_CM_DATA_CFM messages to make sure that the CHLD string and
            the "OK" have been delivered. Needed to avoid HFG app trying to do some HFP specific
            operation before the HF app even knows that the SLC is fully established */
            inst->waitForDataCfm = 2;
        }
    }
}

/* AT set: AT+CHLD= */
void CsrBtHfgAtChldSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    HfgMainInstance_t *mainInst;
    CsrUint8 *arg;
    CsrUint16 cme;
    CsrUint8 val[2];
    CsrUint8 chld;
    CSR_UNUSED(seq);

    /* Decode. If command is ok inform user about state. He will then
     * have to reply. If command is malformed, send error */
    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;
    mainInst = CsrBtHfgGetMainInstance(inst);

    if(arg)
    {
        val[0] = arg[0];
        val[1] = 0xFF;
        if(CsrStrLen((char*)arg) > 1)
        {
            val[1] = (CsrUint8)(arg[1]);
        }

        /* Map the (x,y) pair into the linear command opcode */
        if((val[0] == '0') &&
           (val[1] == 0xFF) &&
           ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_0)==0))
        {
            /* 0 */
            chld = CSR_BT_RELEASE_ALL_HELD_CALL;
        }
        else if((val[0] == '1') &&
                (val[1] == 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_1)==0))
        {
            /* 1 */
            chld = CSR_BT_RELEASE_ACTIVE_ACCEPT;
        }
        else if((val[0] == '1') &&
                (val[1] != 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_1X)==0))
        {
            /* 1x */
            chld = CSR_BT_RELEASE_SPECIFIED_CALL;
        }
        else if((val[0] == '2') &&
                (val[1] == 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_2)==0))
        {
            /* 2 */
            chld = CSR_BT_HOLD_ACTIVE_ACCEPT;
        }
        else if((val[0] == '2') &&
                (val[1] != 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_2X)==0))
        {
            /* 2x */
            chld = CSR_BT_REQUEST_PRIVATE_WITH_SPECIFIED;
        }
        else if((val[0] == '3') &&
                (val[1] == 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_3)==0))
        {
            /* 3 */
            chld = CSR_BT_ADD_CALL;
        }
        else if((val[0] == '4') &&
                (val[1] == 0xFF) &&
                ((mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_4)==0))
        {
            /* 4 */
            chld = CSR_BT_CONNECT_TWO_CALLS;
        }
        else
        {
            chld = 0xFF;
        }

        /* We must check that we support the specific CHLD command */
        if(chld != 0xFF)
        {
            cme = CSR_BT_CME_SUCCESS;
            CsrBtHfgSendHfgCallHandlingInd(inst,
                                      chld,
                                      (CsrUint8 )(val[1]-'0'));
        }

        CsrPmemFree(arg);
    }

    /* Send error if string is invalid */
    if(cme != CSR_BT_CME_SUCCESS)
    {
        CsrBtHfgSendAtResponse(inst, cme);
    }
}

/* AT exec: ATD### (number) */
void CsrBtHfgAtAtdNumberExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrCharString *arg;
    CsrUint16 numLength = 0;
    CSR_UNUSED(seq);

    /* App must reply, or error on invalid */
    arg = (CsrCharString *)csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    if(arg)
    {
        while(arg[numLength] != '\0')
        {/*removes all ; at the end and truncate the string*/
            if(arg[numLength] == ';')
            {
                arg[numLength] = (CsrUint8)'\0';
            }
            numLength++;
        }
        CsrBtHfgSendHfgDialInd(inst,
                          CSR_BT_HFG_DIAL_NUMBER,
                          arg);
    }
    else
    {
        CsrBtHfgSendAtResponse(inst,
                          CSR_BT_CME_INVALID_CHARACTERS_IN_DIAL_STRING);
    }
}

/* AT exec: ATD> (memory) */
void CsrBtHfgAtAtdMemoryExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrCharString *arg;
    CsrUint16 numLength = 0;
    CSR_UNUSED(seq);

    /* App must reply, or error on invalid */
    arg = (CsrCharString *)csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    if(arg)
    {
        while(arg[numLength] != '\0')
        {/*removes all ; at the end and truncate the string*/
            if(arg[numLength] == ';')
            {
                arg[numLength] = (CsrUint8)'\0';
            }
            numLength++;
        }
        CsrBtHfgSendHfgDialInd(inst,
                          CSR_BT_HFG_DIAL_MEMORY,
                          arg);
    }
    else
    {
        CsrBtHfgSendAtResponse(inst,
                          CSR_BT_CME_INVALID_CHARACTERS_IN_DIAL_STRING);
    }
}

/* AT codec: AT+BAC= */
void CsrBtHfgAtBacSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{

    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message and bail
         * out now */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
        CsrUint8 *arg;
        CsrUint16 i;
        /* Get list of codecs supported by the remote HF-device */
        /* No matter what codecs the HF said that it supported before, the new settings are the proper ones: forget the old remote supported codecs */
        inst->remSupportedCodecs = 0;
        for (i= *index; *index < inst->atLen; i++)
        {
            arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
            if (arg)
            {
                csrBtBuildCodecMask(inst,arg);
            }
            CsrPmemFree(arg);
        }
         /* Send reply */
        if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION) &&
            (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION))
        {
            if(seq)
            {
                HFG_CHANGE_STATE(inst->atState, At2Bac_s);
            }
            CsrBtHfgSendAtOk(inst);
            /* Reset the last used codec information if any */
            inst->lastCodecUsed = CSR_BT_WBS_INVALID_CODEC;
            if (inst->pendingCodecNegotiation)
            {/* This is due to a codec negotiation that failed: try again! */
                CsrBtHfgSendCodecNegMsg(inst);
            }
        }
        else
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
        }
    }
}

#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
/* AT codec: AT+%QAC= */
void CsrBtHfgAtQacSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(inst);
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    //Nothing to be done here
}
#endif

/* AT exec: AT+BCC */
void CsrBtHfgAtBccExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    HfgMainInstance_t *mainInst = CsrBtHfgGetMainInstance(inst);
    CSR_UNUSED(index);
    CSR_UNUSED(seq);

    /* Start codec negotiation */
    if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION) &&
        (mainInst->locSupFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION))
    {
        CsrBtHfgSendAtOk(inst);
        if (((inst->lastCodecUsed == CSR_BT_WBS_INVALID_CODEC) &&
            (inst->selectedQceCodecId == CSR_BT_HFG_QCE_UNSUPPORTED)) ||
            (inst->pendingCodecNegotiation))
        {/* First indicate the codec to use */
            inst->lastCodecUsed = CSR_BT_WBS_INVALID_CODEC;
            CsrBtHfgSendCodecNegMsg(inst);
        }
        else
        {
            /* Open sync. connection */
            CsrBtCmScoCancelReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);

            inst->pendingCodecNegotiation = FALSE;
            if (inst->atResponseTimer != 0)
            {
                CsrSchedTimerCancel(inst->atResponseTimer, NULL, NULL);
                inst->atResponseTimer = 0;
            }
            /* Establish the eSCO connection too */
            csrBtHfgSendCmScoConnectReq(inst, CSR_BT_ESCO_DEFAULT_CONNECT);
        }
    }
    else
    {
        CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
    }
}

/* AT set: AT+BCS */
void CsrBtHfgAtBcsSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(seq);
    if (inst->pendingCodecNegotiation)
    {
        CsrUint8 *arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
        if (arg == NULL)
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_INCORRECT_PARAMETERS);
            inst->lastCodecUsed = CSR_BT_WBS_INVALID_CODEC;
        }
        else
        {
            if ((CsrUint8)CsrStrToInt((char*)arg) == inst->lastCodecUsed)
            {
                CsrBtHfgSendAtOk(inst);
                /*Update codec information to application*/
                CsrBtHfgSendSelectedCodecInd(inst);

                /* Open sync. connection */
                CsrBtCmScoCancelReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);

                inst->pendingCodecNegotiation = FALSE;
                if (inst->atResponseTimer != 0)
                {
                    CsrSchedTimerCancel(inst->atResponseTimer, NULL, NULL);
                    inst->atResponseTimer = 0;
                }
                /* Establish the eSCO connection too */
                csrBtHfgSendCmScoConnectReq(inst, CSR_BT_ESCO_DEFAULT_CONNECT);
            }
            else
            {/* Should not happen! The HF should send At+BCS with the same codec ID as
                the GW issued in the unsolicited response code */
                CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
                inst->lastCodecUsed = CSR_BT_WBS_INVALID_CODEC;
            }
            CsrPmemFree(arg);
        }
    }
    else
    {
        CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
    }
}

/* AT exec: AT+BLDN */
void CsrBtHfgAtBldnExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    /* App must reply */
    CsrBtHfgSendHfgDialInd(inst,
                      CSR_BT_HFG_DIAL_REDIAL,
                      NULL);
}

/* AT set: AT+BRSF= */
void CsrBtHfgAtBrsfSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(seq);
    /* Only sent in AT-sequence, and only by HFG devices */
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message and bail
         * out now */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
        CsrUint8 *arg;

        /* App must reply, or error on invalid */
        arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);

        /* This message means that the headset as an old-school
         * HS, and that the SLC has been established. Setup common
         * things and fall through to normal CKPD handling */
        inst->linkType = CSR_BT_HFG_CONNECTION_HFG;
        HFG_CHANGE_STATE(inst->atState, At1Brsf_s);

        if (arg)
        {
            inst->remSupFeatures = CsrStrToInt((char*)arg);
        }

        CsrBtHfgSendAtBrsf(inst);
        CsrBtHfgSendAtOk(inst);

        CsrPmemFree(arg);
    }
}

/* AT read: AT+BTRH? */
void CsrBtHfgAtBtrhRead(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrBool slcEstablished = FALSE;
    CSR_UNUSED(index);
    CSR_UNUSED(seq);

    if ((!inst->pendingSearch) && (inst->atState != At0Idle_s) &&
        (inst->state != ServiceSearch_s) && (inst->state != Connect_s))
    {
        slcEstablished = TRUE;
    }

    /* App must reply */
    if( (!(CsrBtHfgGetMainInstance(inst)->callConfig & CSR_BT_HFG_CRH_DISABLE_BTRH)) &&  slcEstablished )
    {
        CsrBtHfgSendHfgCallHandlingInd(inst,
                                  CSR_BT_BTRH_READ_STATUS,
                                  0);
    }
    else
    {
        CsrBtHfgSendAtResponse(inst, CSR_BT_CME_NO_NETWORK_SERVICE);
    }
}

/* AT set: AT+BTRH= */
void CsrBtHfgAtBtrhSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint16 cme;
    CsrUint8 val;
    CsrBool slcEstablished = FALSE;
    CSR_UNUSED(seq);

    if ((!inst->pendingSearch) && (inst->atState != At0Idle_s) &&
        (inst->state != ServiceSearch_s) && (inst->state != Connect_s))
    {
        slcEstablished = TRUE;
    }
    /* Decode and notify app of BTRH if command ok and supported,
     * otherwise send error */
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if ( (!(CsrBtHfgGetMainInstance(inst)->callConfig & CSR_BT_HFG_CRH_DISABLE_BTRH)) && slcEstablished )
    {
        val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
        if(val != 0xFF)
        {
            val = val - '0' + CSR_BT_BTRH_PUT_ON_HOLD;

            if((val >= CSR_BT_BTRH_PUT_ON_HOLD) && (val <= CSR_BT_BTRH_REJECT_INCOMING))
            {
                cme = CSR_BT_CME_SUCCESS;
                CsrBtHfgSendHfgCallHandlingInd(inst,
                                          val,
                                          0);
            }
        }
    }

    /* Send error if command malformed */
    if(cme != CSR_BT_CME_SUCCESS)
    {
        CsrBtHfgSendAtResponse(inst, cme);
    }
}

/* AT read: AT+CIND? */
void CsrBtHfgAtCindRead(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message for now,
           process the message from queue after SDC is done */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }

    /* Progress AT sequence forward */
    if(seq)
    {
        /* Is the manual CIND indication update enabled? */
        if(CsrBtHfgGetMainInstance(inst)->hfgConfig & CSR_BT_HFG_CNF_MANUAL_INDICATORS)
        {
            /* Send indication to app and bail out. The AT sequence will
             * continue when application has replied */
            CsrBtHfgSendHfgManualIndicatorInd(inst);
            return;
        }

        HFG_CHANGE_STATE(inst->atState, At4CindStatus_s);
    }

    /* Always reply */
    CsrBtHfgSendAtCindStatus(inst);
    CsrBtHfgSendAtOk(inst);
}

/* AT test: AT+CIND=? */
void CsrBtHfgAtCindTest(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message for now,
           process the message from queue after SDC is done */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    /* Progress the AT-sequence */
    if(seq)
    {
        /* If we havn't received a BRSF, this means that the device is
         * a HF-0.96 device, ie. a new HFG style protocol */
        if(inst->atState == At0Idle_s)
        {
            csrBtHfgSlcHfgDone(inst);
        }
        HFG_CHANGE_STATE(inst->atState, At3CindQuestion_s);
    }

    /* Always send reply */
    CsrBtHfgSendAtCindSupport(inst);
    CsrBtHfgSendAtOk(inst);
}

/* AT set: AT+CMER= */
void CsrBtHfgAtCmerSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    /* We received a full AT+CMER=a,b,c,d request. Extract the
     * "d" value as that's what enable/disable indications */
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message for now,
           process the message from queue after SDC is done */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
        CsrUint8 val = 0xFF;
        CsrUintFast8 cnt;

        cnt = 0;
        while(cnt < 4)
        {
            val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
            if(val != 0xFF)
            {
                cnt++;
            }
            else
            {
                break;
            }
        }

        /* Is value OK, set indicator and respond, otherwise send
         * error */
        if((cnt == 4) && ((val == '0') || (val == '1')))
        {
            inst->ind.other[CSR_BT_HFG_SET_CMER_CIEV] = val - '0';
            CsrBtHfgSendAtOk(inst);
        }
        else
        {
            CsrBtHfgSendAtResponse(inst,
                              CSR_BT_CME_NO_NETWORK_SERVICE);
        }

        /* Special extra meaning in AT sequence */
        if(seq)
        {
            /* If 3-way-calling or HF Indicator feature supported by both, continue AT sequence,
             * otherwise we're done setting up the SLC */
            if(((CsrBtHfgGetMainInstance(inst)->locSupFeatures &
                CSR_BT_HFG_SUPPORT_THREE_WAY_CALLING) && (
                inst->remSupFeatures &
                CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING)) ||
                ((CsrBtHfgGetMainInstance(inst)->locSupFeatures &
                CSR_BT_HFG_SUPPORT_HF_INDICATORS) && (
                inst->remSupFeatures &
                CSR_BT_HF_SUPPORT_HF_INDICATORS)))
            {
                HFG_CHANGE_STATE(inst->atState, At5Cmer_s);

                /* Some car-kits don't follow the spec and "forgets" to
                 * send the AT+CHLD.  Because of this, we must start a
                 * timer to cope with the missing signal */
                if(inst->atSlcTimer == 0)
                {
                    inst->atSlcTimer = CsrSchedTimerSet(CSR_BT_HFG_MISSING_SLC_AT_TIMER,
                                                       csrBtHfgSlcAtMissingTimeout,
                                                       0,
                                                       (void*)inst);
                }
            }
            else
            {
                /* 3-way-calling or HF Indicator not supported in features, so cmer is the
                 * last AT sequence */
                HFG_CHANGE_STATE(inst->atState, At10End_s);
                csrBtHfgSlcHfgDone(inst);
            }
        }
    }
}

/* AT set: AT+CMEE= */
void CsrBtHfgAtCmeeSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    char val;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Set indicator and respond */
    val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_EXTENDED_ERROR_CODES)
    {
        if((val == '0') || (val == '1'))
        {
            cme = CSR_BT_CME_SUCCESS;
            inst->ind.other[CSR_BT_HFG_SET_CMEE] = val - '0';
        }
    }
    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT set: AT+CCWA= */
void CsrBtHfgAtCcwaSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    char val;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* App must reply, or error on invalid */
    val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if((val == '0') || (val == '1'))
    {
        cme = CSR_BT_CME_SUCCESS;
        inst->ind.other[CSR_BT_HFG_SET_CCWA] = val - '0';
    }
    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT set: AT+CLIP= */
void CsrBtHfgAtClipSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    char val;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* App must reply, or error on invalid */
    val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if((val == '0') || (val == '1'))
    {
        cme = CSR_BT_CME_SUCCESS;
        inst->ind.other[CSR_BT_HFG_SET_CLIP] = val - '0';
    }
    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT exec: AT+CLCC */
void CsrBtHfgAtClccExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    /* App must reply */
    CsrBtHfgSendHfgCallListInd(inst);
}

/* AT exec: AT+CNUM */
void CsrBtHfgAtCnumExec(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    /* App must reply */
    CsrBtHfgSendHfgSubscriberNumberInd(inst);
}

/* AT read: AT+COPS? */
void CsrBtHfgAtCopsRead(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    CSR_UNUSED(seq);
    if ((CsrBtHfgGetMainInstance(inst)->hfgConfig & CSR_BT_HFG_CNF_DISABLE_COPS) == 0)
    {/* App must reply */
        CsrBtHfgSendHfgOperatorInd(inst);
    }
    else
    { /* COPS not supported by app */
        CsrBtHfgSendAtCops(inst, 3, NULL); /* always mode 3 (see HFP spec.)*/

        CsrBtHfgSendAtOk(inst);
    }
}

/* AT set: AT+COPS= */
void CsrBtHfgAtCopsSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(seq);
    if (CsrBtHfgGetMainInstance(inst)->hfgConfig & CSR_BT_HFG_CNF_DISABLE_COPS)
    {
        CsrBtHfgSendAtResponse(inst, CSR_BT_CME_NO_NETWORK_SERVICE);
    }
    else
    {
        char val[2];

        val[0] = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
        val[1] = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);

        /* Check that argument really was "3,0" */
        if((val[0] == '3') && (val[1] == '0'))
        {
            CsrBtHfgSendAtOk(inst);
        }
        else
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_NO_NETWORK_SERVICE);
        }
    }
}

/* AT set: AT+BVRA= */
void CsrBtHfgAtBvraSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(seq);
    /* App must reply, or error on invalid */
    if (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_VOICE_RECOGNITION)
    {
        char val;

        val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
        if ((val == '0') || (val == '1'))
        {
            CsrBool enable = (val == '1' ? TRUE : FALSE);

            /* Application must always respond, so don't send any error-code
             * here */
            CsrBtHfgSendHfgVoiceRecogInd(inst, enable);
        }
        else if ((val == '2') &&
                 (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_ENHANCE_VOICE_RECOG_STATUS))
        {
            /* Send CSR_BT_HFG_ENHANCED_VOICE_RECOG_IND for enhanced BVRA value */
            CsrBtHfgSendHfgEnhancedVoiceRecogInd(inst, 0x02);
        }
        else
        {
            /* Malformed data */
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_INVALID_CHARACTERS_IN_TEXT_STRING);
        }
    }
    else
    {
        /* No support of VR feature, so send error */
        CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
    }
}

/* AT set: AT+BIA= */
void CsrBtHfgAtBiaSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint8 cnt;
    CsrUint16 i = *index;
    CsrUint8 val[CSR_BT_CIEV_NUMBER_OF_INDICATORS];
    CsrBool  failFormat = FALSE;
    CsrBool  commaReceived = FALSE;
    CSR_UNUSED(seq);

    SynMemCpyS(val, CSR_BT_CIEV_NUMBER_OF_INDICATORS, inst->ind.bia,CSR_BT_CIEV_NUMBER_OF_INDICATORS);

    cnt = 1;
    for (i = *index; i < inst->atLen; i++)
    {
        char value = inst->atCmd[i];

        if (((i == *index) || (commaReceived)) && (value == ','))
        {/* The first character is a ',' or a ',' has been received after a previous one */
            cnt++;
        }
        else
        {
            if((value != '0') && (value != '1') && (value != ','))
            {
                if ((value != '\r') && (value != '\n'))
                {/* Strange, unexpected characters detected in AT command: discard! */
                    failFormat = TRUE;
                }
                i = inst->atLen;
            }
            else if (value != ',')
            {
                if (((cnt < CSR_BT_CALL_STATUS_INDICATOR) || (cnt > CSR_BT_CALL_HELD_INDICATOR) ) &&
                    (cnt < CSR_BT_CIEV_NUMBER_OF_INDICATORS))
                {/* Only accept values 1 and 0 for the entries in the array and only for the entries that are allowed to be modified */
                    val[cnt] = value - '0';
                }
                cnt++;
            }
        }
        if (value == ',')
        {
            commaReceived = TRUE;
        }
        else
        {
            commaReceived = FALSE;
        }
    }

    if (failFormat)
    {/* fail in command format: rejct and do not update local values */
        CsrBtHfgSendAtResponse(inst,
                          CSR_BT_CME_NO_NETWORK_SERVICE);
    }
    else
    {/* AT command received and understood: update local values */
        SynMemCpyS(inst->ind.bia, CSR_BT_CIEV_NUMBER_OF_INDICATORS, val,CSR_BT_CIEV_NUMBER_OF_INDICATORS);
        CsrBtHfgSendAtOk(inst);
    }
}

/* AT set: AT+NREC= */
void CsrBtHfgAtNrecSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    char val;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Set indicator and respond */
    val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_EC_NR_FUNCTION)
    {
        if((val == '0') || (val == '1'))
        {
            CsrBool enable;
            cme = CSR_BT_CME_SUCCESS;
            enable = (val == '1' ? TRUE : FALSE);
            CsrBtHfgSendHfgNoiseEchoInd(inst, enable);
        }
    }
    CsrBtHfgSendAtResponse(inst, cme);
}


/* AT set: AT+VTS= */
void CsrBtHfgAtVtsSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint8 val;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Set indicator and respond */
    val = csrBtHfgGetSingleChar(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(val != 0xFF)
    {
        CsrBtHfgSendHfgGenerateDtmfInd(inst, val);
        cme = CSR_BT_CME_SUCCESS;
    }

    CsrBtHfgSendAtResponse(inst, cme);
}

/* AT set: AT+BINP= */
void CsrBtHfgAtBinpSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrUint8 *arg;
    CsrUint16 cme;
    CSR_UNUSED(seq);

    /* Fetch request code and send to app, who must reply */
    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    cme = CSR_BT_CME_NO_NETWORK_SERVICE;

    if(arg &&
       (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_ATTACH_NUMBER_TO_VOICE_TAG))
    {
        CsrUint8 req;
        req = (CsrUint8)CsrStrToInt((char*)arg);

        if(req > 0)
        {
            CsrBtHfgSendHfgBtInputInd(inst, req);
            cme = CSR_BT_CME_SUCCESS;
        }
        CsrPmemFree(arg);
    }
    else CsrPmemFree(arg);

    /* If BINP was not supported or message malformed, reply */
    if(cme != CSR_BT_CME_SUCCESS)
    {
        CsrBtHfgSendAtResponse(inst, cme);
    }
}

void CsrBtBuildRemHfIndicatorList(HfgInstance_t *inst, CsrUint8 *hfIndStr)
{
    CsrBtHfgRemoteHFIndicator *rcvdAgHfInd;
    CsrUint32 indId;

    indId = CsrStrToInt((char*)hfIndStr);

    rcvdAgHfInd = HFG_REMOTE_HF_INDICATOR_ADD_LAST(&inst->remoteHfIndicatorList);

    rcvdAgHfInd->hfHfIndicator = (CsrBtHfpHfIndicatorId) indId;
    rcvdAgHfInd->indStatus = CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE;
}

void CsrBtHfgAtBindSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message and bail
         * out now */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
        CsrUint8 *arg;
        CsrUint16 i;

        if(inst->atSlcTimer != 0)
        {
            CsrSchedTimerCancel(inst->atSlcTimer,NULL,NULL);
            inst->atSlcTimer = 0;
        }

        /* Get list of supported HF Indicators by remote HF-device */
        for (i= *index; *index < inst->atLen; i++)
        {
            arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
            if (arg)
            {
                CsrBtBuildRemHfIndicatorList(inst,arg);
                CsrPmemFree(arg);
            }

            if (i == (inst->atLen - 1))
            {
                break;
            }
        }
         /* Send reply */
        if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
            (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
        {
            CsrBtHfgSendAtOk(inst);
            if(seq)
            {
                HFG_CHANGE_STATE(inst->atState, At7BindSupport_s);
            }
        }
        else
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
        }
    }
}

void CsrBtHfgAtBindTest(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message and bail
         * out now */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
         /* Send reply */
        if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
            (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
        {
            CsrBtHfgSendAtBindSupport(inst);
            CsrBtHfgSendAtOk(inst);
            if(seq)
            {
                HFG_CHANGE_STATE(inst->atState, At8BindQuery_s);
            }

        }
        else
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
        }
    }
}

void CsrBtHfgAtBindRead(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CSR_UNUSED(index);
    if(inst->pendingSearch)
    {
        /* If we're performing the SDC search, save message and bail
         * out now */
        csrBtHfgSaveCmDataInd(inst);
        return;
    }
    else
    {
         /* Send reply */
        if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
            (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
        {
            CsrBtHfgSendAllAtBindStatus(inst);
            CsrBtHfgSendAtOk(inst);
            if(seq)
            {
                HFG_CHANGE_STATE(inst->atState, At9BindStatus_s);

                /* Wait for two CSR_BT_CM_DATA_CFM messages to make sure that all BIND string and
                the "OK" have been delivered. Needed to avoid HFG app trying to do some HFP specific
                operation before the HF app even knows that the SLC is fully established */
                inst->waitForDataCfm = CsrBtHfgGetMainInstance(inst)->indCount + 1;
            }
        }
        else
        {
            CsrBtHfgSendAtResponse(inst, CSR_BT_CME_OPERATION_NOT_SUPPORTED);
        }
    }
}

void CsrBtHfgAtBievSet(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq)
{
    CsrBool error = TRUE;
    CsrUint8 *arg;
    CsrUint16 indId = 0;
    CsrUint16 value = 0;
    CsrBtHfgRemoteHFIndicator   *hfIndicator;
    CSR_UNUSED(seq);

    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    if (arg)
    {
        indId = (CsrUint16) CsrStrToInt((char*)arg);
    }
    CsrPmemFree(arg);

    arg = csrBtHfgGetArgument(inst->atCmd, inst->atLen, index);
    if (arg)
    {
        value = (CsrUint16) CsrStrToInt((char*)arg);
    }
    CsrPmemFree(arg);

    hfIndicator = HFG_REMOTE_HF_INDICATOR_GET_FROM_IND_ID(&inst->remoteHfIndicatorList, indId);

    if(hfIndicator != NULL)
    {
        if((hfIndicator->indStatus != CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE) &&
           (value <= hfIndicator->valueMax && value >= hfIndicator->valueMin))
        {
            hfIndicator->indvalue = value;
            CsrBtHfgSendHfIndicatorValueInd(inst, indId, value);

            CsrBtHfgSendAtOk(inst);
            error = FALSE;
        }
    }

    if(error)
    {
        CsrBtHfgSendAtError(inst);
    }
}

