/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
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
#include "csr_formatted_io.h"

/* Length of temporary string buffer */
#define CSR_BT_HFG_TMP_STRING 200

/* Local: String append, no formatting */
static void csrBtHfgStr(CsrUint8 *str, CsrUint16 *idx, CsrUint8 *app)
{
    CsrUint16 srcIdx;
    CsrUint16 srcLen;

    if(app == NULL)
    {
        return;
    }

    srcIdx = 0;
    srcLen = (CsrUint16)CsrStrLen((char*)app);
    while(srcIdx < srcLen)
    {
        str[*idx] = app[srcIdx];
        (*idx)++;
        srcIdx++;
    }
}

/* Local: Number append, no formatting */
static void csrBtHfgInt(CsrUint8 *str, CsrUint16 *idx, CsrUint32 num)
{
    CsrUint8 conv[20];
    CsrSnprintf((CsrCharString *) conv, CSR_ARRAY_SIZE(conv), "%u", num);
    csrBtHfgStr(str, idx, conv);
}

/* Local: CR and LF append */
static void csrBtHfgCrlf(CsrUint8 *str, CsrUint16 *idx)
{
    csrBtHfgStr(str, idx, (CsrUint8*)"\r\n");
}

/* Local: Append string with quotation if not already quoted */
static void csrBtHfgStrQ(CsrUint8 *str, CsrUint16 *idx, CsrCharString *app)
{
    if((app == NULL) || (CsrStrLen((char*)app) < 2))
    {
        return;
    }

    if(app[0] == '"')
    {
        csrBtHfgStr(str, idx, (CsrUint8*)app);
    }
    else
    {
        csrBtHfgStr(str, idx, (CsrUint8*)"\"");
        csrBtHfgStr(str, idx, (CsrUint8*)app);
        csrBtHfgStr(str, idx, (CsrUint8*)"\"");
    }
}

static void csrBtHfgCom(CsrUint8 *str, CsrUint16 *idx, CsrBool add)
{
    if(add)
    {
        csrBtHfgStr(str, idx, (CsrUint8*)",");
    }
}

static void HfgAtResponseTimeout(CsrUint16 mi, void *mv)
{
    HfgInstance_t *inst = (HfgInstance_t*)mv;

    /* Timer fired, reset timer ID */
    inst->atResponseTimer = 0;

    if (inst->pendingScoConfirm && inst->pendingCodecNegotiation)
    {   /* SCO connection failed, response not recieved for +BCS:<CodecID> within time "HFG_AT_RESPONSE_TIMEOUT" */
        CSR_LOG_TEXT_ERROR((CsrBtHfgLto, 0, "HfgAtResponseTimeout: Codec connection failed"));
        CsrBtHfgSendHfgExtendedAudioInd(inst,
                                        CSR_SCHED_QID_INVALID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        RESULT_CODE_HFG_OPERATION_TIMEOUT,
                                        CSR_BT_SUPPLIER_HFG);
        inst->pendingSco              = FALSE;
        inst->pendingScoConfirm       = FALSE;
        inst->pendingCodecNegotiation = FALSE;
        inst->lastCodecUsed           = CSR_BT_WBS_INVALID_CODEC;
        inst->scoPcmSlot              = 0;
        inst->scoPcmRealloc           = 0;
    }
    CSR_UNUSED(mi);
}

/* From this point and down follows the actual AT result generator and
 * sender functions */

/* Send OK */
void CsrBtHfgSendAtOk(HfgInstance_t *inst)
{

    CsrUint8 *str;
    CsrUint16 idx = 7;

    str = CsrPmemAlloc(idx);

    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);
    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_OK);

    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CMEE error */
void CsrBtHfgSendAtError(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_ERROR);
    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send response/error-code based on CMEE and code */
void CsrBtHfgSendAtResponse(HfgInstance_t *inst, CsrUint16 cme)
{
    if(cme == CSR_BT_CME_SUCCESS)
    {
        CsrBtHfgSendAtOk(inst);
    }
    else if(inst->ind.other[CSR_BT_HFG_SET_CMEE] != 0)
    {
        CsrBtHfgSendAtCmee(inst, cme);
    }
    else
    {
        CsrBtHfgSendAtError(inst);
    }
}

/* Send CMEE error */
void CsrBtHfgSendAtCmee(HfgInstance_t *inst, CsrUint16 cmee)
{
    CsrUint8 *str;
    CsrUint16 idx = 50;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CMEE_ERROR);
    csrBtHfgInt(str, &idx, cmee);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send RING */
void CsrBtHfgSendAtRing(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = 10;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_RING);
    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CIND supported attributes */
void CsrBtHfgSendAtCindSupport(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIND);
    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIND_SUPPORT_1);
    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIND_SUPPORT_2);
    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CIND status update */
void CsrBtHfgSendAtCindStatus(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIND);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_SERVICE_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_CALL_STATUS_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_CALL_SETUP_STATUS_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_CALL_HELD_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_SIGNAL_STRENGTH_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_ROAM_INDICATOR]);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, inst->ind.ciev[CSR_BT_BATTERY_CHARGE_INDICATOR]);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CHLD supported attributes */
void CsrBtHfgSendAtChldSupport(HfgInstance_t *inst)
{
    HfgMainInstance_t *mainInst;
    CsrBool comma;
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    mainInst = CsrBtHfgGetMainInstance(inst);
    comma = FALSE;

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CHLD);
    csrBtHfgStr(str, &idx, (CsrUint8*)"(");

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_0))
    {
        csrBtHfgStr(str, &idx, (CsrUint8*)"0");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_1))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"1");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_1X))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"1x");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_2))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"2");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_2X))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"2x");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_3))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"3");
        comma = TRUE;
    }

    if(!(mainInst->callConfig & CSR_BT_HFG_CRH_DISABLE_CHLD_4))
    {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgStr(str, &idx, (CsrUint8*)"4");
    }

    csrBtHfgStr(str, &idx, (CsrUint8*)")");
    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send BRSF and bitmap */
void CsrBtHfgSendAtBrsf(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BRSF);
    csrBtHfgInt(str, &idx, CsrBtHfgGetMainInstance(inst)->locSupFeatures);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CIEV update */
void CsrBtHfgSendAtCiev(HfgInstance_t *inst, CsrUint8 ind, CsrUint8 value)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIEV);
    csrBtHfgInt(str, &idx, ind);
    csrBtHfgCom(str, &idx, TRUE);
    csrBtHfgInt(str, &idx, value);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send CIEV update: several indicators at a time */
void CsrBtHfgSendCombinedAtCiev(HfgInstance_t *inst, CsrUint8 indicatorMask)
{
    if (indicatorMask > 0)
    {
        CsrUint8 i;
        CsrBool addStartMark = FALSE;
        CsrUint8 *str;
        CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

        str = CsrPmemAlloc(idx);
        CsrMemSet(str, 0, idx);
        idx = 0;
        csrBtHfgCrlf(str, &idx);

        for (i=0;i<(CSR_BT_CIEV_NUMBER_OF_INDICATORS-1);i++)
        {
            if (indicatorMask & (1<<i))
            {
                if (addStartMark)
                {/* <CR><LF> needed to separate the AT commands */
                    csrBtHfgCrlf(str, &idx);
                }
                csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CIEV);
                csrBtHfgInt(str, &idx, i+1);
                csrBtHfgCom(str, &idx, TRUE);
                csrBtHfgInt(str, &idx, inst->ind.ciev[i+1]);
                csrBtHfgCrlf(str, &idx);
                addStartMark = TRUE;
            }
        }
        CsrBtHfgSendCmDataReq(inst, idx, str);
    }
}

void CsrBtHfgSendCodecNegMsg(HfgInstance_t *inst)
{
    HfgMainInstance_t *mainInst;
    CsrUint8 *str;
    CsrUint16 idx = 20;
    CsrUint8 codec = CSR_BT_WBS_CVSD_CODEC;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    mainInst = CsrBtHfgGetMainInstance(inst);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BCS);
    
    if ((mainInst->supportedCodecs & CSR_BT_WBS_LC3SWB_CODEC_MASK) &&
        (inst->remSupportedCodecs & CSR_BT_WBS_LC3SWB_CODEC_MASK) &&
        (inst->lastCodecUsed != CSR_BT_WBS_LC3SWB_CODEC))
    {
        codec = CSR_BT_WBS_LC3SWB_CODEC;
    }
    else if ((mainInst->supportedCodecs & CSR_BT_WBS_MSBC_CODEC_MASK) &&
        (inst->remSupportedCodecs & CSR_BT_WBS_MSBC_CODEC_MASK) &&
        (inst->lastCodecUsed != CSR_BT_WBS_MSBC_CODEC))
    {
        codec = CSR_BT_WBS_MSBC_CODEC; 
    }
    else
    {
        codec = CSR_BT_WBS_CVSD_CODEC;
    }
    /* Keep track of the last trial */
    inst->lastCodecUsed = codec;
    inst->pendingCodecNegotiation = TRUE;
    csrBtHfgInt(str, &idx, codec);
    
    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);

    /* HF must respond with AT+BCS=<codec ID>, Start a timer.
       Fail codec connection setup, if reponse is not received before timeout */
    if (inst->atResponseTimer != 0)
    { /* Cancel pending AT reponse timer before starting new timer */
        CsrSchedTimerCancel(inst->atResponseTimer, NULL, NULL);
        inst->atResponseTimer = 0;
    }
    inst->atResponseTimer =  CsrSchedTimerSet((CsrTime)HFG_AT_RESPONSE_TIMEOUT, HfgAtResponseTimeout, 0,(void*)inst);
}

/* Send speaker gain */
void CsrBtHfgSendAtSpeakerGain(HfgInstance_t *inst, CsrUint8 gain)
{
    CsrUint8 *opcode;
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    opcode = (inst->linkType == CSR_BT_HFG_CONNECTION_HFG
              ? CSR_BT_HFG_STR_HF_SPEAKER_GAIN
              : CSR_BT_HFG_STR_HS_SPEAKER_GAIN);

    csrBtHfgStr(str, &idx, opcode);
    csrBtHfgInt(str, &idx, gain);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send microphone gain */
void CsrBtHfgSendAtMicGain(HfgInstance_t *inst, CsrUint8 gain)
{
    CsrUint8 *opcode;
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    opcode = (inst->linkType == CSR_BT_HFG_CONNECTION_HFG
              ? CSR_BT_HFG_STR_HF_MIC_GAIN
              : CSR_BT_HFG_STR_HS_MIC_GAIN);

    csrBtHfgStr(str, &idx, opcode);
    csrBtHfgInt(str, &idx, gain);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send call waiting notification without name */
void CsrBtHfgSendAtCcwa(HfgInstance_t *inst,
                   CsrCharString *number,
                   CsrUint8 numType)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CCWA);
    csrBtHfgStrQ(str, &idx, number);
    csrBtHfgCom(str, &idx, TRUE);
    csrBtHfgInt(str, &idx, numType);
    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send response-and-hold status */
void CsrBtHfgSendAtBtrh(HfgInstance_t *inst, CsrUint8 btrh)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BTRH);
    csrBtHfgInt(str, &idx, btrh);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send call list entry (clcc) */
void CsrBtHfgSendAtClcc(HfgInstance_t *inst,
                   CsrUint8 cidx,
                   CsrUint8 dir,
                   CsrUint8 state,
                   CsrUint8 mode,
                   CsrUint8 mpy,
                   CsrCharString *number,
                   CsrUint8 numType)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CLCC);

    csrBtHfgInt(str, &idx, cidx);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, dir);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, state);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, mode);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, mpy);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgStrQ(str, &idx, number);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, numType);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send subscriber number entry (cnum) */
void CsrBtHfgSendAtCnum(HfgInstance_t *inst,
                   CsrCharString      *number,
                   CsrUint8        numType,
                   CsrUint8        service)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CNUM);

    /* alpha is blank */
    csrBtHfgCom(str, &idx, TRUE);

    /* number */
    csrBtHfgStrQ(str, &idx, number);
    csrBtHfgCom (str, &idx, TRUE);

    /* numbertype */
    csrBtHfgInt(str, &idx, numType);
    csrBtHfgCom(str, &idx, TRUE);

    /* speed (is blank) */
    csrBtHfgCom(str, &idx, TRUE);

    /* service */
    csrBtHfgInt(str, &idx, service);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send inband ringing status update (bsir) */
void CsrBtHfgSendAtBsir(HfgInstance_t *inst, CsrBool inband)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BSIR);
    csrBtHfgInt(str, &idx, inband);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send Bluetooth input response (binp) */
void CsrBtHfgSendAtBinp(HfgInstance_t *inst, CsrCharString *response)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;
    if (response != NULL)
    {
        idx = (CsrUint16)(idx + CsrStrLen((char *)response) + 1);
    }

    str = CsrPmemAlloc(idx);

    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BINP);
    csrBtHfgStrQ(str, &idx, response);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send clip without name */
void CsrBtHfgSendAtClip(HfgInstance_t *inst,
                   CsrCharString *number,
                   CsrUint8 numType)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_CLIP);

    csrBtHfgStrQ(str, &idx, number);
    csrBtHfgCom(str, &idx, TRUE);

    csrBtHfgInt(str, &idx, numType);

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send voice recognition activation */
void CsrBtHfgSendAtBvra(HfgInstance_t *inst,
                               CsrUint8 bvra,
                               CsrBtHfpVreState vreState,
                               CsrCharString *textId,
                               CsrBtHfpVrTxtType textType,
                               CsrBtHfpVrTxtOp textOperation,
                               CsrCharString *string)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;
    CsrBool textData = FALSE;

    if ((CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_VOICE_RECOG_TEXT) &&
        (inst->remSupFeatures & CSR_BT_HF_SUPPORT_VOICE_RECOG_TEXT) &&
        (textId != NULL) && (string != NULL))
    {
        textData = TRUE;
        idx += (CsrUint16) (CsrStrLen(string));
    }

    str = CsrPmemZalloc(idx);

    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BVRA);

    csrBtHfgInt(str, &idx, bvra);

    if ((CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_ENHANCE_VOICE_RECOG_STATUS) &&
        (inst->remSupFeatures & CSR_BT_HF_SUPPORT_ENHANCE_VOICE_RECOG_STATUS))
    {
        csrBtHfgCom(str, &idx, TRUE);
        csrBtHfgInt(str, &idx, vreState);
    }

    if (textData)
    {
        csrBtHfgCom(str, &idx, TRUE);
        csrBtHfgStr(str, &idx, (CsrUint8 *)textId);

        csrBtHfgCom(str, &idx, TRUE);
        csrBtHfgInt(str, &idx, textType);

        csrBtHfgCom(str, &idx, TRUE);
        csrBtHfgInt(str, &idx, textOperation);

        csrBtHfgCom(str, &idx, TRUE);
        csrBtHfgStrQ(str, &idx, string);
    }

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send network operator without string */
void CsrBtHfgSendAtCops(HfgInstance_t *inst,
                   CsrUint8 mode,
                   CsrCharString *name)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;

    str = CsrPmemAlloc(idx);
    CsrMemSet(str, 0, idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_COPS);

    csrBtHfgInt(str, &idx, mode);

    csrBtHfgCom(str, &idx, TRUE);

    if(name != NULL)
    {

        csrBtHfgStr(str, &idx, (CsrUint8*)"0,");

        csrBtHfgStrQ(str, &idx, name);
    }
    else
    {
        csrBtHfgCom(str, &idx, TRUE);
    }

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

void CsrBtHfgSendAtBindSupport(HfgInstance_t *inst)
{
    CsrUint8 *str;
    CsrUint16 idx = CSR_BT_HFG_TMP_STRING;
    CsrUint16 hfIndCount;
    CsrBool   comma = FALSE;
    CsrBtHfgHfIndicator *hfgHfInd;
    hfIndCount = CsrBtHfgGetMainInstance(inst)->indCount;
    hfgHfInd = CsrBtHfgGetMainInstance(inst)->localHfIndicatorList;

    str = (CsrUint8 *)CsrPmemZalloc(idx);
    idx = 0;
    csrBtHfgCrlf(str, &idx);

    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BIND);
    csrBtHfgStr(str, &idx, (CsrUint8*)"(");

   while(hfIndCount > 0)
   {
        csrBtHfgCom(str, &idx, comma);
        csrBtHfgInt(str, &idx,
            hfgHfInd[CsrBtHfgGetMainInstance(inst)->indCount - hfIndCount].hfIndicatorID);
        comma =TRUE;
        hfIndCount--;
   }

    csrBtHfgStr(str, &idx, (CsrUint8*)")");
    csrBtHfgCrlf(str, &idx);

    CsrBtHfgSendCmDataReq(inst, idx, str);
}

/* Send enabled/disabled state of all AG supported HF Indicators */
void CsrBtHfgSendAllAtBindStatus(HfgInstance_t *inst)
{

    CsrUint16 hfIndCount;
    CsrBtHfgHfIndicator *hfgHfInd;
    hfIndCount = CsrBtHfgGetMainInstance(inst)->indCount;
    hfgHfInd = CsrBtHfgGetMainInstance(inst)->localHfIndicatorList;

    while(hfIndCount > 0)
    {
        CsrBtHfpHfIndicatorId       hfIndicatorID;
        CsrBtHfpHfIndicatorStatus   status;
        CsrBtHfgRemoteHFIndicator   *remHfInd;

        hfIndicatorID = hfgHfInd[CsrBtHfgGetMainInstance(inst)->indCount - hfIndCount].hfIndicatorID;
        status = hfgHfInd[CsrBtHfgGetMainInstance(inst)->indCount - hfIndCount].status;

        remHfInd = HFG_REMOTE_HF_INDICATOR_GET_FROM_IND_ID(&inst->remoteHfIndicatorList, hfIndicatorID);
        if(remHfInd != NULL)
        {
            remHfInd->indStatus = status;
            remHfInd->valueMax = hfgHfInd[CsrBtHfgGetMainInstance(inst)->indCount - hfIndCount].valueMax;
            remHfInd->valueMin = hfgHfInd[CsrBtHfgGetMainInstance(inst)->indCount - hfIndCount].valueMin;
        }

        CsrBtHfgSendAtBindStatus(inst, hfIndicatorID, status);
        hfIndCount--;
    }
}

/* Send enable/disable state of HF Indicators */
void CsrBtHfgSendAtBindStatus(HfgInstance_t *inst, CsrBtHfpHfIndicatorId hfIndId,
                               CsrBtHfpHfIndicatorStatus status)
{
    CsrUint8 *str;
    CsrUint16 idx = 20;

    str = (CsrUint8 *)CsrPmemZalloc(idx);
    idx = 0;

    csrBtHfgCrlf(str, &idx);
    csrBtHfgStr(str, &idx, CSR_BT_HFG_STR_BIND);

    csrBtHfgInt(str, &idx, hfIndId);
    csrBtHfgCom(str, &idx, TRUE);
    if(status == CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE)
    {
        csrBtHfgInt(str, &idx, 0);
    }
    else
    {
        csrBtHfgInt(str, &idx, 1);
    }

    csrBtHfgCrlf(str, &idx);
    CsrBtHfgSendCmDataReq(inst, idx, str);
}

