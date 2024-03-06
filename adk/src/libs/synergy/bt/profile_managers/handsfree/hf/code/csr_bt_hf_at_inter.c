/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_sched.h"
#include "csr_pmem.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_bt_util.h"
#include "csr_bt_hf_main.h"
#include "csr_bt_hf_util.h"
#include "csr_bt_hf_at_inter.h"
#include "csr_bt_hf_main_sef.h"
#include "csr_bt_hf_lib.h"
#include "csr_bt_hfhs_data_sef.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_hf_streams.h"
#endif

#define TO_UPPER(x)        ((((x) >= 'a') && ((x) <= 'z')) ? ((x) & 0xDF) : (x))
#define IS_DIGIT_CHAR(x)   ((((x) >= '0') && ((x) <= '9')) ? TRUE : FALSE)

/* AT command table structure */
typedef struct
{
    CsrUint8        token;       /* Numeric token */
    char          *at;          /* AT command string */
    HfAtHandler_t  func;        /* AT handler function */
} HfAtCheck_t;

/**********************************************************************
*  Forward declarations: function prototypes
***********************************************************************/
void CsrBtHfSendHfCurrentOperatorSelectionCfm(HfMainInstanceData_t *instData, CsrUint8 *atTextString);

CsrBool CsrBtHfVgmSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfVgsSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfCindHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfStoreChldInfo(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfFindCievValue(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfOkHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfErrorHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBrsfHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBtrhHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfSendHfClccInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfSendHfCnumInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfRingHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBsirHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfCopsHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfCmeErrorHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfSendClipInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfSendCcwaInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfSendBtInputCfm(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBvraHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBusyHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfNoCarrierHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfNoAnswerHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfDelayedHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfBlacklistedHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfCodecNegSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfHfIndResHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
CsrBool CsrBtHfQcsHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
CsrBool CsrBtHfQacHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString);
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

CsrBool SendConfirmMessage(HfMainInstanceData_t  *instData, CsrUint16 resultToSend);
void cancelAtResponseTimer(HfMainInstanceData_t *instData);
CsrUint32 StrATLen(const char *string);
void CsrBtHfSendHfAtCmdInd(HfMainInstanceData_t *instData);
CsrBool IsSlcEstablished(HfMainInstanceData_t *instData);
CsrBool CsrBtHfCodecAcceptable(HfMainInstanceData_t *instData);

static const HfAtCheck_t hfAtCheck[] =
{
    { CSR_BT_VGM_TOKEN,             "+VGM",        CsrBtHfVgmSet          },
    { CSR_BT_VGS_TOKEN,             "+VGS",        CsrBtHfVgsSet          },
    { CSR_BT_CIND_STATUS_TOKEN,     "+CIND:",      CsrBtHfCindHandler     },
    { CSR_BT_CHLD_TOKEN,            "+CHLD:",      CsrBtHfStoreChldInfo   },
    { CSR_BT_CIEV_TOKEN,            "+CIEV:",      CsrBtHfFindCievValue   },
    { CSR_BT_OK_TOKEN,              "OK",          CsrBtHfOkHandler       },
    { CSR_BT_ERROR_TOKEN,           "ERROR",       CsrBtHfErrorHandler    },
    { CSR_BT_BRSF_TOKEN,            "+BRSF:",      CsrBtHfBrsfHandler     },
    { CSR_BT_BTRH_STATUS_TOKEN,     "+BTRH",       CsrBtHfBtrhHandler     },
    { CSR_BT_CMEE_STATUS_TOKEN,     "+CMEERROR",   CsrBtHfCmeErrorHandler },
    { CSR_BT_CCWA_TOKEN,            "+CCWA:",      CsrBtHfSendCcwaInd     },
    { CSR_BT_CLIP_TOKEN,            "+CLIP:",      CsrBtHfSendClipInd     },
    { CSR_BT_CLCC_STATUS_TOKEN,     "+CLCC:",      CsrBtHfSendHfClccInd   },
    { CSR_BT_CNUM_STATUS_TOKEN,     "+CNUM:",      CsrBtHfSendHfCnumInd   },
    { CSR_BT_RING_TOKEN,            "RING",        CsrBtHfRingHandler     },
    { CSR_BT_BSIR_TOKEN,            "+BSIR:",      CsrBtHfBsirHandler     },
    { CSR_BT_COPS_TOKEN,            "+COPS:",      CsrBtHfCopsHandler     },
    { CSR_BT_BVRA_TOKEN,            "+BVRA:",      CsrBtHfBvraHandler     },
    { CSR_BT_BUSY_TOKEN,            "BUSY",        CsrBtHfBusyHandler     },
    { CSR_BT_NO_CARRIER_TOKEN,      "NO CARRIER",  CsrBtHfNoCarrierHandler},
    { CSR_BT_NO_ANSWER_TOKEN,       "NO ANSWER",   CsrBtHfNoAnswerHandler },
    { CSR_BT_DELAYED_TOKEN,         "DELAYED",     CsrBtHfDelayedHandler  },
    { CSR_BT_BLACKLISTED_TOKEN,     "BLACKLISTED", CsrBtHfBlacklistedHandler },
    { CSR_BT_BINP_TOKEN,            "+BINP:",      CsrBtHfSendBtInputCfm  },
    { CSR_BT_BCS_TOKEN,             "+BCS:",       CsrBtHfCodecNegSet     },
    { CSR_BT_BIND_TOKEN,            "+BIND:",      CsrBtHfHfIndResHandler },
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    { CSR_BT_QAC_TOKEN,             "+%QAC:",     CsrBtHfQacHandler       },
    { CSR_BT_QCS_TOKEN,             "+%QCS:",     CsrBtHfQcsHandler       },
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

    { CSR_BT_OTHER_TOKEN,           NULL,          NULL                   }
};

/*************************************************************************************
 SendConfirmMessage : Find out what message to give the application and do it
************************************************************************************/
CsrBool SendConfirmMessage(HfMainInstanceData_t  *instData, CsrUint16 resultToSend)
{
    CsrBool retValue = TRUE;
    CsrBtHfPrim type = 0xFFFF;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    switch(linkPtr->lastAtCmdSent)
    {
        case vgm:
        {
            type = CSR_BT_HF_MIC_GAIN_STATUS_CFM;
            break;
        }
        case vgs:
        {
            type = CSR_BT_HF_SPEAKER_GAIN_STATUS_CFM;
            break;
        }
        case bcc:
        {
            if (resultToSend == CSR_BT_CME_SUCCESS)
            {
                /* Allow CM to accept incoming SCO connection */
                CsrBtHfAcceptIncomingSco(linkPtr);
            }
            else
            {
                /* App requested for audio connection, inform the failure */
                if (linkPtr->lastAudioReq == CSR_BT_HF_AUDIO_ON)
                {
                    linkPtr->audioPending = FALSE;
                    CsrBtHfSendHfAudioConnectInd(instData, linkPtr->pcmSlot, 0, 0, 0, 0, 0, 0,
                                                 RESULT_CODE_HF_AUDIO_CONNECT_ATTEMPT_FAILED,
                                                 0xDEAD, CSR_BT_SUPPLIER_HF, CSR_BT_HF_AUDIO_CONNECT_CFM);
                }
            }
            break;
        }
        case bcs:
        {
            linkPtr->lastAtCmdSent = idleCmd;
            CsrBtHfAcceptIncomingSco(linkPtr);
        }
        /* else, ignore and wait for HFG move. */
        break;

        case bac:
        {
            linkPtr->lastAtCmdSent = idleCmd;
            if ((linkPtr->atSequenceState == codecSupport) && (linkPtr->state == ServiceSearch_s))
            {
                linkPtr->atSequenceState = cindSupport;
                sendCindSupport(instData);
            }
            return retValue;
        }
        case brsf:
        {
            if ((linkPtr->atSequenceState == supportFeatures) && (linkPtr->state == ServiceSearch_s))
            {
                linkPtr->serviceState = serviceConnect_s;
                if (linkPtr->remoteVersion == 0)
                {/* remote version yet unknown: BRSF not supported means version 0.96....*/
                    linkPtr->remoteVersion = CSR_BT_FIRST_HFP_NO_ESCO;
                }
                linkPtr->lastAtCmdSent = idleCmd;
                if (/* (linkPtr->remoteVersion == CSR_BT_FIRST_HFP_CODEC_NEG_ESCO) &&*/
                    (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) &&
                    (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION))
                {/* Codec negotiation supported locally and by the remote */
                    linkPtr->atSequenceState = codecSupport;
                    sendCodecSupport(instData);
                }
                else
                {
                    linkPtr->atSequenceState = cindSupport;
                    sendCindSupport(instData);
                }
            }
            return retValue;
        }
        case cindStatusCmd:
        {
            linkPtr->lastAtCmdSent = idleCmd;
            CsrBtHfSendIndicatorsUpdateCfm(instData,resultToSend);
            return retValue;
        }
        case cnum:
        {
            type = CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_CFM;
            break;
        }
        case cmer:
        {
            type = CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_CFM;
            break;
        }
        case btrh:
        case chld:
        {
            type = CSR_BT_HF_CALL_HANDLING_CFM;
            break;
        }
        case ccwa:
        {
            type = CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_CFM;
            if (linkPtr->pendingSlcXtraCmd)
            {
                linkPtr->lastAtCmdSent = idleCmd;
                if (((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION) != 0) ||
                    ((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_EXTENDED_ERROR_CODES) == 0))
                {/* Finished sending automatic AT commands; send low power mode request
                     if not already in LP mode, and LP request enabled */
                    linkPtr->pendingSlcXtraCmd = FALSE;
                }
                else
                {
                    CsrBtHfSendHfHouseCleaning(instData);
                }
                return retValue;
            }
            break;
        }
        case clip:
        {
            type = CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_CFM;
            if (linkPtr->pendingSlcXtraCmd)
            {
                linkPtr->lastAtCmdSent = idleCmd;
                if  (((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CCWA_ACTIVATION) == 0) &&
                        (((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_THREE_WAY_CALLING) ||
                         (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_CONTROL)) &&
                        ((instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING) ||
                         (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_ENHANCED_CALL_CONTROL))) )
                {
                    CsrBtHfSendHfHouseCleaning(instData);
                }
                else if (((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION) == 0) &&
                    (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_EXTENDED_ERROR_CODES))
                {
                    CsrBtHfSendHfHouseCleaning(instData);
                }
                else
                {/* Finished sending automatic AT commands; send low power mode request
                     if not already in LP mode, and LP request enabled */
                    linkPtr->pendingSlcXtraCmd = FALSE;
                }
                return retValue;
            }
            break;
        }
        case clcc:
        {
            type = CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM;
            break;
        }
        case cmee:
        {
            type = CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_CFM;
            if (linkPtr->pendingSlcXtraCmd)
            {   /* Finished sending automatic AT commands; send low power mode request
                   if not already in LP mode, and LP request enabled */
                linkPtr->pendingSlcXtraCmd = FALSE;
                linkPtr->lastAtCmdSent = idleCmd;
                return retValue;
            }
            break;
        }
        case bvra:
        {
            type = CSR_BT_HF_SET_VOICE_RECOGNITION_CFM;
            break;
        }
        case binp:
        {
            CsrBtHfSendBtInputCfm(instData,NULL);
            /*No break intended!*/
        }
        /*Fall-through*/
        case copsSet:
        { /* Confirmation already sent */
            linkPtr->lastAtCmdSent = idleCmd;
            return retValue;
        }
        case copsQueryCmd:
        {
            CsrBtHfSendHfCurrentOperatorSelectionCfm(instData, NULL);
            linkPtr->lastAtCmdSent = idleCmd;
            return retValue;
        }
        case answer:
        {
            type = CSR_BT_HF_CALL_ANSWER_CFM;
            break;
        }
        case callEnd:
        {
            type = CSR_BT_HF_CALL_END_CFM;
            break;
        }
        case ckpd:
        {
            linkPtr->lastAtCmdSent = idleCmd;
            return retValue;
        }
        case dialNumber:
        case dialMem:
        case redial:
        {
            type = CSR_BT_HF_DIAL_CFM;
            break;
        }
        case nrec:
        {
            type = CSR_BT_HF_SET_ECHO_AND_NOISE_CFM;
            break;
        }
        case vts:
        {
            type = CSR_BT_HF_GENERATE_DTMF_CFM;
            break;
        }
        case bia:
        {
            type = CSR_BT_HF_INDICATOR_ACTIVATION_CFM;
            break;
        }
        case bievSet:
        {
            type = CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM;
            break;
        }
        case other:
        {   type = CSR_BT_HF_AT_CMD_CFM;
            break;
        }
        default: /* Just send the message back to the app as an AT cmd ind */
        {
            retValue = FALSE;
            break;
        }
    }
    linkPtr->lastAtCmdSent = idleCmd;

    if ((type != 0xFFFF) && (retValue))
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,resultToSend,type);
    }

    return retValue;
}

/************************************************************************************
*      Data response timer handling: timeout handler and cancellation
*************************************************************************************/
void CsrBtHfAtResponseTimeout(CsrUint16 mi, void *mv)
{
    HfInstanceData_t *linkPtr;
    HfMainInstanceData_t *inst;
    HfHsData_t  *data;

    inst = (HfMainInstanceData_t*) mv;
    inst->index = (CsrUint8)mi;
    linkPtr = (HfInstanceData_t *)&(inst->linkData[inst->index]);
    data = linkPtr->data;
    CsrPmemFree(data->recvAtCmd);
    data->recvAtCmd = NULL;

    /* Timer was fired */
    data->atResponseTimerId = 0;
    /* Now send error response to the application */
    SendConfirmMessage(inst,CSR_BT_CME_AG_FAILURE);

    /* And check whether more messages are waiting to be delivered */
    if ((linkPtr->lastAtCmdSent == idleCmd) || (linkPtr->atSequenceState < copsQuery))
    {/* Now check if there are new messages to send in the queue */
        CsrBool startAtResponseTimer = FALSE;
        if (HfProcessSavedAtMessages(inst, data, &startAtResponseTimer))
        {
            if (startAtResponseTimer)
            {
                data->atResponseTimerId = CsrSchedTimerSet((CsrTime)(inst->atRespWaitTime*1000000),
                                                           CsrBtHfAtResponseTimeout,
                                                           (CsrUint16)inst->index,(void*)inst);
            }
        }
        else
        {
            data->allowed2SendCmData = TRUE;
        }
    }

}

void cancelAtResponseTimer(HfMainInstanceData_t *instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (linkPtr->data->atResponseTimerId != 0)
    {
        void *mv;
        CsrUint16 mi;
        CsrSchedTimerCancel(linkPtr->data->atResponseTimerId, &mi, &mv);
        linkPtr->data->atResponseTimerId = 0;
    }
}

/*************************************************************************************
 StrATLen : Find Length of AT string without \r\n
************************************************************************************/
CsrUint32 StrATLen(const char *string)
{
    CsrUint32    length = 0;

    if (string != NULL)
    {
        while (*string++ != '\r')
        {
            length++;
        }
    }

    return length;
}

/******************************************************************************************
*           Accept incoming SCO after SLC has been completely established
*******************************************************************************************/
void CsrBtHfAcceptIncomingSco(HfInstanceData_t *linkPtr)
{
    if ((!linkPtr->scoConnectAcceptPending) && (linkPtr->scoHandle == HF_SCO_UNUSED))
    {
        CsrBtCmScoCommonParms *scoParms = (CsrBtCmScoCommonParms *)CsrPmemAlloc(sizeof(CsrBtCmScoCommonParms));

        CsrBtHfSetIncomingScoAudioParams(linkPtr, scoParms);

        linkPtr->scoConnectAcceptPending = TRUE;
        CsrBtCmScoAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE,
                                       linkPtr->hfConnId,
                                       scoParms->audioQuality,
                                       scoParms->txBandwidth,
                                       scoParms->rxBandwidth,
                                       scoParms->maxLatency,
                                       scoParms->voiceSettings,
                                       scoParms->reTxEffort);
        CsrPmemFree(scoParms);
    }
}

/*************************************************************************************
*   Send CSR_BT_HF_AT_CMD_IND
**************************************************************************************/
void CsrBtHfSendHfAtCmdInd(HfMainInstanceData_t *instData)
{
    CsrBtHfAtCmdInd *prim;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    prim                 = (CsrBtHfAtCmdInd *)CsrPmemAlloc(sizeof(CsrBtHfAtCmdInd));
    prim->type           = CSR_BT_HF_AT_CMD_IND;
    prim->atCmdString    = (CsrCharString *)linkPtr->data->recvAtCmd;
    prim->connectionId   = linkPtr->hfConnId;
    CsrBtHfMessagePut(instData->appHandle, prim);
}
/*************************************************************************************
 whiteSpace : Remove whitespaces from the received message
************************************************************************************/
static CsrBool whiteSpace(CsrUint8 theChar)
{
    CsrBool retVal = FALSE;

    switch (theChar)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            {
                retVal = TRUE;
                break;
            }
    }

    return retVal;
}

/*************************************************************************************
 getByteValue : Return the received value
************************************************************************************/
static CsrBool getByteValue(CsrUint8 **line, CsrUint32 *tmpValue)
{
    CsrBool valueFound = FALSE;
    *tmpValue = 0;

    while ((**line != '\0') && (IS_DIGIT_CHAR(**line)))
    {
        *tmpValue = (CsrUint32)(*tmpValue * 10 + **line - '0');
        (*line)++;
        valueFound = TRUE;
    }

    return valueFound;
}

/* Returns TRUE if 'cmd' can be parsed correctly else returns FALSE */
static CsrBool parseBvraCmd(HfInstanceData_t *linkPtr,
                              CsrUint8 *cmd,
                              CsrBool *started,
                              CsrBtHfpVreState *vreState,
                              CsrCharString **textId,
                              CsrBtHfpVrTxtType *textType,
                              CsrBtHfpVrTxtOp *textOperation,
                              CsrCharString **string)
{
    /* CsrStringToken() uses standard strtok_s or strtok_r for non-hydra platform.
       If there is no delimeter found in non-empty "context" string passed, CsrStringToken()
       returns the same "context" string as the token when standard function is used whereas it
       returns NULL for Hydra platform. Due to this behavioural difference, we can't have same
       delimeter for both platforms.
       If AG does not support Enhanced voice recognition feature, '\r' in delimeter string helps
       to retrieve the Non-NULL token from the AT string "+BVRA:1" or "+BVRA:0" */
#ifdef HYDRA
    const CsrCharString *delim = ",\r";
#else
    const CsrCharString *delim = ",";
#endif
    CsrCharString *next_token = NULL;
    CsrCharString *l_cmd = (CsrCharString *)CsrStrDup((CsrCharString *)cmd);
    CsrCharString *token;

    /* VRE Started or Stopped */
    token = CsrStringToken(l_cmd, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(l_cmd);
        return FALSE;
    }
    *started = (CsrStrToInt(token) != 0) ? TRUE : FALSE;

    if ((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_ENHANCE_VOICE_RECOG_STATUS) &&
        (CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->localSupportedFeatures & CSR_BT_HF_SUPPORT_ENHANCE_VOICE_RECOG_STATUS))
    {
        /* Fetch VRE State */
        token = CsrStringToken(NULL, delim, &next_token);
        if (!(token))
        {
            CsrPmemFree(l_cmd);
            /* No valid VRE state found.
               Return true as we have valid feature enable/disable value */
            return TRUE;
        }
        *vreState = CsrStrToInt(token);
    }

    if ((linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_VOICE_RECOG_TEXT) &&
        (CSR_BT_HF_MAIN_INSTANCE_GET(linkPtr)->localSupportedFeatures & CSR_BT_HF_SUPPORT_VOICE_RECOG_TEXT))
    {
        /* Fetch ID of the current text */
        token = CsrStringToken(NULL, delim, &next_token);
        if (!(token))
        {
            CsrPmemFree(l_cmd);
            /* No text data, as valid "textID" not present.
               Return true to send VR Status and VRE State fields. */
            return TRUE;
        }
        *textId = CsrStrDup(token);

        /* Fetch ID of the textType */
        token = CsrStringToken(NULL, delim, &next_token);
        if (!(token))
        {
            CsrPmemFree(l_cmd);
            return FALSE;
        }
        *textType = CsrStrToInt(token);

        /* Fetch ID of the operation of the text */
        token = CsrStringToken(NULL, delim, &next_token);
        if (!(token))
        {
            CsrPmemFree(l_cmd);
            return FALSE;
        }
        *textOperation = CsrStrToInt(token);

        /* Fetch text message.
           We can not use "," delimeter as text string may contain a ",".
           Instead just use "\r" to read complete text string.
           AT parser makes sure that this character is present in received AT data. */
        delim = "\r";
        token = CsrStringToken(NULL, delim, &next_token);
        if (!(token))
        {
            CsrPmemFree(l_cmd);
            return FALSE;
        }
        else
        {
            CsrUint16 textLen = (CsrUint16)CsrStrLen(token);

            /* Allocate full string if received text length is less than or equal to
             * configured size or HF_MAX_BVRA_TEXT_SIZE is configured 0 */
            if (textLen <= HF_MAX_BVRA_TEXT_SIZE ||
                (textLen - HF_MAX_BVRA_TEXT_SIZE == textLen))
            {
                *string = CsrStrDup(token);
            }
            else
            {
                /* Allocate just the configurable string size */
                *string = CsrMemDup(token, HF_MAX_BVRA_TEXT_SIZE + 1);
                (*string)[HF_MAX_BVRA_TEXT_SIZE - 1] = '"';
                (*string)[HF_MAX_BVRA_TEXT_SIZE] = '\0';
            }
        }
    }

    CsrPmemFree(l_cmd);

    return TRUE;
}

/*************************************************************************************
*   IsSlcEstablished: ready to send and receive data?
**************************************************************************************/
CsrBool IsSlcEstablished(HfMainInstanceData_t *instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
    {
        if (linkPtr->atSequenceState == rest)
        {
            return TRUE;
        }
    }
    else
    { /* HS_CONNECTION: if we have received data we are connected basically... */
        return TRUE;
    }
    return FALSE;
}

/*************************************************************************************
    Send a CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM to app
************************************************************************************/
void CsrBtHfSendHfCurrentOperatorSelectionCfm(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrBtHfGetCurrentOperatorSelectionCfm    *prim;
    CsrUint8 len = 0;

    if (atTextString != NULL)
    {
        len = (CsrUint8)CsrStrLen((char *)atTextString)+1;
    }

    prim = (CsrBtHfGetCurrentOperatorSelectionCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetCurrentOperatorSelectionCfm));
    prim->type = CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM;
    prim->connectionId = instData->linkData[instData->index].hfConnId;

    prim->copsString = NULL;

    if (atTextString != NULL)
    {
        prim->copsString = CsrPmemAlloc(len);
        CsrStrNCpyZero((char *)prim->copsString,(char *)atTextString,len);
    }
    prim->cmeeResultCode = CSR_BT_CME_SUCCESS;

    CsrBtHfMessagePut(instData->appHandle, prim);
}

/*************************************************************************************
*  Handle OK response: OK_TOKEN
**************************************************************************************/
CsrBool CsrBtHfOkHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
   CsrBool returnValue = FALSE;
   HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

   CSR_UNUSED(atTextString);

    cancelAtResponseTimer(instData);

    if ((linkPtr->state == Connect_s) && (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS))
    {
        CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
        linkPtr->lastAtCmdSent = idleCmd;
        if (linkPtr->scoHandle != HF_SCO_UNUSED)
        {/* this means that the HS have received a CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM in connect state */
            CsrBtHfSendHfAudioConnectInd(instData, linkPtr->pcmSlot, 0, 0, 0, 0, 0, 0,
                                         CSR_BT_RESULT_CODE_HF_SUCCESS, 0xDEAD, CSR_BT_SUPPLIER_HF,
                                         CSR_BT_HF_AUDIO_CONNECT_IND);
        }
        else
        {
            linkPtr->state = Connected_s;
        }

        returnValue = TRUE;
    }
    else
    {
        if ((linkPtr->state == ServiceSearch_s) && (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF))
        {
            linkPtr->lastAtCmdSent = idleCmd;
            if (linkPtr->pendingCancel)
            {/* Just make sure the data pointer is freed and wait for disconnection.... */
                return TRUE;
            }
            if (linkPtr->atSequenceState == supportFeatures)
            {
                linkPtr->serviceState = serviceConnect_s;
                if ( /* (linkPtr->remoteVersion == CSR_BT_FIRST_HFP_CODEC_NEG_ESCO) && */
                    (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) &&
                    (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION))
                {/* Codec negotiation supported locally and by the remote */
                    linkPtr->atSequenceState = codecSupport;
                    sendCodecSupport(instData);
                }
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
                else if (instData->hfQceCodecMask != CSR_BT_HF_QCE_UNSUPPORTED
                         && (!CmIsSWBDisabled(&instData->currentDeviceAddress))
                        )
                {
                    linkPtr->atSequenceState = qceSupport;
                    /* Prepare and send AT+QAC command indicating support for all the
                       Qualcomm extended codecs.*/
                    sendQac(instData);
                }
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
                else
                {
                    linkPtr->atSequenceState = cindSupport;
                    sendCindSupport(instData);
                }
                returnValue = TRUE;
            }
            else if (linkPtr->atSequenceState == codecSupport)
            {
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
                if (instData->hfQceCodecMask != CSR_BT_HF_QCE_UNSUPPORTED
                    && (!CmIsSWBDisabled(&instData->currentDeviceAddress))
                   )
                {
                    linkPtr->atSequenceState = qceSupport;
                    /* Prepare and send AT+QAC command indicating support for all the
                       Qualcomm extended codecs.*/
                    sendQac(instData);
                }
                else
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
                {
                    linkPtr->atSequenceState = cindSupport;
                    sendCindSupport(instData);
                }
                returnValue = TRUE;
            }
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
            else if (linkPtr->atSequenceState == qceSupport)
            {
                    linkPtr->atSequenceState = cindSupport;
                    sendCindSupport(instData);
                    returnValue = TRUE;
            }
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
            else if (linkPtr->atSequenceState == cindSupport)
            {
                linkPtr->atSequenceState = cindStatus;
                sendCindStatus(instData);
                returnValue = TRUE;
            }
            else if ((linkPtr->atSequenceState == cindStatus) &&
                    ((instData->mainConfig & CSR_BT_HF_CNF_DISABLE_CMER_UNDER_SLC_ESTABLISHMENT) == 0))
            {
                linkPtr->atSequenceState = eventReport;
                sendSetCmer(instData,TRUE);
                returnValue = TRUE;
            }
            else if ((linkPtr->atSequenceState == eventReport) ||
                ((linkPtr->atSequenceState == cindStatus) && (instData->mainConfig & CSR_BT_HF_CNF_DISABLE_CMER_UNDER_SLC_ESTABLISHMENT)))
            {   /* Finished setting up ServiceConnection */
                linkPtr->atSequenceState = serviceLevel;
                CsrBtHfSendHfHouseCleaning(instData);
                linkPtr->data->dataReceivedInConnected = FALSE;
                linkPtr->state = Connected_s;
                linkPtr->atSequenceState = rest;
                /* check if we want to send CHLD command */
                if ( (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING) &&
                     (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_THREE_WAY_CALLING) &&
                     (!(instData->mainConfig & CSR_BT_HF_CNF_DISABLE_CHLD_UNDER_SLC_ESTABLISHMENT)))
                {
                    /* Both HF and HFG supports 3-way calling */
                    linkPtr->atSequenceState = callHold;
                    sendCallHoldStatus(instData);
                }
                else if((instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
                        (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
                {
                    linkPtr->atSequenceState = hfIndSupport;
                    sendHfSupportedHfInd(instData);
                }
                else
                {/* It is first now that the SLC is established! */
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
                returnValue = TRUE;
            }
            else if (linkPtr->atSequenceState == callHold)
            {
                /* Finished setting up ServiceConnection */

                if((instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
                   (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
                {
                    linkPtr->atSequenceState = hfIndSupport;
                    sendHfSupportedHfInd(instData);
                }
                else if (linkPtr->disconnectReqReceived)
                {
                    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                }
                else
                {
                    linkPtr->atSequenceState = serviceLevel;
                    CsrBtHfSendHfHouseCleaning(instData);
                    linkPtr->data->dataReceivedInConnected = FALSE;
                    linkPtr->state = Connected_s;
                    linkPtr->atSequenceState = rest;

                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
                returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == hfIndSupport)
            {
                    linkPtr->atSequenceState = hfIndQuery;
                    queryAgSupportedHfInd(instData);
                    returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == hfIndQuery)
            {
                    linkPtr->atSequenceState = enableHfIndQuery;
                    queryAgEnabledHfInd(instData);
                    returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == enableHfIndQuery)
            {
                if (linkPtr->disconnectReqReceived)
                {
                    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                }
                else
                {
                    linkPtr->atSequenceState = serviceLevel;
                    CsrBtHfSendHfHouseCleaning(instData);
                    linkPtr->data->dataReceivedInConnected = FALSE;
                    linkPtr->state = Connected_s;
                    linkPtr->atSequenceState = rest;

                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
                returnValue = TRUE;
            } 

        }
        else
        {
            if (linkPtr->atSequenceState == callHold)
            {
                linkPtr->lastAtCmdSent = idleCmd;
                if((instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS) &&
                        (linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_HF_INDICATORS))
                {
                    linkPtr->atSequenceState = hfIndSupport;
                    sendHfSupportedHfInd(instData);
                }
                else
                {
                    linkPtr->atSequenceState = rest;
                    if (linkPtr->disconnectReqReceived)
                    {
                        CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                    }
                    else
                    {
                        CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                        CsrBtHfAcceptIncomingSco(linkPtr);
                    }
                }
                returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == hfIndSupport)
            {
                    linkPtr->lastAtCmdSent = idleCmd;
                    linkPtr->atSequenceState = hfIndQuery;
                    queryAgSupportedHfInd(instData);
                    returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == hfIndQuery)
            {
                    linkPtr->lastAtCmdSent = idleCmd;
                    linkPtr->atSequenceState = enableHfIndQuery;
                    queryAgEnabledHfInd(instData);
                    returnValue = TRUE;
            }
            else if(linkPtr->atSequenceState == enableHfIndQuery)
            {
                linkPtr->lastAtCmdSent = idleCmd;
                linkPtr->atSequenceState = rest;
                if (linkPtr->disconnectReqReceived)
                {
                    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                }
                else
                {
                    CsrBtHfSendHfServiceConnectInd(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
                    CsrBtHfAcceptIncomingSco(linkPtr);
                }
                returnValue = TRUE;
            }
            else if (linkPtr->atSequenceState == copsQuery)
            {
                linkPtr->lastAtCmdSent = idleCmd;
                CsrBtHfAtCopsQuerySend(instData);
                linkPtr->atSequenceState = rest;
                returnValue = TRUE;
            }
            else
            {
                returnValue = SendConfirmMessage(instData, CSR_BT_CME_SUCCESS);
            }
      }
   }
   return returnValue;
}

/*************************************************************************************
*  Handle ERROR response: ERROR_TOKEN
**************************************************************************************/
CsrBool CsrBtHfCmeErrorHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        CsrBtHfSendHfCmeeInd(instData, atTextString);
    }
    return TRUE;
}

/*******************************************************************************************
*  Handle alternative error responses: busy, no carrier, no answer, delayed and blacklisted
*  as specified in version 1.05 of the HFP
********************************************************************************************/
CsrBool CsrBtHfBusyHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        SendConfirmMessage(instData,CSR_BT_CME_BUSY);
    }
    return TRUE;
}

CsrBool CsrBtHfNoCarrierHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        SendConfirmMessage(instData, CSR_BT_CME_NO_CARRIER);
    }
    return TRUE;
}

CsrBool CsrBtHfNoAnswerHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        SendConfirmMessage(instData, CSR_BT_CME_NO_ANSWER);
    }
    return TRUE;
}

CsrBool CsrBtHfDelayedHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        SendConfirmMessage(instData, CSR_BT_CME_DELAYED);
    }
    return TRUE;
}

CsrBool CsrBtHfBlacklistedHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        cancelAtResponseTimer(instData);
        SendConfirmMessage(instData, CSR_BT_CME_BLACKLISTED);
    }
    return TRUE;
}

/*************************************************************************************
*  Handle ERROR response: ERROR_TOKEN
**************************************************************************************/
CsrBool CsrBtHfErrorHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrBool returnValue = FALSE;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    CSR_UNUSED(atTextString);

    cancelAtResponseTimer(instData);

    if ((linkPtr->state == ServiceSearch_s) && (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF))
    {/* Hfg does not support version 1.0, continue AT sequence. Use supported features
        found during service search or default value (saved in instance data) */
        if (linkPtr->atSequenceState == supportFeatures)
        {
            linkPtr->atSequenceState = cindSupport;
            linkPtr->serviceState = serviceConnect_s;
            if (linkPtr->remoteVersion == 0)
            {
                linkPtr->remoteVersion = CSR_BT_FIRST_HFP_NO_ESCO; /* 0.96 */
            }
            sendCindSupport(instData);
            returnValue = TRUE;
        }
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
        else if (linkPtr->atSequenceState == qceSupport)
        {
            linkPtr->atSequenceState = cindSupport;
            sendCindSupport(instData);
            returnValue = TRUE;
        }
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
        else
        {
            returnValue = SendConfirmMessage(instData,CSR_BT_CME_AG_FAILURE);
        }
    }
    else
    {
        if(linkPtr->atSequenceState == copsQuery)
        {
            CsrBtHfSendHfCurrentOperatorSelectionCfm(instData, atTextString);
            linkPtr->atSequenceState = rest;
            returnValue = TRUE;
        }
        else
        {
            returnValue = SendConfirmMessage(instData,CSR_BT_CME_AG_FAILURE);
        }
    }
    return returnValue;
}

/*************************************************************************************
*  Handle VGS message
**************************************************************************************/
CsrBool CsrBtHfVgsSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrUint32 value = 0;
        CsrUint8 *index_i;
        HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

        atTextString++;/* skip the first charachter; it will be ':' or '=' */

        index_i = atTextString;
        index_i = (CsrUint8 *)CsrStrChr((char *)index_i,' '); /* There might be spaces after the separator charachter */
        if (index_i == NULL)
        {
            index_i = atTextString;
        }
        else
        {/* jump to the next charachter */
            index_i++;
        }

        if (getByteValue(&index_i,&value))
        {
            if (value <= CSR_BT_MAX_VGS_VALUE)
            {
                if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
                {
                    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_REMOTE_VOLUME_CONTROL)
                    {
                        /* only send Indication if the device supports RemoteVolumeControl */
                        CsrBtHfSendHfSpeakerGainInd(instData, (CsrUint8) value);
                    }
                }
                else if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS)
                {
                    if (!(instData->mainConfig & CSR_BT_HF_CNF_DISABLE_REMOTE_VOLUME_CONTROL))
                    {
                        /* only send Indication if the device supports RemoteVolumeControl */
                        CsrBtHfSendHfSpeakerGainInd(instData, (CsrUint8) value);
                    }
                }
            }
        }
    }
    return TRUE;
}

/*************************************************************************************
*  Handle VGM message
**************************************************************************************/
CsrBool CsrBtHfVgmSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrUint32 value = 0;
        CsrUint8 *index_i;
        HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

        atTextString++;/* skip the first charachter; it will be ':' or '=' */

        index_i = atTextString;
        index_i = (CsrUint8 *)CsrStrChr((char *)index_i,' '); /* There might be spaces after the separator charachter */
        if (index_i == NULL)
        {
            index_i = atTextString;
        }
        else
        {/* jump to the next charachter */
            index_i++;
        }

        if (getByteValue(&index_i,&value))
        {
            if (value <= CSR_BT_MAX_VGM_VALUE)
            {
                if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
                {
                    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_REMOTE_VOLUME_CONTROL)
                    {
                        /* only send Indication if the device supports RemoteVolumeControl */
                        CsrBtHfSendHfMicGainInd(instData, (CsrUint8) value);
                    }
                }
                else if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS)
                {
                    if (!(instData->mainConfig & CSR_BT_HF_CNF_DISABLE_REMOTE_VOLUME_CONTROL))
                    {
                        /* only send Indication if the device supports RemoteVolumeControl */
                        CsrBtHfSendHfMicGainInd(instData, (CsrUint8) value);
                    }
                }
            }
        }
    }
    return TRUE;
}

/*************************************************************************************
*  Handle BRSF message
**************************************************************************************/
CsrBool CsrBtHfBrsfHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrUint32 value = 0;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (getByteValue(&atTextString,&value))
    {
        linkPtr->supportedFeatures = value;
        if (linkPtr->remoteVersion == 0)
        {/* remote version yet unknown; guess out of the value received. 
            Bit 10 is Codec Negotiation - in V1.6 only
            Bit 6 gives a clue: if it is 1 then at least version 1.05; else version 1.0 */
            if (value & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION)
            {
                linkPtr->remoteVersion = CSR_BT_FIRST_HFP_CODEC_NEG_ESCO; /* 1.06 */
            }
            else if (value & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_STATUS)
            {
                linkPtr->remoteVersion = CSR_BT_FIRST_HFP_ESCO; /* 1.05 */
            }
            else
            {
                linkPtr->remoteVersion = CSR_BT_LAST_HFP_NO_ESCO; /* 1.00 */
            }
        }
        /* Make sure that the network attribute has the proper value; must be the same as bit 5 of the BRSF value received.
           Per default it is set to 1 before service search in order to follow the spec....*/
        if (value & CSR_BT_HFG_SUPPORT_ABILITY_TO_REJECT_CALL)
        {
            linkPtr->network = 1;
        }
        else
        {
            linkPtr->network = 0;
        }
    }

    return TRUE;
}

/*************************************************************************************
*  Handle BTRH message
**************************************************************************************/
CsrBool CsrBtHfBtrhHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfSendHfCallHandlingInd(instData, atTextString);
    }
    return TRUE;
}

/*************************************************************************************
*  Handle RING message
**************************************************************************************/
CsrBool CsrBtHfRingHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CSR_UNUSED(atTextString);
    if (IsSlcEstablished(instData))
    {
        CsrBtHfSendHfRingInd(instData);
    }
    return TRUE;
}

/*************************************************************************************
*  Handle BSIR message
**************************************************************************************/
CsrBool CsrBtHfBsirHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrUint32 value = 0;

        if (getByteValue(&atTextString,&value))
        {
            CsrBtHfSendHfInBandRingToneInd(instData, (CsrBool) value);
        }
    }
    return TRUE;
}

/*************************************************************************************
*  Handle COPS message
**************************************************************************************/
CsrBool CsrBtHfCopsHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrBtHfSendHfCurrentOperatorSelectionCfm(instData, atTextString);
    instData->linkData[instData->index].atSequenceState = rest;
    return TRUE;
}

/*************************************************************************************
    Send a HF_CLCC_IND to app
************************************************************************************/
CsrBool CsrBtHfSendHfClccInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfGetCurrentCallListInd *prim;
        CsrUint8 len = (CsrUint8)CsrStrLen((char *)atTextString)+1;

        prim = (CsrBtHfGetCurrentCallListInd *)CsrPmemAlloc(sizeof(CsrBtHfGetCurrentCallListInd));
        prim->type = CSR_BT_HF_GET_CURRENT_CALL_LIST_IND;
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        prim->clccString = CsrPmemAlloc(len);
        CsrStrNCpyZero((char *)prim->clccString,(char *)atTextString,len);

        CsrBtHfMessagePut(instData->appHandle, prim);
    }

    return TRUE;
}

/*************************************************************************************
    Send a CSR_BT_HF_CALL_NOTIFICATION_IND to app
************************************************************************************/
CsrBool CsrBtHfSendClipInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfCallNotificationInd *prim;
        CsrUint8 len = (CsrUint8)CsrStrLen((char *)atTextString)+1;

        prim = (CsrBtHfCallNotificationInd*)CsrPmemAlloc(sizeof(CsrBtHfCallNotificationInd));
        prim->type = CSR_BT_HF_CALL_NOTIFICATION_IND;
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        prim->clipString = CsrPmemAlloc(len);
        CsrStrNCpyZero((char *)prim->clipString,(char *)atTextString,len);

        CsrBtHfMessagePut(instData->appHandle, prim);
    }
    return TRUE;
}


/*************************************************************************************
    Send a CSR_BT_HF_CALL_WAITING_NOTIFICATION_IND to app
************************************************************************************/
CsrBool CsrBtHfSendCcwaInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfCallWaitingNotificationInd *prim;
        CsrUint8 len = (CsrUint8)CsrStrLen((char *)atTextString)+1;

        prim = (CsrBtHfCallWaitingNotificationInd*)CsrPmemAlloc(sizeof(CsrBtHfCallWaitingNotificationInd));
        prim->type = CSR_BT_HF_CALL_WAITING_NOTIFICATION_IND;
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        prim->ccwaString = CsrPmemAlloc(len);
        CsrStrNCpyZero((char *)prim->ccwaString,(char *)atTextString,len);

        CsrBtHfMessagePut(instData->appHandle, prim);
    }
    return TRUE;
}

/*************************************************************************************
    Send a CSR_BT_HF_BT_INPUT_CFM to app
************************************************************************************/
CsrBool CsrBtHfSendBtInputCfm(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfBtInputCfm *prim;
        CsrUint8 len = 1;

        if (atTextString != NULL)
        {
          len += (CsrUint8)CsrStrLen((char *)atTextString);
        }

        prim = (CsrBtHfBtInputCfm*)CsrPmemAlloc(sizeof(CsrBtHfBtInputCfm));
        prim->type = CSR_BT_HF_BT_INPUT_CFM;
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        prim->cmeeResultCode = CSR_BT_CME_SUCCESS;
        if (len > 1)
        {
            prim->dataRespString = CsrPmemAlloc(len);
            CsrStrNCpyZero((char *)prim->dataRespString,(char *)atTextString,len);
        }
        else
        {
            prim->dataRespString = NULL;
        }

        CsrBtHfMessagePut(instData->appHandle, prim);
    }
    return TRUE;
}

/*************************************************************************************
    Send a CSR_BT_HF_SET_VOICE_RECOGNITION_IND to app
************************************************************************************/
CsrBool CsrBtHfBvraHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfSetVoiceRecognitionInd *prim = CsrPmemAlloc(sizeof(*prim));
        HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

        prim->type         = CSR_BT_HF_SET_VOICE_RECOGNITION_IND;
        prim->connectionId = linkPtr->hfConnId;
        prim->textId = NULL;
        prim->string = NULL;

        if (parseBvraCmd(linkPtr,
                         atTextString,
                         &prim->started,
                         &prim->vreState,
                         &prim->textId,
                         &prim->textType,
                         &prim->textOperation,
                         &prim->string))
        {
            CsrBtHfMessagePut(instData->appHandle, prim);
        }
        else
        {
            CsrPmemFree(prim);
        }
    }
    return TRUE;
}

/*************************************************************************************
    Send a HF_CNUM_IND to app
************************************************************************************/
CsrBool CsrBtHfSendHfCnumInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrBtHfGetSubscriberNumberInformationInd    *prim;
        CsrUint8 len = (CsrUint8)CsrStrLen((char *)atTextString)+1;

        prim = (CsrBtHfGetSubscriberNumberInformationInd *)CsrPmemAlloc(sizeof(CsrBtHfGetSubscriberNumberInformationInd));
        prim->type = CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_IND;
        prim->connectionId = instData->linkData[instData->index].hfConnId;
        prim->cnumString = CsrPmemAlloc(len);
        CsrStrNCpyZero((char *)prim->cnumString,(char *)atTextString,len);

        CsrBtHfMessagePut(instData->appHandle, prim);
    }
    return TRUE;
}

/*************************************************************************************
    Send a HF_CMEE_IND to app
************************************************************************************/
void CsrBtHfSendHfCmeeInd(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    char *index_i;
    char *index_j;

    index_i = (char *)atTextString;
    index_i = CsrStrChr(index_i,':');

    if(index_i != NULL)
    {
        if(index_i[1] == ' ')
        {
            index_i++;
        }
    }
    if(index_i!=NULL)
    {
        CsrUint8 length;
        CsrUint8 *temp;
        CsrUint16 error = 0;

        index_j = CsrStrChr(index_i,'\r');
        length = (CsrUint8)(StrATLen(index_i+1) - StrATLen(index_j));
        temp = CsrPmemAlloc(length);
        SynMemCpyS(temp, length, index_i+1,length);

        if(length == 1)
        {
            error = *temp - '0';
        }
        else if(length == 2)
        {
            error = (temp[0] - '0')*10 + (temp[1] - '0');
        }

        CsrPmemFree(temp);

        SendConfirmMessage(instData, error);
    }
    instData->linkData[instData->index].lastAtCmdSent = idleCmd;
}

/*************************************************************************************
    Store HF_CHLD_IND information in the link instance for later use
************************************************************************************/
CsrBool CsrBtHfStoreChldInfo(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
    {
        CsrUint16 length;
        char    * chldStart;

        chldStart = (char *)atTextString;
        while ((*chldStart != '(') && (*chldStart != '\r'))
        {
            chldStart++;
        }
        length = 0;
        while (((*(chldStart + length)) != ')') && ((*(chldStart + length)) != '\r'))
        {
            length++;
        }
        length++;

        /* Check whether length is valid before copying the +CHLD: <values> */
        if (length >= HF_CHLD_MAX_STR_SIZE)
        {
            length = HF_CHLD_MAX_STR_SIZE - 1;
        }

        CsrMemSet(linkPtr->agIndicators.chldStringStored, 0, HF_CHLD_MAX_STR_SIZE);

        CsrStrNCpyZero((CsrCharString *) linkPtr->agIndicators.chldStringStored,
                       chldStart,
                       length + 1);
    }
    return TRUE;
}

/*************************************************************************************
    Send a CSR_BT_HF_STATUS_INDICATOR_UPDATE_IND to app
*************************************************************************************/
static void csrBtHfSendStatusIndicatorUpdateInd(HfMainInstanceData_t *instData,
                          CsrUint32               theIndex,
                          CsrUint32               theValue)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if (linkPtr->agIndicators.cindData.instCount > 0)
    {
        CsrBtHfStatusIndicatorUpdateInd *prim;
        CsrUint32       x=0, cindSupportStringLen;
        CsrUint16       searchIndex;
        CsrUint8       *startName, *cindString;
        CsrUint8        lengthName;
        CsrBool         startFound, stop;

        cindString  = CsrBtHfDecodeCindString(&linkPtr->agIndicators.cindData);

        if(cindString == NULL)
        {
            return;
        }

        prim = (CsrBtHfStatusIndicatorUpdateInd *)CsrPmemAlloc(sizeof(CsrBtHfStatusIndicatorUpdateInd));
        prim->type  = CSR_BT_HF_STATUS_INDICATOR_UPDATE_IND;
        prim->value = (CsrUint8)theValue;
        prim->connectionId = linkPtr->hfConnId;
        prim->index = (CsrUint8)theIndex;

        startName   = (&cindString[0]) - 1;
        lengthName  = 0;
        searchIndex = 0;

        cindSupportStringLen = (CsrUint32) CsrStrLen((char *)cindString);

        while ((x<cindSupportStringLen) && (searchIndex != theIndex))
        {
            searchIndex++;
            stop       = FALSE;
            startFound = FALSE;
            startName  = (startName + lengthName + 1);
            lengthName = 0;
            while ((!stop) && (x<cindSupportStringLen))
            {
                x++;
                if (startFound)
                {
                    lengthName++;
                    if (*(startName + lengthName) == '"')
                    {
                        stop = TRUE;
                    }
                }
                else
                {
                    if (*startName == '"')
                    {
                        startFound = TRUE;
                        startName++;
                    }
                    else
                    {
                        startName++;
                    }
                }
            }
        }

        if (x<cindSupportStringLen)
        {
            prim->name = CsrPmemAlloc(lengthName+1);
            SynMemCpyS(prim->name, lengthName+1, startName, lengthName);
            prim->name[lengthName] = '\0';
            CsrBtHfMessagePut(instData->appHandle, prim);
        }
        else
        {/* If not sent, remember to free the heap area! */
            CsrPmemFree(prim);
        }

        CsrPmemFree(cindString);
    }
}

CsrBool CsrBtHfFindCievValue(HfMainInstanceData_t *instData, CsrUint8  *atTextString)
{
    if (IsSlcEstablished(instData))
    {
        CsrUint8 * cindString;
        CsrUint8 valueLength;
        CsrUint32 valueIndex, value;

        cindString = atTextString;

        /* skip initial spaces */
        while (CsrIsSpace(*cindString))
        {
            cindString++;
        }
        valueLength = 0;
        while ((*(cindString + valueLength) != ',') && (*(cindString + valueLength) != '\r'))
        {
            valueLength++;
        }

        *(cindString + valueLength) = '\0';
        if (getByteValue(&cindString, &valueIndex))
        {
            cindString++;
        }
        /* Reset valueLength before reading next value */
        valueLength = 0;
        while (*(cindString + valueLength) != '\r')
        {
            valueLength++;
        }
        *(cindString + valueLength) = '\0';
        if (getByteValue(&cindString, &value))
        {
            cindString++;
        }
        csrBtHfSendStatusIndicatorUpdateInd(instData, valueIndex, value);
    }

    return TRUE;
}

CsrBool CsrBtHfCindHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrUint32 cindStringLength = 0;
    CsrUint8 *atString;
    CsrUint8 *cindStringPP=NULL;


    atString = atTextString;

    while(*atString != '\r')
    {
         cindStringLength++;
         atString++;
    }

    if(linkPtr->disconnectReqReceived == FALSE && instData->deactivated == FALSE)
    {
		if (linkPtr->atSequenceState == cindSupport && *atTextString == '(')
		{
			CsrBtHfEncodeCindString(atTextString, cindStringLength, &linkPtr->agIndicators.cindData);
		}
		else if (linkPtr->atSequenceState == cindStatus)
		{
			cindStringPP = (CsrUint8 *)&linkPtr->agIndicators.cindStartValueString;
			CsrStrNCpyZero(((char *)(cindStringPP)), (char *)atTextString, cindStringLength + 1);
		}
    }
    return TRUE;
}

static CsrBool hfValidateOptionalCodec(HfMainInstanceData_t *instData)
{
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrUint8 i;
    CsrBool retValue = FALSE;

    for (i = 0; i < instData->optionalCodecCount; i++)
    {
        if (instData->optionalCodecList[i] == linkPtr->codecToUse)
        {
            retValue = TRUE;
        }
    }
    return retValue;
#else /* !HF_ENABLE_OPTIONAL_CODEC_SUPPORT */
    CSR_UNUSED(instData);
    return FALSE;
#endif
}

/*****************************************************************************************
  handle +BCS response code
*****************************************************************************************/
CsrBool CsrBtHfCodecAcceptable(HfMainInstanceData_t *instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrBool retValue = FALSE;
    CsrUint8 mask =  (1 << (linkPtr->codecToUse - 1));

    if ((mask & instData->supportedCodecsMask) || hfValidateOptionalCodec(instData))
    {
        retValue = TRUE;
    }
    return retValue;
}

static void csrBtHfHandleCodecNeg(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrUint8 *index_i;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrBool newAcceptSettings = FALSE;

    index_i = atTextString;
    index_i = (CsrUint8 *)CsrStrChr((char *)index_i,' '); /* There might be spaces after the separator charachter */
    if (index_i == NULL)
    {
        index_i = atTextString;
    }
    else
    {/* jump to the next character */
        index_i++;
    }

    if (linkPtr->codecToUse != (CsrUint8)CsrStrToInt((char*)index_i))
    {
        newAcceptSettings = TRUE;
    }
    linkPtr->codecToUse = (CsrUint8)CsrStrToInt((char*)index_i);
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    /* Mark the qceCodecId to unsupported since the Ag has negotiated the codec again. */
    linkPtr->hfQceCodecId = CSR_BT_HF_QCE_UNSUPPORTED;
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    /* if the codec suggestion from the HFG is acceptable, answer with AT+BCS= "<codecId>";
       else, answer with AT+BAC="<codec1>","<codec2>",... */
    if (CsrBtHfCodecAcceptable(instData))
    {/* AT+BCS; accept */
        CsrBtHfSendAtBcs(instData);

        if (newAcceptSettings)
        {/* Make sure to cancel accept of old settings and start accepting the new ones */
            CsrBtCmScoCancelReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
            linkPtr->scoConnectAcceptPending = FALSE;
        }
        /* Codec agreed upon: accept incoming SCO connection */
        CsrBtHfAcceptIncomingSco(linkPtr);

        if (!instData->deferSelCodecInd)
        {
            /* Send selected codec indication to application.

              Here we will not wait for confirmation('OK') to AT+BCS command because of below reasons:
              a. With some devices confirmation('OK') to AT+BCS was getting received after audio connection establihment
                 and thus CSR_BT_HF_AUDIO_CONNECT_IND was geting sent to host before CSR_BT_HF_SELECTED_CODEC_IND.
              a. Also, as per HFP specification if HF responds with AT+BCS=<codec> with same codec value as in +BCS:<codec>
                 than AG has to always respond this AT with 'OK', so there is no need to wait for the 'OK' when HF 
                 accepts the sent codec from AG.
            */
            CsrBtHfSendSelectedCodecInd(instData);
        }
    }
    else
    {/* AT+BAC; codec not acceptable */
        sendCodecSupport(instData);
    }
}

CsrBool CsrBtHfCodecNegSet(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    /* If the SLC connection is not established,
     * don't send CSR_BT_HF_SELECTED_CODEC_IND. This is done to avoid
     * IOP issues as there are some phones in the market which sends codec
     * negotiation before the SLC connection is established. 
     * For such cases, CSR_BT_HF_SELECTED_CODEC_IND will be sent along
     * with CSR_BT_HF_AUDIO_CONNECT_IND. */
    instData->deferSelCodecInd = !IsSlcEstablished(instData);

    csrBtHfHandleCodecNeg(instData, atTextString);

    return TRUE;
}

/*****************************************************************************************
  handle the AT-command string and take appropriate action.
*****************************************************************************************/

/* Local: AT compare function. Perform a very safe comparison of the
 * atStr against maStr, with full range checking etc. Set 'index' to
 * the first occurrence of any trailing non-whitespace
 * (ie. values). */
static CsrBool csrBtHfCompare(char *atStr, CsrUint16 atLen, char *maStr, CsrUint16 *index)
{
    CsrUint16 atIdx;
    CsrUint16 maIdx;
    CsrUint16 maLen;

    /* Dummy checks*/
    if((atStr == NULL) ||
       (maStr == NULL) ||
       (index == NULL) ||
       (atLen < CsrStrLen(maStr)))
    {
        return FALSE;
    }

    atIdx = 0;
    maIdx = 0;
    *index = 0;
    maLen = (CsrUint16)CsrStrLen(maStr);

    /* Scan characters */
    while((maIdx < maLen) &&
          (atIdx < atLen))
    {
        /* Skip whitespace */
        while((atStr[atIdx] == ' ') ||
           (atStr[atIdx] == '\n') ||
           (atStr[atIdx] == '\r') ||
           (atStr[atIdx] == '\t'))
        {
            atIdx++;
        }

        /* Does AT command match? */
        if(TO_UPPER(atStr[atIdx]) != maStr[maIdx])
        {
            /* Mismatch, break out now */
            return FALSE;
        }

        /* Next set of character */
        atIdx++;
        maIdx++;
    }

    /* Skip any last whitespaces */
    while((atIdx < atLen) &&
          ( (atStr[atIdx] == ' ') ||
            (atStr[atIdx] == '\n') ||
            (atStr[atIdx] == '\r') ||
            (atStr[atIdx] == '\t')) )
    {
        atIdx++;
    }

    /* If index has reached the end, it's an actual match */
    if(maIdx == maLen)
    {
        *index = atIdx;
        return TRUE;
    }
    else
    {
        *index = 0;
        return FALSE;
    }
}

/*******************************************************************
 * Local: Fault-tolerant parsing of raw ATs to get token - 'at' and
 * 'atLen' is the AT input string and length. At exit 'index' is the
 * index of the first parameter (unless OTHER_TOKEN is returned)
 *******************************************************************/
static CsrUint8 csrBtHfGetIndex(char *at,CsrUint16 atLen, CsrUint16 *index)
{
    CsrUint8 i;

    /* Scan the AT command table for a token */
    i = 0;
    while(hfAtCheck[i].at != NULL)
    {
        if(csrBtHfCompare(at,atLen,hfAtCheck[i].at,index))
        {
            break;
        }
        i++;
    }

    /* The 'i' will always be valid at this point */
    return i;
}

/*******************************************************************
 * This is the function that receives the actual raw AT command.  Find
 * out if token index and call the appropiate handle if necessary
 *******************************************************************/
static void CsrBtHfHandleAtCommand(HfMainInstanceData_t *instData, CsrUint32  theConnectionId,
                         CsrUint8  *handledAtCmd, CsrUint16  handledAtCmdSize)
{
    CsrUint16 voff = 0;
    CsrUint8 index;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    /*If in Transparent mode, send all commands up to application */
    if((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)==CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)
        {
            /* Transparent AT mode - send directly upstream to app now */
                CsrBtHfAtCmdInd *prim;
                prim                 = (CsrBtHfAtCmdInd *)CsrPmemAlloc(sizeof(CsrBtHfAtCmdInd));
                prim->type           = CSR_BT_HF_AT_CMD_IND;
                prim->atCmdString    = CsrPmemAlloc(handledAtCmdSize+1);
                SynMemCpyS((CsrUint8 *)prim->atCmdString, handledAtCmdSize+1, handledAtCmd,handledAtCmdSize);
                prim->atCmdString[handledAtCmdSize] = '\0';
                prim->connectionId   = linkPtr->hfConnId;
#ifdef CSR_TARGET_PRODUCT_VM
                CSR_LOG_TEXT_INFO((CsrBtHfLto, 0, "CsrBtHfHandleAtCommand: AT Command received: %s, ConId:%d",prim->atCmdString, theConnectionId));
#endif
                CsrBtHfMessagePut(instData->appHandle, prim);
                /*Application needs to */
                linkPtr->lastAtCmdSent = idleCmd;

        }
    else
    {
        index = csrBtHfGetIndex((char*)handledAtCmd,handledAtCmdSize,&voff);
        /* Execute command if known */
        if ((hfAtCheck[index].token == CSR_BT_OTHER_TOKEN) ||
            ((hfAtCheck[index].func(instData, (CsrUint8 *)&handledAtCmd[voff])) == FALSE) )
          { /* Note that "voff" is the char index of the first AT
             * argument. Pass the address in order to save a lot of
             * variables in the AT handlers... */
             if (IsSlcEstablished(instData))
             {/* Forward the unrecognized AT command to the application only if the SLC is established */
                CsrBtHfAtCmdInd *prim;

                prim                 = (CsrBtHfAtCmdInd *)CsrPmemAlloc(sizeof(CsrBtHfAtCmdInd));
                prim->type           = CSR_BT_HF_AT_CMD_IND;
                prim->atCmdString    = CsrPmemAlloc(handledAtCmdSize+1);
                SynMemCpyS((CsrUint8 *)prim->atCmdString, handledAtCmdSize+1, handledAtCmd,handledAtCmdSize);
                prim->atCmdString[handledAtCmdSize] = '\0';
                prim->connectionId   = linkPtr->hfConnId;
#ifdef CSR_TARGET_PRODUCT_VM
                CSR_LOG_TEXT_INFO((CsrBtHfLto, 0, "CsrBtHfHandleAtCommand: AT Command received: %s, ConId:%d",prim->atCmdString, theConnectionId));
#endif
                CsrBtHfMessagePut(instData->appHandle, prim);
             }
           }
    }
    CSR_UNUSED(theConnectionId);
}

void CsrBtHfHandleMultipleAtCommand(HfMainInstanceData_t *instData, CsrUint32 theConnectionId)
{
    CsrUintFast16 i        = 0;
    CsrUint16     beginPos = 0;
    CsrUint16     endPos   = 0;
    CsrBool       handledSignal         = FALSE;
    CsrBool       lastAtCmdWasResponded = FALSE;
    CsrUint16     handledAtCmdSize      = 0;
    CsrUint8     *handledAtCmd          = NULL;
    HfHsData_t  *data                  = NULL;
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    CsrBool restartAtTimer = FALSE;


    data = linkPtr->data;

    if (data)
    {
        for ( i = 0; i < data->recvAtCmdSize; i++)
        {
            handledSignal = FALSE;

            /* The AT-parsing below is done in this way, to be most compliant with other devices out on the market.
               We are aware that the specification says that an AT command SHALL start and end with a \r\n,
               but fact is that not all devices out there follows this rule, and hence we have made the AT-parser
               generic enough to also cater for those devices. */

            /* scan the received command sequence to see if a command
               is received (cr or lf must be found) */
            if ((data->recvAtCmd[i] == '\r') || (data->recvAtCmd[i] == '\n'))
            {
                /* Found a delimiter */
                beginPos = endPos;
                endPos = (CsrUint16)i;
                if (endPos-beginPos > 1)
                {
                    /* Found something between the delimiters */
                    CsrUintFast16 x;
                    CsrBool foundNonWhiteSpace = FALSE;
                    for (x=beginPos+1; x<endPos; x++)
                    {
                        /* Check if the found stuff is different from only whitespaces */
                        if (whiteSpace(data->recvAtCmd[x]) == FALSE)
                        {
                            /* Found a non-white-space character */
                            foundNonWhiteSpace = TRUE ;
                        }
                    }
                    if (foundNonWhiteSpace == TRUE)
                    {
                        /* Create an AT-specification correct AT command to pass on in the system */
                        CsrUint16 startCopyPos = beginPos;

                        while ( ( *(data->recvAtCmd + startCopyPos) == '\r' ) ||
                             ( *(data->recvAtCmd + startCopyPos) == '\n' ) )
                        {
                            startCopyPos++;
                        };
                        handledSignal    = TRUE;
                        if (startCopyPos > beginPos)
                        {
                            handledAtCmdSize = endPos - beginPos + 3;
                        }
                        else
                        {/* We need one more byte of space as we shall copy one more byte into the heap area allocated */
                            handledAtCmdSize = endPos - beginPos + 4;
                        }
                        handledAtCmd     = (CsrUint8 *) CsrPmemAlloc(handledAtCmdSize+1);

                        handledAtCmd[0]  = '\r';
                        handledAtCmd[1]  = '\n';
                        SynMemCpyS((handledAtCmd + 2), handledAtCmdSize - 1, (data->recvAtCmd + startCopyPos), handledAtCmdSize - 4);
                        handledAtCmd[handledAtCmdSize - 2] = '\r';
                        handledAtCmd[handledAtCmdSize - 1] = '\n';
                        handledAtCmd[handledAtCmdSize] = '\0';
                    }
                }
            }

            if (handledSignal)
            {
                CsrBtHfHandleAtCommand(instData, theConnectionId, handledAtCmd, handledAtCmdSize);
                if (linkPtr->lastAtCmdSent == idleCmd)
                {
                    /* If we have received an "OK" response, lastAtCmdSent would be marked as "idleCmd". In that case,
                       we are free to send next command.
                       In case we have received multiple AT commands in single packet, linkPtr->lastAtCmdSend may get updated
                       as we process next incoming command within this loop, but we still want to remember that our
                       previous outgoing command was completed and we can send our next command after this loop. */
                       lastAtCmdWasResponded = TRUE;
                }
            }
            CsrPmemFree(handledAtCmd);
            handledAtCmd = NULL;
        }
        CsrPmemFree(data->recvAtCmd);
        data->recvAtCmd = NULL;

        if ((linkPtr->lastAtCmdSent == idleCmd) || (linkPtr->atSequenceState < copsQuery) || lastAtCmdWasResponded)
        {/* Now check if there are new messages to send in the queue */
            if (!HfProcessSavedAtMessages(instData, data, &restartAtTimer))
            {
                data->allowed2SendCmData = TRUE;
            }
        }
        else if (linkPtr->data->atResponseTimerId == 0)
        {
            restartAtTimer = TRUE;
        }

        if (((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == 0x00000000) && restartAtTimer)
        {
            linkPtr->data->atResponseTimerId = CsrSchedTimerSet((CsrTime)(instData->atRespWaitTime*1000000),
                                               CsrBtHfAtResponseTimeout,
                                               (CsrUint16)instData->index,
                                               (void*)instData);
         }
    }
}

CsrBool CsrBtHfHfIndResHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    if(linkPtr->disconnectReqReceived == FALSE && instData->deactivated == FALSE)
    {
         CsrUint8 *atString;
         atString = atTextString;

        if(*atString == '(')
        {
            CsrUint8 count;

            count = 1;
            while(*atString != '\r')
            {
                 if(*atString == ',')
                 {
                    count++;
                 }
                 atString++;
            }
            atString = atTextString;

            atString++;                               /* drop '(' */
            while(count > 0)
            {
                CsrUint32 indId;
                CsrUint8 *tempStr;

                if(getByteValue(&atString, &indId))
                {
                    CsrBtHfRemoteHfIndicator *rcvdAgHfInd;
                    rcvdAgHfInd = REMOTE_HF_INDICATOR_ADD_LAST((CsrCmnList_t *)&linkPtr->remoteHfIndicatorList);

                    rcvdAgHfInd->agHfIndicator = (CsrBtHfpHfIndicatorId) indId;
                    rcvdAgHfInd->indStatus = CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE;
                    rcvdAgHfInd->validVal = FALSE;
                }
                tempStr = (CsrUint8 *) CsrStrChr((char *) atString, ',');
                if(tempStr !=NULL)
                {
                    atString = tempStr;
                    atString++;                             /* drop ',' */
                }
                count--;
            }
        }
        else
        {
            CsrBtHfRemoteHfIndicator *rcvdAgHfInd;
            CsrUint32 indId;
            CsrUint32 status;
            CsrBtHfpHfIndicatorStatus enable;

            if(!getByteValue(&atString, &indId))
            {
                return TRUE;
            }

            atString = (CsrUint8 *) CsrStrChr((char *) atString, ',');
            atString++;                                     /* drop ',' */

            if(!getByteValue(&atString, &status))
            {
                return TRUE;
            }

            if(status)
            {
                enable = CSR_BT_HFP_HF_INDICATOR_STATE_ENABLE;
            }
            else
            {
                enable = CSR_BT_HFP_HF_INDICATOR_STATE_DISABLE;
            }

            rcvdAgHfInd = REMOTE_HF_INDICATOR_GET_FROM_IND_ID((CsrCmnList_t *)&linkPtr->remoteHfIndicatorList, (CsrUint8)indId);

            if((rcvdAgHfInd != NULL) && (enable != rcvdAgHfInd->indStatus))
            {
                rcvdAgHfInd->validVal = FALSE;
                rcvdAgHfInd->indStatus = enable;
                /* Send change in HF Indicator status to application when HF Indicator Activation/Deactivation
                   is not part of SLC establishment, ie when unsolicited '+BIND: ind,status' AT command is received */                
                if ((IsSlcEstablished(instData)) && (linkPtr->lastAtCmdSent != bindRead))
                {
                    CsrBtHfSendHfIndicatorStatusInd(instData, rcvdAgHfInd->agHfIndicator, rcvdAgHfInd->indStatus);
                }
            }
        }
    }
    return TRUE;
}

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
CsrBool CsrBtHfQcsHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
    CsrUint32 selectedCodecId = 0;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

    if (getByteValue(&atTextString, &selectedCodecId))
    {
        /* Proceed if QCE codec is supported and QHS is enabled for a given connection. */
        if (instData->hfQceCodecMask != CSR_BT_HF_QCE_UNSUPPORTED &&
            (!CmIsSWBDisabled(&linkPtr->currentDeviceAddress)) &&
            (instData->hfQceCodecMask & (1 << selectedCodecId)))
        {
            /* Store the selected codec ID for this connection. */
            linkPtr->hfQceCodecId = (CsrUint16) selectedCodecId;
        }
        else
        {
            /* if selected codec id is not supported or we don't support QCE at all, set codec id to unsupported */
            linkPtr->hfQceCodecId = CSR_BT_HF_QCE_UNSUPPORTED;
        }
    }

    /* Send AT+%QCS=codecId command as a response */
    sendQcs(instData);

    return TRUE;
}

#if 0 /* hfAgQceCodecMask is not being used currently */
static CsrUint8 csrHfGetAgMask(CsrUint8 *str)
{
    CsrUint8 value = (CsrUint8) CsrStrToInt((const CsrCharString *) str);
    return (value < 16) ? (1 << value) : 0;
}
#endif

CsrBool CsrBtHfQacHandler(HfMainInstanceData_t *instData, CsrUint8 *atTextString)
{
#if 0 /* hfAgQceCodecMask is not being used currently */
    CsrUint16 agSupportMask = 0;
    CsrUint8 *comma = (CsrUint8 *) ",";
    CsrUint8 len = CsrStrLen((char *) atTextString);
    CsrUint8 tempStr[2];
    CsrUint8 tempIndex = 0, strIndex = 0;
    HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

    while (strIndex < (len - 1))
    {
        if (atTextString[strIndex] == comma[0])
        {
            agSupportMask |= csrHfGetAgMask(tempStr);
            tempIndex = 0;
            CsrMemSet(tempStr, 0, sizeof(tempStr));
        }
        else
        {
            tempStr[tempIndex++] = atTextString[strIndex];
        }
        strIndex++;
    }

    /* this is for the last value */
    agSupportMask |= csrHfGetAgMask(tempStr);

    /* Store the final mask into the link information */
    linkPtr->hfAgQceCodecMask = agSupportMask;
#else
    CSR_UNUSED(instData);
    CSR_UNUSED(atTextString);
#endif

    return TRUE;
}
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

CsrBool HfProcessSavedAtMessages(HfMainInstanceData_t *instData, HfHsData_t *data, CsrBool *startAtResponseTimer)
{
    CsrBool    proccessedSavedAtCmd = FALSE;
    CsrUint16  eventClass;
    void      *msg =NULL;

    *startAtResponseTimer = FALSE;

    if (CsrMessageQueuePop(&data->cmDataReqQueue, &eventClass, &msg))
    {
        proccessedSavedAtCmd = TRUE;
        
        if (eventClass == CSR_BT_HF_PRIM)
        {/* This is a request to send an AT command */
            /* Save recvMsgP pointer (to be freed by caller)and event class*/
            void *saveRecvMsgP = instData->recvMsgP;
            CsrUint16 saveEventClass = instData->eventClass;

            data->allowed2SendCmData = TRUE;
            instData->eventClass = eventClass;
            instData->recvMsgP = msg;
            CsrBtHfCommonAtCmdPrimReqHandler(instData,(CsrBtHfPrim *)msg);
            SynergyMessageFree(eventClass, instData->recvMsgP);

            /* Restore saved recvMsgP and event class */
            instData->recvMsgP = saveRecvMsgP;
            instData->eventClass = saveEventClass;
        }
        else
        {/* CSR_BT_CM_PRIM */
            HfInstanceData_t *linkPtr;
            CsrBtCmDataReq *cmPrim = msg;

            linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
            linkPtr->lastAtCmdSent = FindCurrentCmdFromPayload(cmPrim->payload);
#ifdef CSR_STREAMS_ENABLE
            CsrStreamsDataSend(CM_GET_UINT16ID_FROM_BTCONN_ID(linkPtr->hfConnId),
                               RFCOMM_ID,
                               cmPrim->payloadLength,
                               cmPrim->payload);
            SynergyMessageFree(eventClass, cmPrim);
#else
            CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, eventClass, cmPrim);
#endif
            *startAtResponseTimer = TRUE;
        }
    }

    return (proccessedSavedAtCmd);
}

