/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP BapClientAse interface implementation.
 */

/**
 * \addtogroup BAP
 * @{
 */

#include <stdio.h>
#include "bap_client_list_container_cast.h"
#include "bap_client_list_util_private.h"
#include "bap_profile.h"
#include "bap_utils.h"
#include "bap_pac_record.h"
#include "bap_client_connection.h"
#include "bap_client_ase.h"
#include "bap_client_debug.h"
#include "bap_stream_group.h"
#include "bap_ascs.h"
#include <stdio.h>

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*! \brief RTTI information for the BapClientAse structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapClientAse,  'A','s','E','i')

/*static void bap_client_ase_v_handle_ascs_msg(BapAse* const ase, uint8 opcode, AscsMsg* const msg);*/
static void bapClientAseVDelete(BapAse * const ase);

const BapAseVTable clientAseVtable =
{
    bapAseVHandleAscsMsg,
    bapClientAseVDelete
};

BapClientAse *bapClientAseNew(BapAseCodecConfiguration * const aseCodecConfiguration,
                              BapClientPacRecord * const pacRecord,
                              struct BapConnection * const connection,
                              struct BapCis* const cis,
                              struct BapStreamGroup * const streamGroup)
{
    BapClientAse *clientAse = (BapClientAse *)CsrPmemAlloc(sizeof(BapClientAse));

    if (clientAse != NULL)
    {
        /* Parameters are always valid here (i.e. the function bapPacRecordCheckCodecConfiguration
         * is used by the caller.
         */
        bapAseInitialise(&clientAse->ase,
                         aseCodecConfiguration->aseId,
                         BAP_CONTEXT_TYPE_UNKNOWN, /* context_type */
                         aseCodecConfiguration->codecId,
                         cis,
                         aseCodecConfiguration->serverDirection,
                         connection,
                         pacRecord,
                         &clientAseVtable);

        clientAse->state = CLIENT_ASE_STATE_IDLE;

        /* Keep a record of what stream group this ase is in. */
        clientAse->ase.streamGroup = streamGroup;

        type_name_initialise_rtti_member_variable(BapClientAse, clientAse);
    }

    return clientAse;
}

static void bapClientAseVDelete(BapAse * const ase)
{
    BapClientAse* clientAse = CONTAINER_CAST(ase, BapClientAse, ase);
    /*
     * Execute any 'common' code (in the base class)
     */
    bapAseDestroy(ase);
    /*
     * Free the correct pointer, i.e. the one pointing to the derived class
     */
    CsrPmemFree(clientAse);
}

BapResult bapClientAseCodecConfigure(BapClientAse * const clientAse,
                                     BapInternalUnicastClientCodecConfigureReq * const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_CONFIGURE,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseQosConfigure(BapClientAse * const clientAse,
                                   BapInternalUnicastClientQosConfigureReq * const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_QOS_CONFIGURE,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;

}

BapResult bapClientAseEnable(BapClientAse * const clientAse,
                             BapInternalUnicastClientEnableReq *const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_ENABLE,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseDisable(BapClientAse * const clientAse,
                              BapInternalUnicastClientDisableReq *const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_DISABLE,
                              (void *)clientAse,
                              (void*)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseRelease(BapClientAse * const clientAse,
                              BapInternalUnicastClientReleaseReq *const primitive)
{
    fsm_result_t fsmResult;
    
    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_RELEASE,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseUpdateMetadata(BapClientAse * const clientAse,
                                     BapInternalUnicastClientUpdateMetadataReq *const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_UPDATE_METADATA,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseCisConnect(BapClientAse * const clientAse,
                                 BapInternalUnicastClientCisConnectReq *const primitive)
{
    fsm_result_t fsmResult;

    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              CLIENT_ASE_EVENT_CIS_CONNECT,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}

BapResult bapClientAseReceiverReady(BapClientAse * const clientAse,
                                    BapInternalUnicastClientReceiverReadyReq *const primitive)
{
    fsm_result_t fsmResult;
    fsm_event_t event = CLIENT_ASE_EVENT_STOP_READY_SEND;

    if( primitive->readyType == BAP_RECEIVER_START_READY)
    {
        event = CLIENT_ASE_EVENT_START_READY;
    }
    
    fsmResult = fsm_16bit_run(&bapClientAseFsm,
                              &clientAse->state,
                              event,
                              (void *)clientAse,
                              (void *)primitive);

    return (fsmResult == FSM_RESULT_FAIL) ?
            BAP_RESULT_INVALID_STATE :
            BAP_RESULT_SUCCESS;
}


static bool bapClientAllNotificationReceived(BapStreamGroup * const streamGroup)
{
    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        if (bapConfirmationCounterAggregateConfirmationResultIsSuccess(&streamGroup->cfmCounter))
        {
            streamGroup->cfmCounter.nExpectedCfm = 0;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    return FALSE;
}

fsm_event_t bapClientAseAfSendConfigCodecOp(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientCodecConfigureReq * prim = (BapInternalUnicastClientCodecConfigureReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseCodecConfigurations,
                                prim->numAseCodecConfigurations);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCodecConfigureCfmReceived(void *arg1, void *arg2)
{
    BapResult errorCode = BAP_RESULT_SUCCESS;
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AscsConfigureCodecNotify *cfm = (AscsConfigureCodecNotify *)streamAseArg->cfm;
    uint32  pdMin;
    uint32  pdMax;
    uint8 *ptr = NULL;
    BapAseInfo aseInfo;
    uint8  peerFraming = 0;
    uint8  peerPhy = 0;
    uint8  peerRetransmissionEffort;
    uint16 peerTransportLatency;
    BapCodecConfiguration  codecConfiguration;

    /* decode and verify server supported QOS parameters */

    peerFraming = cfm->preferredFraming;
    peerPhy = cfm->preferredPhy;
    peerRetransmissionEffort = cfm->retransmissionNumber;
    peerTransportLatency = ((uint16)cfm->transportLatency[0] | (uint16)(cfm->transportLatency[1] << 8));

    ptr = (uint8 *)&cfm->presentationDelayMin[0];
    pdMin = (uint32)readUint24(&ptr);
    ptr = (uint8 *)&cfm->presentationDelayMax[0];
    pdMax = (uint32)readUint24(&ptr);

    streamAseArg->clientAse->ase.presentationDelayMin = pdMin;
    streamAseArg->clientAse->ase.presentationDelayMax = pdMax;

    memset(&codecConfiguration, 0, sizeof(BapCodecConfiguration));

    /* Get Codec specific configuration parameter */
    if(cfm->codecSpecificConfigLength)
    {
        QblLtv ltv;
        uint8* ltvFormatData;

        /* Set default value if not present */
        codecConfiguration.lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;

        /* Decode LTV format data from the Codec specific parameter */
        for (ltvFormatData = &cfm->codecSpecificConfig[0];
             ltvFormatData < &cfm->codecSpecificConfig[0] + cfm->codecSpecificConfigLength;
             ltvFormatData = qblLtvGetNextLtvStart(&ltv))
        {
            qblLtvInitialise(&ltv, ltvFormatData);

            if ( ! qblLtvDecodeCodecSpecificCapabilities(&ltv, &codecConfiguration))
            {
                BAP_CLIENT_DEBUG(" BAP:: Wrong Codec Config parameter for Type %d\n", ltv.type);
                break; /* Exit the loop */
            }
        }
    }
    
    bapAseSetErrorCode(&streamAseArg->clientAse->ase, errorCode);

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseCodecConfigureInd(streamGroup->rspPhandle,
                                     errorCode,
                                     streamGroup->id,
                                     streamAseArg->clientAse->ase.presentationDelayMin,
                                     streamAseArg->clientAse->ase.presentationDelayMax,
                                     &aseInfo,
                                     peerFraming,
                                     peerPhy,
                                     peerRetransmissionEffort,
                                     peerTransportLatency,
                                     &codecConfiguration,
                                     TRUE);

    /* Keep track of how many cfms we've received */
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               TRUE);

    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupCodecConfigureCfm(streamGroup->rspPhandle,
                                                 BAP_RESULT_SUCCESS,
                                                 streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCodecConfigureCfmFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseCodecConfigureInd(streamGroup->rspPhandle,
                                     errorNotify->responseCode,
                                     streamGroup->id,
                                     streamAseArg->clientAse->ase.presentationDelayMin,
                                     streamAseArg->clientAse->ase.presentationDelayMax,
                                     &aseInfo,
                                     0,
                                     0,
                                     0,
                                     0,
                                     NULL,
                                     TRUE);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupCodecConfigureCfm(streamGroup->rspPhandle,
                                                 BAP_RESULT_SUCCESS,
                                                 streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCodecConfigureIndReceived(void *arg1, void *arg2)
{
    BapResult errorCode = BAP_RESULT_SUCCESS;
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AscsConfigureCodecNotify *cfm = (AscsConfigureCodecNotify *)streamAseArg->cfm;
    uint32  pdMin;
    uint32  pdMax;
    uint32  prefPdMin;
    uint32  prefPdMax;
    uint8 *ptr = NULL;
    BapAseInfo aseInfo;
    uint8  peerFraming;
    uint8  peerPhy;
    uint8  peerRetransmissionEffort;
    uint16 peerTransportLatency;
    BapCodecConfiguration  codecConfiguration;

    peerFraming = cfm->preferredFraming;
    peerPhy = cfm->preferredPhy;
    peerRetransmissionEffort = cfm->retransmissionNumber;
    peerTransportLatency = ((uint16)cfm->transportLatency[0] | (uint16)(cfm->transportLatency[1] << 8));

    ptr = (uint8 *)&cfm->presentationDelayMin[0];
    pdMin = (uint32)readUint24(&ptr);
    pdMax = (uint32)readUint24(&ptr);

    ptr = (uint8 *)&cfm->preferredPDmin[0];
    prefPdMin = (uint32)readUint24(&ptr);
    prefPdMax = (uint32)readUint24(&ptr);

    if((prefPdMin != 0x000000) && (prefPdMin > pdMin))
        pdMin = prefPdMin;
    if((prefPdMax != 0x000000) && (prefPdMax < pdMax))
        pdMax = prefPdMax;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    memset(&codecConfiguration, 0, sizeof(BapCodecConfiguration));

    /* Get Codec specific configuration parameter */
    if(cfm->codecSpecificConfigLength)
    {
        QblLtv ltv;
        uint8* ltvFormatData;

        /* Set default value if not present */
        codecConfiguration.lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;

        /* Decode LTV format data from the Codec specific parameter */
        for (ltvFormatData = &cfm->codecSpecificConfig[0];
             ltvFormatData < &cfm->codecSpecificConfig[0] + cfm->codecSpecificConfigLength;
             ltvFormatData = qblLtvGetNextLtvStart(&ltv))
        {
            qblLtvInitialise(&ltv, ltvFormatData);

            if ( ! qblLtvDecodeCodecSpecificCapabilities(&ltv, &codecConfiguration))
            {
                BAP_CLIENT_DEBUG(" BAP:: Wrong Codec Config parameter for Type %d\n", ltv.type);
                break; /* Exit the loop */
            }
        }
    }
    
    bapUtilsSendAseCodecConfigureInd(streamGroup->rspPhandle,
                                     errorCode,
                                     streamGroup->id,
                                     pdMin,
                                     pdMax,
                                     &aseInfo,
                                     peerFraming,
                                     peerPhy,
                                     peerRetransmissionEffort,
                                     peerTransportLatency,
                                     &codecConfiguration,
                                     FALSE);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfSendConfigQosOp(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientQosConfigureReq * const prim = (BapInternalUnicastClientQosConfigureReq *)arg2;

    /* We now need to start collecting 'cfm's for the ASE_QOS_CONFIGURE_REQs we've just sent */
    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseQosConfigurations,
                                prim->numAseQosConfigurations);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfQosConfigureCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapResult errorCode = BAP_RESULT_SUCCESS;
    BapAseInfo aseInfo;

    /* TODO: Pass the acceptor's qos values to the app to see if they are acceptable for the use case
     *
     * For now check that the qos values are the same as those sent */
    {
        uint32 localSduIntervalMicrosecs;
        uint8  localPacking;
        uint8  localFraming;
        uint8  localPhy;
        uint16 localMaxSduSize;
        uint8  localRetransmissionEffort;
        uint16 localTransportLatencyMilliseconds;

        if (! bapAseQosDeserialise(&streamAseArg->clientAse->ase.qos,
                                   &localSduIntervalMicrosecs,
                                   &localPacking,
                                   &localFraming,
                                   &localPhy,
                                   &localMaxSduSize,
                                   &localRetransmissionEffort,
                                   &localTransportLatencyMilliseconds))
        {
            errorCode = BAP_RESULT_INVALID_PARAMETER;
        }
    }

    /* TODO decode audio_context */
    
    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseQosConfigureInd(streamGroup->rspPhandle,
                                   errorCode,
                                   streamGroup->id,
                                   streamAseArg->clientAse->ase.presentationDelayMin,
                                   &aseInfo);

    /* Keep track of how many cfms we've received (and how many were successful)*/
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               (errorCode == BAP_RESULT_SUCCESS));

    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupQosConfigureCfm(streamGroup->rspPhandle,
                                               BAP_RESULT_SUCCESS,
                                               streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfQosConfigureCfmFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseQosConfigureInd(streamGroup->rspPhandle,
                                   errorNotify->responseCode,
                                   streamGroup->id,
                                   streamAseArg->clientAse->ase.presentationDelayMin,
                                   &aseInfo);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupQosConfigureCfm(streamGroup->rspPhandle,
                                               BAP_RESULT_SUCCESS,
                                               streamGroup->id);
    }

    return FSM_EVENT_NULL;
}


fsm_event_t bapClientAseAfCisConnect(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientCisConnectReq * prim = (BapInternalUnicastClientCisConnectReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->cisCount,
                                prim->cisCount);
    return FSM_EVENT_NULL;

}


fsm_event_t bapClientAseAfCisConnectCfmSuccessReceived(void *arg1, void *arg2)
{
    (void)arg1;
    (void)arg2;
    
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCisConnectCfmFailedReceived(void *arg1, void *arg2)
{
    (void)arg1;
    (void)arg2;
    
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfSendCisDisconnect(void *arg1, void *arg2)
{
    (void)arg1;
    (void)arg2;
    
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCisDisconnectReceived(void *arg1, void *arg2)
{
    (void)arg1;
    (void)arg2;
    
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfCisDisconnectIndReceived(void *arg1, void *arg2)
{
    (void)arg1;
    (void)arg2;
    
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfSendEnableOp(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientEnableReq * const prim = (BapInternalUnicastClientEnableReq *)arg2;

    /* TODO : expect CFMs only for the ENABLE REQs that were successfully sent */
    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseEnableParameters,
                                prim->numAseEnableParameters);


    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfEnableCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;
    uint8* aseChar = (uint8*)streamAseArg->cfm;
    uint8 metadataLength = aseChar[ASE_METADATA_LENGTH_OFFSET];
    uint8* metadata = NULL;

    if (metadataLength)
    {
    	metadata = &aseChar[ASE_METADATA_OFFSET];
    }

    /* Keep track of how many cfms we've received */
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               TRUE);

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseEnableInd(streamGroup->rspPhandle,
                             BAP_RESULT_SUCCESS,
                             streamGroup->id,
                             &aseInfo,
                             metadataLength,
                             metadata);

    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupEnableCfm(streamGroup->rspPhandle,
                                         BAP_RESULT_SUCCESS,
                                         streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfEnableFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseEnableInd(streamGroup->rspPhandle,
                             errorNotify->responseCode,
                             streamGroup->id,
                             &aseInfo,
                             0,
                             NULL);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupEnableCfm(streamGroup->rspPhandle,
                                         BAP_RESULT_SUCCESS,
                                         streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfSendUpdateMetadataOp(void *arg1, void *arg2)
{
    BapClientAse *client_ase = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = client_ase->ase.streamGroup;
    BapInternalUnicastClientUpdateMetadataReq * const prim = (BapInternalUnicastClientUpdateMetadataReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseMetadataParameters,
                                prim->numAseMetadataParameters);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfUpdateMetadataCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;
    uint16 streamingAudioContexts = BAP_CONTEXT_TYPE_UNKNOWN;
    uint8* aseChar = (uint8*)streamAseArg->cfm;
    uint8 metadataLength = aseChar[ASE_METADATA_LENGTH_OFFSET];
    uint8* metadata = NULL;

    if (metadataLength)
    {
    	metadata = &aseChar[ASE_METADATA_OFFSET];
    }

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    if(metadataLength)
    {
        uint8 value[STREAMING_AUDIO_CONTEXT_LENGTH - 1];
        /* checking for Streaming Audio context metadata */
        if (bapUtilsFindLtvValue(metadata, metadataLength,STREAMING_AUDIO_CONTEXT_TYPE,
			                     &value[0], (STREAMING_AUDIO_CONTEXT_LENGTH - 1)))
        {
            streamingAudioContexts = value[0]| (value[1] << 8);
        }
    }

    bapUtilsSendAseUpdateMetadataInd(streamGroup->rspPhandle,
                                     BAP_RESULT_SUCCESS,
                                     streamGroup->id,
                                     &aseInfo,
                                     streamingAudioContexts,
                                     metadataLength,
                                     metadata);

    /* Keep track of how many cfms we've received */
    if(streamGroup->cfmCounter.nExpectedCfm > 0)
    {
        bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                                   TRUE);
    }
    else
        return FSM_EVENT_NULL;

    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapUtilsSendStreamGroupMetadataCfm(streamGroup->rspPhandle,
                                           BAP_RESULT_SUCCESS,
                                           streamGroup->id);
    }


    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfUpdateMetadataFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseUpdateMetadataInd(streamGroup->rspPhandle,
                                     errorNotify->responseCode,
                                     streamGroup->id,
                                     &aseInfo,
                                     BAP_CONTEXT_TYPE_UNKNOWN,
                                     0,
                                     NULL);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapUtilsSendStreamGroupMetadataCfm(streamGroup->rspPhandle,
                                           BAP_RESULT_SUCCESS,
                                           streamGroup->id);
    }

    return FSM_EVENT_NULL;
}


fsm_event_t bapClientAseAfSendStartReadyOp(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientReceiverReadyReq * const prim = (BapInternalUnicastClientReceiverReadyReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAses,
                                prim->numAses);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfStartReadyReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;
    bool clientInitiated = TRUE;
    /* Keep track of how many cfms we've received */
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               TRUE);

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    if( streamAseArg->clientAse->ase.serverDirection == BAP_SERVER_DIRECTION_SINK)
    {
        clientInitiated = FALSE;
    }
    bapUtilsSendAseReceiverReadyInd(streamGroup->rspPhandle,
                                    streamGroup->id,
                                    BAP_RECEIVER_START_READY,
                                    BAP_RESULT_SUCCESS,
                                    &aseInfo,
                                    clientInitiated);

    if((clientInitiated) &&(bapClientAllNotificationReceived(streamGroup)))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupReceiverReadyCfm(streamGroup->rspPhandle,
                                                BAP_RESULT_SUCCESS,
                                                streamGroup->id,
                                                BAP_RECEIVER_START_READY);
    }

    return FSM_EVENT_NULL;
}


fsm_event_t bapClientAseAfStartReadyFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReceiverReadyInd(streamGroup->rspPhandle,
                                    streamGroup->id,
                                    BAP_RECEIVER_START_READY,
                                    errorNotify->responseCode,
                                    &aseInfo,
                                    TRUE);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupReceiverReadyCfm(streamGroup->rspPhandle,
                                                BAP_RESULT_SUCCESS,
                                                streamGroup->id,
                                                BAP_RECEIVER_START_READY);
    }

    return FSM_EVENT_NULL;
}


fsm_event_t bapClientAseAfSendDisableOp(void *arg1, void *arg2)
{
    BapClientAse *clientAse = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = clientAse->ase.streamGroup;
    BapInternalUnicastClientDisableReq * const prim = (BapInternalUnicastClientDisableReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseDisableParameters,
                                prim->numAseDisableParameters);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfDisableCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;

    /* Keep track of how many cfms we've received */
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               TRUE);

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;

    /* special handling */
    if((aseInfo.aseState == ASE_STATE_QOS_CONFIGURED) && 
        (streamAseArg->clientAse->ase.serverDirection == BAP_SERVER_DIRECTION_SINK))
    {
        aseInfo.aseState = ASE_STATE_DISABLING;
    }
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseDisableInd(streamGroup->rspPhandle,
                              BAP_RESULT_SUCCESS,
                              streamGroup->id,
                              &aseInfo,
                              TRUE);

    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapUtilsSendStreamGroupDisableCfm(streamGroup->rspPhandle,
                                          BAP_RESULT_SUCCESS,
                                          streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfDisableCfmFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseDisableInd(streamGroup->rspPhandle,
                              errorNotify->responseCode,
                              streamGroup->id,
                              &aseInfo,
                              TRUE);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapUtilsSendStreamGroupDisableCfm(streamGroup->rspPhandle,
                                          BAP_RESULT_SUCCESS,
                                          streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfDisableIndReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;

    /* Send Server initiated Disable Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseDisableInd(streamGroup->rspPhandle,
                              BAP_RESULT_SUCCESS,
                              streamGroup->id,
                              &aseInfo,
                              FALSE);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfStopReadySendOp(void *arg1, void *arg2)
{
    BapClientAse *client_ase = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = client_ase->ase.streamGroup;
    BapInternalUnicastClientReceiverReadyReq * const prim = (BapInternalUnicastClientReceiverReadyReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAses,
                                prim->numAses);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfStopReadyCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;
    bool clientInitiated = TRUE;
    
    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    if( streamAseArg->clientAse->ase.serverDirection == BAP_SERVER_DIRECTION_SINK)
    {
        clientInitiated = FALSE;
    }
    else
    {
        /* Keep track of how many cfms we've received */
        bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                                   TRUE);
    }

    bapUtilsSendAseReceiverReadyInd(streamGroup->rspPhandle,
                                    streamGroup->id,
                                    BAP_RECEIVER_STOP_READY,
                                    BAP_RESULT_SUCCESS,
                                    &aseInfo,
                                    clientInitiated);

    if(clientInitiated && bapClientAllNotificationReceived(streamGroup))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupReceiverReadyCfm(streamGroup->rspPhandle,
                                                BAP_RESULT_SUCCESS,
                                                streamGroup->id,
                                                BAP_RECEIVER_STOP_READY);
    }
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfStopReadyCfmFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReceiverReadyInd(streamGroup->rspPhandle,
                                    streamGroup->id,
                                    BAP_RECEIVER_STOP_READY,
                                    errorNotify->responseCode,
                                    &aseInfo,
                                    TRUE);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupDisableCfm(streamGroup->rspPhandle,
                                          BAP_RESULT_SUCCESS,
                                          streamGroup->id);
    }

    return FSM_EVENT_NULL;
}


fsm_event_t bapClientAseAfSendReleaseOp(void *arg1, void *arg2)
{
    BapClientAse *client_ase = (BapClientAse *)arg1;
    BapStreamGroup *streamGroup = client_ase->ase.streamGroup;
    BapInternalUnicastClientReleaseReq * const prim = (BapInternalUnicastClientReleaseReq *)arg2;

    bapConfirmationCounterReset(&streamGroup->cfmCounter,
                                prim->numAseReleaseParameters,
                                prim->numAseReleaseParameters);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfReleaseCfmReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;

    /* Keep track of how many cfms we've received */
    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               TRUE);


    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReleaseInd(streamGroup->rspPhandle,
                              BAP_RESULT_SUCCESS,
                              streamGroup->id,
                              &aseInfo,
                              TRUE);


    if(bapClientAllNotificationReceived(streamGroup))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupReleaseCfm(streamGroup->rspPhandle,
                                          BAP_RESULT_SUCCESS,
                                          streamGroup->id);
    }
    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfReleaseCfmFailedReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg *streamAseArg = (BapStreamGroupAseArg *)arg2;
    AseErrorNotify * errorNotify = (AseErrorNotify *)streamAseArg->cfm;
    BapAseInfo aseInfo;

    /* Send intermediate Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReleaseInd(streamGroup->rspPhandle,
                              errorNotify->responseCode,
                              streamGroup->id,
                              &aseInfo,
                              TRUE);

    bapConfirmationCounterReceivedConfirmation(&streamGroup->cfmCounter,
                                               FALSE);

    if (bapConfirmationCounterAllConfirmationsHaveBeenReceived(&streamGroup->cfmCounter))
    {
        bapConfirmationCounterReset(&streamGroup->cfmCounter, 0, 0);
        bapUtilsSendStreamGroupReleaseCfm(streamGroup->rspPhandle,
                                          BAP_RESULT_SUCCESS,
                                          streamGroup->id);
    }

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfReleaseIndReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;

    /* Send Server initiated Release Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReleaseInd(streamGroup->rspPhandle,
                              BAP_RESULT_SUCCESS,
                              streamGroup->id,
                              &aseInfo,
                              FALSE);

    return FSM_EVENT_NULL;
}

fsm_event_t bapClientAseAfReleasedIndReceived(void *arg1, void *arg2)
{
    BapStreamGroup *streamGroup = (BapStreamGroup *)arg1;
    BapStreamGroupAseArg  *streamAseArg = (BapStreamGroupAseArg *)arg2;
    BapAseInfo aseInfo;

    /* Send Server initiated Release Indication */
    aseInfo.aseId = streamAseArg->clientAse->ase.id;
    aseInfo.aseState = streamAseArg->clientAse->ase.aseStateOnServer;
    aseInfo.cisId= streamAseArg->clientAse->ase.cis->cisId;

    bapUtilsSendAseReleasedInd(streamGroup->rspPhandle,
                               streamGroup->id,
                               &aseInfo);

    return FSM_EVENT_NULL;
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */


/**@}*/
