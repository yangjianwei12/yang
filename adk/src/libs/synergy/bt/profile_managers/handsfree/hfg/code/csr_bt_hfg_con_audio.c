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
#include "csr_msg_transport.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_main.h"
#include "csr_bt_hfg_proto.h"
#include "csr_bt_tasks.h"

static const HfgAudioParams_t scoNegotiationOrder[] =
{
    {
        CSR_BT_ESCO_DEFAULT_2P0_S4_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S4_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S4_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S4_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S4_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S4_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_2P0_S4,
    },

    {
        CSR_BT_ESCO_DEFAULT_2P0_S3_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S3_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S3_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S3_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S3_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S3_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_2P0_S3,
    },

    {
        CSR_BT_ESCO_DEFAULT_2P0_S2_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S2_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_2P0_S2_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_2P0_S2_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_2P0_S2_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_2P0_S2_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_2P0_S2,
    },

    {
        CSR_BT_ESCO_DEFAULT_1P2_S1_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_1P2_S1_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_1P2_S1_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_1P2_S1_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_1P2_S1_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_1P2_S1_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_1P2_S1,
    },

    {
        CSR_BT_SCO_DEFAULT_1P1_TX_BANDWIDTH,
        CSR_BT_SCO_DEFAULT_1P1_RX_BANDWIDTH,
        CSR_BT_SCO_DEFAULT_1P1_MAX_LATENCY,
        CSR_BT_SCO_DEFAULT_1P1_VOICE_SETTINGS,
        CSR_BT_SCO_DEFAULT_1P1_AUDIO_QUALITY,
        CSR_BT_SCO_DEFAULT_1P1_RE_TX_EFFORT,
        CSR_BT_SCO_DEFAULT_1P1,
    },
};


static const HfgAudioParams_t wbsScoNegotiationOrder[] =
{
    {
        CSR_BT_ESCO_DEFAULT_T2_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_T2_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_T2_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_T2_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_T2_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_T2_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_T2,
    },

    {
        CSR_BT_ESCO_DEFAULT_T1_TX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_T1_RX_BANDWIDTH,
        CSR_BT_ESCO_DEFAULT_T1_MAX_LATENCY,
        CSR_BT_ESCO_DEFAULT_T1_VOICE_SETTINGS,
        CSR_BT_ESCO_DEFAULT_T1_AUDIO_QUALITY,
        CSR_BT_ESCO_DEFAULT_T1_RE_TX_EFFORT,
        CSR_BT_ESCO_DEFAULT_T1,
    },
};

static void hgfCopyInstScoParms(HfgAudioParams_t *aud, HfgInstance_t *inst)
{
    aud->audioQuality     = inst->scoParams.audioQuality;
    aud->txBandwidth     = inst->scoParams.txBandwidth;
    aud->rxBandwidth     = inst->scoParams.rxBandwidth;
    aud->maxLatency     = inst->scoParams.maxLatency;
    aud->voiceSettings     = inst->scoParams.voiceSettings;
    aud->reTxEffort     = inst->scoParams.reTxEffort;
}

static void hgfCopyScoParms(HfgAudioParams_t *dst, const HfgAudioParams_t *src)
{
    dst->audioQuality     = src->audioQuality;
    dst->txBandwidth     = src->txBandwidth;
    dst->rxBandwidth     = src->rxBandwidth;
    dst->maxLatency     = src->maxLatency;
    dst->voiceSettings     = src->voiceSettings;
    dst->reTxEffort     = src->reTxEffort;
}

static bool hfgValidateQcCodec(uint16 codec_id)
{
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    if((CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3  == codec_id) ||
       (CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3_QHS3 == codec_id) ||
       (CSR_BT_HF_QCE_CODEC_TYPE_128_QHS3 == codec_id) ||
       (CSR_BT_HF_QCE_CODEC_TYPE_64_QHS3 == codec_id))
    {
        return TRUE;
    }
#endif
    return FALSE;
}

/* Local: Downstream message for connect SCO */
void csrBtHfgSendCmScoConnectReq(HfgInstance_t *inst, CsrUint8 setting)
{
    HfgAudioParams_t aud;
    CsrBtCmScoCommonParms *parms;
    CsrUint16 parmsOffset;
    CsrUint16 parmsLen;
    CsrSize idx;

    CsrUint8  pcmSlot = inst->scoPcmSlot;
    CSR_UNUSED(setting);  

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "csrBtHfgSendCmScoConnectReq")); 

    if ((CsrBtHfgGetMainInstance(inst)->hfgConfig & CSR_BT_HFG_CNF_DISABLE_ESCO_TO_OLD_DEVICES) &&
        (inst->remoteVersion < CSR_BT_FIRST_HFP_ESCO))
    {/* Try only SCO */
        idx = CSR_ARRAY_SIZE(scoNegotiationOrder) - 1; /* SCO is the last entry in the table */
        parmsLen = 1;
        hgfCopyScoParms(&aud, &scoNegotiationOrder[idx]);
        CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmsLen);
        CsrBtCmCommonScoConnectBuild(parms,
                                     &parmsOffset,
                                     aud.audioQuality,
                                     aud.txBandwidth,
                                     aud.rxBandwidth,
                                     aud.maxLatency,
                                     aud.voiceSettings,
                                     aud.reTxEffort);
    }
    else
    {
        if (inst->lastCodecUsed == CSR_BT_WBS_MSBC_CODEC
            || inst->lastCodecUsed == CSR_BT_WBS_LC3SWB_CODEC)
        {
            /* WBS or SWB audio settings */
            parmsLen = 2;
            CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmsLen);
            for (idx = 0; idx < CSR_ARRAY_SIZE(wbsScoNegotiationOrder); ++idx)
            {
                hgfCopyScoParms(&aud, &wbsScoNegotiationOrder[idx]);
                CsrBtCmCommonScoConnectBuild(parms,
                                             &parmsOffset,
                                             aud.audioQuality,
                                             aud.txBandwidth,
                                             aud.rxBandwidth,
                                             aud.maxLatency,
                                             aud.voiceSettings,
                                             aud.reTxEffort);
            }
        }
        else
        {
            hgfCopyInstScoParms(&aud, inst);
            if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS) &&
                (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_ESCO_S4_T2_SETTINGS))
            {
                parmsLen = CSR_ARRAY_SIZE(scoNegotiationOrder) + 1;
            }
            else
            {
                parmsLen = CSR_ARRAY_SIZE(scoNegotiationOrder);
            }

            CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "audioQual = %d, txBand = %d, rxBand = %d, maxLat = %d, voiceSettings = %d, reTx = %d", aud.audioQuality, aud.txBandwidth, aud.rxBandwidth,aud.maxLatency,aud.voiceSettings,aud.reTxEffort));
            
            CsrBtCmCommonScoConnectPrepare(&parms, &parmsOffset, parmsLen);
            CsrBtCmCommonScoConnectBuild(parms,
                                         &parmsOffset,
                                         aud.audioQuality,
                                         aud.txBandwidth,
                                         aud.rxBandwidth,
                                         aud.maxLatency,
                                         aud.voiceSettings,
                                         aud.reTxEffort);


                                         

            if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS) &&
                (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_ESCO_S4_T2_SETTINGS))
            {
                idx = 0;
            }
            else
            {
                idx = 1;
            }
            while (idx < CSR_ARRAY_SIZE(scoNegotiationOrder))
            {
                hgfCopyScoParms(&aud, &scoNegotiationOrder[idx]);
                CsrBtCmCommonScoConnectBuild(parms,
                                             &parmsOffset,
                                             aud.audioQuality,
                                             aud.txBandwidth,
                                             aud.rxBandwidth,
                                             aud.maxLatency,
                                             aud.voiceSettings,
                                             aud.reTxEffort);
                idx++;
            }
        }
    }
    CsrBtCmScoConnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                             pcmSlot,
                             inst->scoPcmRealloc,
                             parms,
                             parmsLen,
                             inst->hfgConnId);
}

/* Downstream: Audio request */
void CsrBtHfgConnectedHfgAudioDisconnectReqHandler(HfgInstance_t *inst)
{
    inst->pendingScoDisconnect = TRUE;
    if(!inst->pendingSco)
    {
        inst->pendingSco = TRUE;
        /* We're requested to disconnect SCO */
        if(inst->scoHandle != CSR_SCHED_QID_INVALID)
        {
            /* Send disconnect */
            CsrBtCmScoDisconnectReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);
        }
        else
        {
            /* SCO already disconnected */
            CsrBtHfgSendHfgAudioDisconnectCfm(inst, inst->scoHandle, CSR_BT_RESULT_CODE_HFG_SUCCESS, CSR_BT_SUPPLIER_HFG);
            inst->pendingSco = FALSE;
            inst->pendingScoDisconnect = FALSE;
        }
    }/* pendingSco */
}

/* Downstream: Audio request */
void CsrBtHfgConnectedHfgAudioReqHandler(HfgInstance_t *inst)
{
    CsrBtHfgAudioConnectReq *prim;
    prim = (CsrBtHfgAudioConnectReq*)inst->msg;

    if (prim->qceCodecId != CSR_BT_HFG_QCE_UNSUPPORTED &&
        hfgValidateQcCodec(prim->qceCodecId))
    {
        inst->selectedQceCodecId = prim->qceCodecId;
    }

    CSR_LOG_TEXT_INFO((CsrBtHfgLto, 0, "CsrBtHfgConnectedHfgAudioReqHandler")); 

    inst->pendingScoDisconnect = FALSE;

    if ((!inst->pendingSco) && (!inst->pendingCodecNegotiation))
    {
        /* We're requested to connect SCO */
        CsrBool negotiateWbsCodec = FALSE;

        inst->pendingSco = TRUE;
        inst->pendingScoConfirm = TRUE;

        if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION) &&
            (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION))
        {/* WBS */
            if (CSR_BT_WBS_INVALID_CODEC == inst->lastCodecUsed)
            {/* Negotiation needed! */
                negotiateWbsCodec = TRUE;
            }
        }

        /* start codec negotiation only if there is something to negotiate 
         * also if the peer side is a Q device and supports any of the Q codecs 
         * then we skip negotiation and go ahead with the connection */
        if (negotiateWbsCodec && 
            (inst->selectedQceCodecId == CSR_BT_HFG_QCE_UNSUPPORTED))
        {
            CsrBtHfgSendCodecNegMsg(inst);
            if (inst->scoHandle == CSR_SCHED_QID_INVALID)
            { /* Store the neccessary data */
                inst->scoPcmSlot = prim->pcmSlot;
                inst->scoPcmRealloc = prim->pcmRealloc;
            }
        }
        else
        { /* if no audio negotiation needed, just do as always: try to connect audio.*/
            if(inst->scoHandle == CSR_SCHED_QID_INVALID)
            {
                inst->scoPcmSlot = prim->pcmSlot;
                inst->scoPcmRealloc = prim->pcmRealloc;

                if(inst->linkType == CSR_BT_HFG_CONNECTION_HFG)
                {
                    /* In HFG connections, we always have an incoming
                     * SCO connection, so take that one down */
                    CsrBtCmScoCancelReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);
                }


                /* Now, start connecting SCO by trying highest quality
                 * first */
                csrBtHfgSendCmScoConnectReq(inst, CSR_BT_ESCO_DEFAULT_CONNECT);
            }
            else
            {
                /* SCO connection already up */
                CsrBtHfgSendHfgExtendedAudioInd(inst,
                                           inst->scoHandle,
                                           inst->scoPcmSlot,
                                           0,0,0,0,0,0,
                                           CSR_BT_RESULT_CODE_HFG_SUCCESS,
                                           CSR_BT_SUPPLIER_HFG);
                inst->pendingSco = FALSE;
            }
        }
    }/* pendingSco or codec negotiation */

}

#ifdef CSR_BT_INSTALL_HFG_CONFIG_AUDIO
/* Downstream: Setup user-defined (e)SCO settings */
void CsrBtHfgXHfgConfigAudioReqHandler(HfgInstance_t *inst)
{
    CsrBtHfgConfigAudioReq *prim;
    prim = (CsrBtHfgConfigAudioReq*)inst->msg;

    if((prim->audioSetting != NULL) &&
       (prim->audioSettingLen > 0))
    {
        /* Decode data and store it in our user-defined settings */
        switch(prim->audioType)
        {
            case CSR_BT_HFG_AUDIO_RETRANSMISSION:
                {
                    CsrBtHfgAudioRetransmission *data;
                    data = (CsrBtHfgAudioRetransmission*)prim->audioSetting;
                    inst->scoParams.reTxEffort = *data;
                    break;
                }

            case CSR_BT_HFG_AUDIO_MAX_LATENCY:
                {
                    CsrBtHfgAudioMaxLatency *data;
                    data = (CsrBtHfgAudioMaxLatency*)prim->audioSetting;
                    inst->scoParams.maxLatency = *data;
                    break;
                }

            case CSR_BT_HFG_AUDIO_SUP_PACKETS:
                {
                    CsrBtHfgAudioSupPackets *data;
                    data = (CsrBtHfgAudioSupPackets *)prim->audioSetting;
                    inst->scoParams.audioQuality = *data;
                    break;
                }

            case CSR_BT_HFG_AUDIO_TX_BANDWIDTH:
                {
                    CsrBtHfgAudioTxBandwidth *data;
                    data = (CsrBtHfgAudioTxBandwidth*)prim->audioSetting;
                    inst->scoParams.txBandwidth = *data;
                    break;
                }

            case CSR_BT_HFG_AUDIO_RX_BANDWIDTH:
                {
                    CsrBtHfgAudioRxBandwidth *data;
                    data = (CsrBtHfgAudioRxBandwidth *)prim->audioSetting;
                    inst->scoParams.rxBandwidth = *data;
                    break;
                }

            case CSR_BT_HFG_AUDIO_VOICE_SETTINGS:
                {
                    CsrBtHfgAudioVoiceSettings *data;
                    data = (CsrBtHfgAudioVoiceSettings *)prim->audioSetting;
                    inst->scoParams.voiceSettings = *data;
                    break;
                }
        }
    }
}
#endif /* CSR_BT_INSTALL_HFG_CONFIG_AUDIO */

/* SCO connect confirm */
void CsrBtHfgConnectedCmScoConnectCfmHandler(HfgInstance_t *inst)
{
    CsrBtCmScoConnectCfm *prim;
    HfgMainInstance_t *mainInst;

    prim = (CsrBtCmScoConnectCfm *) inst->msg;
    mainInst = CsrBtHfgGetMainInstance(inst);

    /* If sco already in use which means a race condition may have
     * occurred, so ignore the confirm */
    inst->pendingCodecNegotiation = FALSE;
    if(inst->scoHandle == CSR_SCHED_QID_INVALID)
    {
        if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            inst->scoHandle = prim->eScoHandle;
            inst->scoPcmSlot = prim->pcmSlot;
            /* Audio is up and running */
            CsrBtHfgSendHfgExtendedAudioInd(inst,
                                       inst->scoHandle,
                                       inst->scoPcmSlot,
                                       prim->linkType,
                                       prim->weSco,
                                       prim->rxPacketLength,
                                       prim->txPacketLength,
                                       prim->airMode,
                                       prim->txInterval,
                                       CSR_BT_RESULT_CODE_HFG_SUCCESS,
                                       CSR_BT_SUPPLIER_HFG);

            /* Should we send audio status? */
            if(mainInst->hfgConfig & CSR_BT_HFG_CNF_AUDIO_STATUS)
            {
                CsrBtHfgAudioScoStatus *set;
                set = (CsrBtHfgAudioScoStatus *)CsrPmemAlloc(sizeof(CsrBtHfgAudioScoStatus));
                set->linkType = prim->linkType;
                set->txInterval = prim->txInterval;
                set->weSco = prim->weSco;
                set->rxPacketLength = prim->rxPacketLength;
                set->txPacketLength = prim->txPacketLength;
                set->airMode = prim->airMode;
                set->resultCode = prim->resultCode;
                set->resultSupplier = prim->resultSupplier;
                CsrBtHfgSendScoHfgStatusAudioInd(inst, set);
            }

            /* We knew the SCO was coming... */
            if(inst->pendingSco)
            {
                /* But we also requested it to be disconnected */
                if(inst->pendingScoDisconnect)
                {
                    CsrBtCmScoDisconnectReqSend(CSR_BT_HFG_IFACEQUEUE, inst->hfgConnId);
                }
                else
                {
                    inst->pendingSco = FALSE;
                }
            }
        }
        else
        { /* SCO setup failed, so begin possible retry */
            if ((inst->remSupFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION) &&
                (CsrBtHfgGetMainInstance(inst)->locSupFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION))
            {
            /* WBS or SWB */
                if (inst->lastCodecUsed == CSR_BT_WBS_MSBC_CODEC
                    || inst->lastCodecUsed == CSR_BT_WBS_LC3SWB_CODEC)
                {/* Fall back to another codec and try again */
                    CsrBtHfgSendCodecNegMsg(inst);
                    return;
                }
            }

            if((mainInst->hfgConfig & CSR_BT_HFG_CNF_AUDIO_STATUS))
            {
                /* Notify user if all SCO attempts failed */
                CsrBtHfgAudioScoStatus *set;

                set = (CsrBtHfgAudioScoStatus*)CsrPmemAlloc(sizeof(CsrBtHfgAudioScoStatus));
                set->linkType = prim->linkType;
                set->txInterval = prim->txInterval;
                set->weSco = prim->weSco;
                set->rxPacketLength = prim->rxPacketLength;
                set->txPacketLength = prim->txPacketLength;
                set->airMode = prim->airMode;
                set->resultCode = prim->resultCode;
                set->resultSupplier = prim->resultSupplier;
                CsrBtHfgSendScoHfgStatusAudioInd(inst, set);
            }

            /* SCO connection failed */
            CsrBtHfgSendHfgExtendedAudioInd(inst,
                                       inst->scoHandle,
                                       inst->scoPcmSlot,
                                       prim->linkType,
                                       prim->weSco,
                                       prim->rxPacketLength,
                                       prim->txPacketLength,
                                       prim->airMode,
                                       prim->txInterval,
                                       prim->resultCode,
                                       prim->resultSupplier);
            inst->pendingSco = FALSE;

            /* As connection attempts are failed, let HFG allow further incoming SCO connection */
            CsrBtCmScoAcceptConnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                                      inst->hfgConnId,
                                      CSR_BT_COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY,
                                      CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH,
                                      CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH,
                                      CSR_BT_SCO_DEFAULT_ACCEPT_MAX_LATENCY,
                                      CSR_BT_SCO_DEFAULT_ACCEPT_VOICE_SETTINGS,
                                      CSR_BT_SCO_DEFAULT_ACCEPT_RE_TX_EFFORT);
        }
    }
}

/* SCO disconnect indication */
void CsrBtHfgConnectedCmScoDisconnectIndHandler(HfgInstance_t *inst)
{
    CsrBtCmScoDisconnectInd *prim;

    prim = (CsrBtCmScoDisconnectInd*)inst->msg;

    /* If sco already disconnected a race condition may have occurred
     * so ignore the message */
    if(inst->scoHandle != CSR_SCHED_QID_INVALID)
    {
        if(prim->status)
        {
            inst->scoHandle = CSR_SCHED_QID_INVALID;

            if ((inst->pendingScoDisconnect) && (inst->scoWantedState))
            {
                CsrBtHfgSendHfgAudioDisconnectCfm(inst, prim->eScoHandle, CSR_BT_RESULT_CODE_HFG_SUCCESS, CSR_BT_SUPPLIER_HFG);
            }
            else
            {
                CsrBtHfgSendHfgAudioDisconnectInd(inst, prim->eScoHandle, prim->reasonCode, prim->reasonSupplier);
            }
            inst->scoWantedState = TRUE;
            if(inst->pendingSco)
            {
                if(!(inst->pendingScoDisconnect))
                {
                    /* Restart full SCO negotiation */
                    csrBtHfgSendCmScoConnectReq(inst, CSR_BT_ESCO_DEFAULT_CONNECT);
                }
                else
                {
                    inst->pendingSco = FALSE;
                    inst->pendingScoDisconnect = FALSE;
                    if(inst->linkType == CSR_BT_HFG_CONNECTION_HFG)
                    {
                        CsrUint16 voiceSettings = CSR_BT_SCO_DEFAULT_ACCEPT_VOICE_SETTINGS;

                        if (inst->lastCodecUsed == CSR_BT_WBS_MSBC_CODEC 
                            || inst->lastCodecUsed == CSR_BT_WBS_LC3SWB_CODEC)
                        {
                            voiceSettings |= CSR_BT_AIRCODING_TRANSPARENT_DATA;
                            CsrBtCmWriteVoiceSettingsReqSend(CSR_BT_HFG_IFACEQUEUE, voiceSettings);
                        }
                        CsrBtCmScoAcceptConnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                                                  inst->hfgConnId,
                                                  CSR_BT_COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY,
                                                  CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH,
                                                  CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH,
                                                  CSR_BT_SCO_DEFAULT_ACCEPT_MAX_LATENCY,
                                                  voiceSettings,
                                                  CSR_BT_SCO_DEFAULT_ACCEPT_RE_TX_EFFORT);
                    }
                }
            }
            else
            {
                inst->pendingScoDisconnect = FALSE;
                if(inst->linkType == CSR_BT_HFG_CONNECTION_HFG)
                {
                    /* HFG must accept incoming SCO */
                    CsrUint16 voiceSettings = CSR_BT_SCO_DEFAULT_ACCEPT_VOICE_SETTINGS;

                    if (inst->lastCodecUsed == CSR_BT_WBS_MSBC_CODEC
                        || inst->lastCodecUsed == CSR_BT_WBS_LC3SWB_CODEC)
                    {
                        voiceSettings |= CSR_BT_AIRCODING_TRANSPARENT_DATA;
                        CsrBtCmWriteVoiceSettingsReqSend(CSR_BT_HFG_IFACEQUEUE, voiceSettings);
                    }
                    CsrBtCmScoAcceptConnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                                              inst->hfgConnId,
                                              CSR_BT_COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY,
                                              CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH,
                                              CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH,
                                              CSR_BT_SCO_DEFAULT_ACCEPT_MAX_LATENCY,
                                              voiceSettings,
                                              CSR_BT_SCO_DEFAULT_ACCEPT_RE_TX_EFFORT);

                }
            }
        }
        else
        {
            /* SCO disconnect failed, so we have to retry... */
            if((inst->pendingSco) &&
               !(inst->pendingScoDisconnect))
            {
                /* SCO still on, disconnect not requested */
                CsrBtHfgSendHfgExtendedAudioInd(inst,
                                           inst->scoHandle,
                                           inst->scoPcmSlot,
                                           0,0,0,0,0,0,
                                           CSR_BT_RESULT_CODE_HFG_SUCCESS,
                                           CSR_BT_SUPPLIER_HFG);
            }
            else
            {

                CsrBtCmScoDisconnectReqSend(CSR_BT_HFG_IFACEQUEUE,
                                       inst->hfgConnId);
            }
        }
    }
}

/* SCO accept connection confirm */
void CsrBtHfgConnectedCmScoAcceptConnectCfmHandler(HfgInstance_t *inst)
{
    CsrBtCmScoAcceptConnectCfm *prim;
    HfgMainInstance_t *mainInst;

    prim = (CsrBtCmScoAcceptConnectCfm*)inst->msg;
    mainInst = CsrBtHfgGetMainInstance(inst);

    /* If SCO already connected ignore the message */
    if(inst->scoHandle == CSR_SCHED_QID_INVALID)
    {
        if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            CsrBool pendingScoTemp;

            inst->scoHandle = prim->eScoHandle;
            inst->scoPcmSlot = prim->pcmSlot;
            /* Make sure to handle crossing SCO requests this is surely an incoming connection so it should give a CSR_BT_HFG_AUDIO_CONNECT_IND */
            pendingScoTemp = inst->pendingSco;
            inst->pendingSco = FALSE;

            CsrBtHfgSendHfgExtendedAudioInd(inst,
                                       inst->scoHandle,
                                       inst->scoPcmSlot,
                                       prim->linkType,
                                       prim->weSco,
                                       prim->rxPacketLength,
                                       prim->txPacketLength,
                                       prim->airMode,
                                       prim->txInterval,
                                       CSR_BT_RESULT_CODE_HFG_SUCCESS,
                                       CSR_BT_SUPPLIER_HFG);

            inst->pendingSco = pendingScoTemp;
        }
        else
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
    }
    /* Notify user of SCO */
    if((mainInst->hfgConfig & CSR_BT_HFG_CNF_AUDIO_STATUS)  &&
        (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
        (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
    {
        CsrBtHfgAudioScoStatus *set;
        set = (CsrBtHfgAudioScoStatus*)CsrPmemAlloc(sizeof(CsrBtHfgAudioScoStatus));
        set->linkType = prim->linkType;
        set->txInterval = prim->txInterval;
        set->weSco = prim->weSco;
        set->rxPacketLength = prim->rxPacketLength;
        set->txPacketLength = prim->txPacketLength;
        set->airMode = prim->airMode;
        set->resultCode = prim->resultCode;
        set->resultSupplier = prim->resultSupplier;

        CsrBtHfgSendScoHfgStatusAudioInd(inst, set);
    }
}

/* Downstream: SCO PCM mapping */
void CsrBtHfgConnectedHfgMapScoPcmResHandler(HfgInstance_t *inst)
{
    CsrBtHfgAudioAcceptConnectRes* prim;
    CsrBtCmScoCommonParms *scoParms = NULL;
    CsrUint8 pcmSlot;

    prim = (CsrBtHfgAudioAcceptConnectRes*)inst->msg;

    if(prim->acceptParameters)
    {
        scoParms = CsrPmemAlloc(sizeof(CsrBtCmScoCommonParms));
        scoParms->audioQuality = prim->acceptParameters->packetTypes;
        scoParms->txBandwidth = prim->acceptParameters->txBandwidth;
        scoParms->rxBandwidth = prim->acceptParameters->rxBandwidth;
        scoParms->maxLatency = prim->acceptParameters->maxLatency;
        scoParms->voiceSettings = prim->acceptParameters->contentFormat;
        scoParms->reTxEffort = prim->acceptParameters->reTxEffort;
        CsrPmemFree(prim->acceptParameters);
    }
    pcmSlot = prim->pcmSlot;

    CsrBtCmMapScoPcmResSend(inst->hfgConnId,
                       prim->acceptResponse,
                       scoParms,
                       pcmSlot,
                       prim->pcmReassign);
}

/* Upstream: SCO PCM mapping */
void CsrBtHfgXCmMapScoPcmIndHandler(HfgInstance_t *inst)
{
    /* Simply forward CM indication as HFG */
    CsrBtHfgSendHfgMapScoPcmInd(inst);
}

