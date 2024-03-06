/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"

#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_pacs.h"
#include "bap_server_lib.h"

#define BAP_VS_COMPANY_ID                   (0x000A) /* Qualcomm Technologies Intl. Ltd.*/
#define BAP_VS_CODEC_ID_APTX                (0x0001) /* AptX */
#define BAP_VS_CODEC_ID_APTX_LITE           (0x0002) /* AptX Lite */
#define BAP_VS_CODEC_ID_APTX_AD             (0x00AD) /* AptX Adaptive */
#define BAP_VS_CODEC_ID_APTX_AD_R4          (0x01AD) /* AptX Adaptive R4 */

#define BAPServer_isVSAptXAdaptive(info)  ((info->codecId.codingFormat == 0xff)&&\
                                    (info->codecId.companyId == BAP_VS_COMPANY_ID) && \
                                    ((info->codecId.vendorSpecificCodecId == BAP_VS_CODEC_ID_APTX) || \
                                    (info->codecId.vendorSpecificCodecId == BAP_VS_CODEC_ID_APTX_AD) || \
                                    (info->codecId.vendorSpecificCodecId == BAP_VS_CODEC_ID_APTX_AD_R4)))

#define BAPServer_isAptXLite(info)  ((info->codecId.codingFormat == 0xff)&&\
                                    (info->codecId.companyId == BAP_VS_COMPANY_ID) && \
                                    (info->codecId.vendorSpecificCodecId == BAP_VS_CODEC_ID_APTX_LITE))

static GattAscsServerConfigureQosRsp * pendingConfigureQosResponse = NULL;
static PdLookupTable pDTable = { 0 };
/* store the Server preferred Max transport latency values to pass to ASCS */
static uint16 aseMaxTransportLatencylist[BAP_SERVER_MAX_NUM_ASES];
static BapServerQosParams bapServerQOSParams;

bool BapServerUnicastRegisterCodecPdMin(bapProfileHandle profileHandle,
                                        const BapServerCodecPdMin *bapServerCodecPdMin,
                                        uint8 numEntries)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst)
    {
        pDTable.pdLookup = bapServerCodecPdMin;
        pDTable.numEntries = numEntries;
        return TRUE;
    }
    else
    {
        BAP_SERVER_DEBUG("BapServerUnicastRegisterPdLookupTable: BAP instance is null\n");
    }
    return FALSE;
}

bool BapServerUnicastSetQosParams(bapProfileHandle profileHandle,
                             const BapServerQosParams *bapServerQosParams)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if (bapInst && bapServerQosParams)
    {
        CsrMemCpy(&bapServerQOSParams, bapServerQosParams, sizeof(BapServerQosParams));
        return TRUE;
    }

    return FALSE;
}

void bapServerQosParamInit(void)
{
    bapServerQOSParams.lowMaxTransportLatancy = ASE_TRANSPORT_LOW_LATENCY_MAX;
    bapServerQOSParams.balanceMaxTransportLatancy = ASE_TRANSPORT_BALANCE_LATENCY_MAX;
    bapServerQOSParams.highMaxTransportLatancy = ASE_TRANSPORT_LATENCY_MAX;
    bapServerQOSParams.preferredRtn = ASE_RETRANSMISSION_NUMBER;
}

void BapServerUnicastPopulateConfigureCodecData(ConnectionId cid, uint8 aseId, GattAscsServerConfigureCodecServerReqInfo * configureCodecData)
{
    CSR_UNUSED(cid);

    configureCodecData->retransmissionNumber= (bapServerQOSParams.preferredRtn > 0)?
            bapServerQOSParams.preferredRtn : ASE_RETRANSMISSION_NUMBER;
    configureCodecData->phyPreference= ASE_PHY_PREFERENCE;
    configureCodecData->framing= ASE_PREFERRED_FRAMING;
    configureCodecData->transportLatencyMax= ASE_TRANSPORT_LATENCY_MAX;
    configureCodecData->presentationDelayMin= ASE_PRESENTATION_DELAY_MIN;
    configureCodecData->presentationDelayMax= ASE_PRESENTATION_DELAY_MAX;
    configureCodecData->preferredPresentationDelayMin= ASE_PREFERRED_PRESENTATION_DELAY_MAX;
    configureCodecData->preferredPresentationDelayMax= ASE_PREFERRED_PRESENTATION_DELAY_MAX;
    if((aseId > 0) && (aseId <= BAP_SERVER_MAX_NUM_ASES))
    {
        configureCodecData->transportLatencyMax = (aseMaxTransportLatencylist[aseId-1] > 0)?
            aseMaxTransportLatencylist[aseId-1] : ASE_TRANSPORT_LATENCY_MAX;
    }
}

bool BapServerValidateStreamingContext(bapProfileHandle profileHandle, 
                                      ConnId connectionId,uint8 aseId,
                                      uint8 * metadata, uint8 *metadataLength)
{
    AudioContextType audioContext = AUDIO_CONTEXT_TYPE_UNKNOWN;
    GattAscsAseDirection direction = GATT_ASCS_ASE_DIRECTION_SERVER_UNINITIALISED;
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE((bapProfileHandle) profileHandle);

    if(bapInst == NULL)
        return FALSE;

    /* If there is no metadata, app shall assume to be audio context to be unspecified */
    if(( *metadataLength == 0 ) && (metadata == NULL))
    {
        /* No validation required for Unspecified context */
        return TRUE;
    }

    audioContext = BapServerLtvUtilitiesGetStreamingAudioContext(metadata,
                                                                             *metadataLength);
    direction = GattAscsReadAseDirection(bapInst->ascsHandle, connectionId,  aseId);

    if (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
    {
        return (audioContext && (GattPacsServerGetAudioContextsAvailability(
                        bapInst->pacsHandle, PACS_SERVER_IS_AUDIO_SINK) & audioContext) == audioContext);
    }
    else if (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
    {
        return (audioContext && (GattPacsServerGetAudioContextsAvailability(
                        bapInst->pacsHandle, PACS_SERVER_IS_AUDIO_SOURCE)& audioContext) == audioContext);
    }

    return FALSE;
}

static AseDirectionType bapServerConvertAscsDirectionToLeBapDirection(GattAscsAseDirection ascs_direction)
{
    AseDirectionType direction = ASE_DIRECTION_UNINITIALISED;

    switch (ascs_direction)
    {
        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK:
            direction = ASE_DIRECTION_AUDIO_SINK;
        break;
        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE:
            direction = ASE_DIRECTION_AUDIO_SOURCE;
        break;
        default:
        break;
    }

    return direction;
}

static bool bapCheckVBChannelPresent(GattAscsServerConfigureCodecInd * ind)
{
    uint8 i;
    for( i = 0; i < ind->numAses; i++ )
    {
        if((ind->gattAscsServerConfigureCodecClientInfo[i].targetLatency == GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY) &&
            (ind->gattAscsServerConfigureCodecClientInfo[i].direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE))
        {
            uint8 sampleValue = 0;
            uint8 ltvType = BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY;

            if(ind->gattAscsServerConfigureCodecClientInfo[i].codecId.codingFormat == GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
                ltvType = BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY_VS;
            
            (void)BapServerLtvUtilitiesFindLtvValue(ind->gattAscsServerConfigureCodecClientInfo[i].codecConfiguration,
                ind->gattAscsServerConfigureCodecClientInfo[i].codecConfigurationLength,
                        ltvType, &sampleValue, 1);

            /* Check for Voice back channel config 16khz or 32Khz*/
            if((sampleValue == BAP_SERVER_SAMPLING_FREQUENCY_16kHz) || (sampleValue == BAP_SERVER_SAMPLING_FREQUENCY_32kHz)
#ifdef ENABLE_LEA_GAMING_MODE_SOURCE_80KBPS
            || (sampleValue == BAP_SERVER_SAMPLING_FREQUENCY_48kHz)
#endif
            )
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static uint32 bapServerGetMinPD(uint8 targetLatency, uint8 sampleFreq, uint8 frameDuration,
    GattAscsAseDirection direction, uint8 codecId , bool vbc)
{
    uint8 i = 0;
    uint16 samplingValue=(1 << (sampleFreq - 1));
    uint8 fDurValue= (1 << frameDuration);

    if(pDTable.pdLookup == NULL)
    {
        BAP_SERVER_DEBUG(" bapServerGetMinPD no pdLookup\n");
        return ASE_PRESENTATION_DELAY_MIN;
    }
    
    for( i = 0; i< pDTable.numEntries; i++)
    {
        if(targetLatency == pDTable.pdLookup[i].targetLatency)
        {
            if((samplingValue & pDTable.pdLookup[i].samplingFreq) &&
                (fDurValue & pDTable.pdLookup[i].frameDuration))
            {
                if(direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
                {
                    if(codecId == PACS_LC3_CODEC_ID)
                    {
                        if(!vbc && (pDTable.pdLookup[i].sourceLc3PdMin == 0))
                        {
                            /* Gaming mode without voice back channel */
                            return pDTable.pdLookup[i].sinkLc3PdMin;
                        }
                        else if(vbc && (pDTable.pdLookup[i].sourceLc3PdMin))
                        {
                            /* Gaming mode with voice back channel */
                            return pDTable.pdLookup[i].sinkLc3PdMin;
                        }
                        else if(!vbc)
                        {
                            return pDTable.pdLookup[i].sinkLc3PdMin;
                        }

                    }
                    else if(codecId == PACS_VENDOR_CODEC_ID)
                    {
                        if(!vbc && (pDTable.pdLookup[i].sourceVsAptxPdMin == 0))
                        {
                            /* Gaming mode without voice back channel */
                            return pDTable.pdLookup[i].sinkVsAptxPdMin;
                        }
                        else if(vbc && (pDTable.pdLookup[i].sourceVsAptxPdMin))
                        {
                            return pDTable.pdLookup[i].sinkVsAptxPdMin;
                        }
                        else if(!vbc)
                        {
                            return pDTable.pdLookup[i].sinkVsAptxPdMin;
                        }
                    }
                }
                else
                {
                     if(codecId == PACS_LC3_CODEC_ID)
                        return pDTable.pdLookup[i].sourceLc3PdMin;
                     else if(codecId == PACS_VENDOR_CODEC_ID)
                        return pDTable.pdLookup[i].sourceVsAptxPdMin;
                }
            }
        }
    }
    return ASE_PRESENTATION_DELAY_MAX;
}
static void bapServerOnResourceAvailableResponse(BAP *bapInst,
                                                 bool cig_resources_available)
{
    if(pendingConfigureQosResponse)
    {
        if(pendingConfigureQosResponse->cid && pendingConfigureQosResponse->numAses)
        {
            uint8 i;

            for( i = 0; i < pendingConfigureQosResponse->numAses; i++)
            {
                pendingConfigureQosResponse->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_INSUFFICIENT_RESOURCES;

                if(cig_resources_available )
                {
                    uint32 sampleRate = 0;
                    uint16 frameDuration = 0;
                    GattAscsServerConfigureCodecInfo * codecInfo = GattAscsReadCodecConfiguration(bapInst->ascsHandle,
                                                                                                   pendingConfigureQosResponse->cid,
                                                                                                   pendingConfigureQosResponse->ase[i].gattAscsAseResult.aseId);
                    if(codecInfo)
                    {
                        sampleRate = BapServerLtvUtilitiesGetSampleRate(codecInfo->infoFromServer.codecConfiguration,
                                                   codecInfo->infoFromServer.codecConfigurationLength);

                        if (BAPServer_isVSAptXAdaptive(codecInfo))
                            frameDuration = pendingConfigureQosResponse->ase[i].gattAscsServerConfigureQosRspInfo->sduInterval;
                        else  if (BAPServer_isAptXLite(codecInfo))
                            frameDuration = BAP_SERVER_DEFAULT_APTX_LITE_FRAME_DURATION;
                        else
                            frameDuration = BapServerLtvUtilitiesGetFrameDuration(codecInfo->infoFromServer.codecConfiguration,
                                                                                                 codecInfo->infoFromServer.codecConfigurationLength);

                        BAP_SERVER_DEBUG("bapServerOnResourceAvailableResponse p_delay %u pd_min %u",
                                         pendingConfigureQosResponse->ase[i].gattAscsServerConfigureQosRspInfo->presentationDelay,
                                         codecInfo->infoFromServer.presentationDelayMin);
                    }

                    if (   (sampleRate == 0) || (frameDuration == 0))
                    {
                        pendingConfigureQosResponse->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE;
                        pendingConfigureQosResponse->ase[i].gattAscsAseResult.additionalInfo = GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION;
                    }
                    else
                    {
                        BapServerAseQosConfiguredInd * message = NULL;
                        message = (BapServerAseQosConfiguredInd*)CsrPmemZalloc(sizeof(BapServerAseQosConfiguredInd));

                        message->type = BAP_SERVER_ASE_QOS_CONFIGURED_IND;
                        message->connectionId = pendingConfigureQosResponse->cid;
                        message->aseQosConfig.aseId = pendingConfigureQosResponse->ase[i].gattAscsAseResult.aseId;
                        message->aseQosConfig.cisId = pendingConfigureQosResponse->ase[i].gattAscsServerConfigureQosRspInfo->cisId;
                        message->aseQosConfig.sampleRate = sampleRate;
                        message->aseQosConfig.sduSize = pendingConfigureQosResponse->ase[i].gattAscsServerConfigureQosRspInfo->maximumSduSize;
                        message->aseQosConfig.frameDuration = frameDuration;
                        message->aseQosConfig.presentationDelay = pendingConfigureQosResponse->ase[i].gattAscsServerConfigureQosRspInfo->presentationDelay;

                        BapServerMessageSend(bapInst->appUnicastTask, message);

                        pendingConfigureQosResponse->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_SUCCESS;
                    }
                }
            }
        }
        GattAscsServerConfigureQosResponse(bapInst->ascsHandle, pendingConfigureQosResponse);
        FREE_RESPONSE_ASE(pendingConfigureQosResponse);
        pendingConfigureQosResponse = NULL;
    }
    else
    {
        BAP_SERVER_PANIC("PANIC: bapServerOnResourceAvailableResponse null ind");
    }
}

static void bapServerCheckCigResourcesAreAvailable(BAP *bapInst)
{
    bapServerOnResourceAvailableResponse(bapInst, TRUE);
}

static GattAscsAseResultValue bapServerValidateCodecSpecificConfigLtvTypeFields(uint8* ltvData, uint8 ltvDataLength)
{
    LTV ltv;
    uint8* dataPtr = ltvData;

    /* Step over each LTV */
    while ((dataPtr - ltvData) < ltvDataLength)
    {
        LTV_INITIALISE(&ltv, dataPtr);

        switch (LTV_TYPE(&ltv))
        {
            case BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_OCTETS_PER_FRAME_CODEC:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY_VS:
                /* Fall through */
            case BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION_VS:
                /* Fall through */
                break;
            default:
            {
                /* This 'Type' is unrecognised */
                return GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE;
            }
        }
        dataPtr = LTV_NEXT(&ltv);
    }
    return GATT_ASCS_ASE_RESULT_SUCCESS;
}

static GattAscsAseResultValue bapServerValidateCodecConfigLtvs(GattAscsServerConfigureCodecClientInfo* clientInfo)
{
    GattAscsAseResultValue ltvTypeFieldResult = bapServerValidateCodecSpecificConfigLtvTypeFields(clientInfo->codecConfiguration,
                                                                                                  clientInfo->codecConfigurationLength);
    if (ltvTypeFieldResult != GATT_ASCS_ASE_RESULT_SUCCESS)
    {
        return ltvTypeFieldResult;
    }

    if (!bapServerIsLtvValueInRange(clientInfo->codecConfiguration,
                                    clientInfo->codecConfigurationLength,
                                    sizeof(uint8),
                                    BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY,
                                    0x01,  /* spec defined min value: BAPS Assigned Numbers v11 */
                                    0x0D)) /* spec defined max value: BAPS Assigned Numbers v11 */
    {
        return GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE;
    }

    if (!bapServerIsLtvValueInRange(clientInfo->codecConfiguration,
                                    clientInfo->codecConfigurationLength,
                                    sizeof(uint8),
                                    BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION,
                                    0x00,  /* spec defined min value: BAPS Assigned Numbers v11 */
                                    0x01)) /* spec defined max value: BAPS Assigned Numbers v11 */
    {
        return GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE;
    }

    return GATT_ASCS_ASE_RESULT_SUCCESS;
}

static void bapServerOnConfigureCodecInd(BAP *bapInst, GattAscsServerConfigureCodecInd * ind)
{
    if(ind)
    {
        MAKE_ASCS_SERVER_RESPONSE_ASE(GattAscsServerConfigureCodecRsp, ind->numAses);
        response->cid = ind->cid;
        response->numAses = ind->numAses;

        if(ind->cid && ind->numAses)
        {
            uint8 i;
            for( i = 0; i < ind->numAses; i++ )
            {   uint8 samplingValue = 0;  /* Sampling frequency value from LTV format */
                uint8 frameDurValue = 0;
                GattAscsTargetLatency targetLatency = ind->gattAscsServerConfigureCodecClientInfo[i].targetLatency;
                GattAscsAseDirection direction = ind->gattAscsServerConfigureCodecClientInfo[i].direction;
                uint32 pDMin = ASE_PRESENTATION_DELAY_MIN;

                response->ase[i].gattAscsAseResult.aseId = ind->gattAscsServerConfigureCodecClientInfo[i].aseId;
                response->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_SUCCESS;

                /* Validate codec config parameters range for LC3 */
                if(ind->gattAscsServerConfigureCodecClientInfo[i].codecId.codingFormat != GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
                {
                    response->ase[i].gattAscsAseResult.value = bapServerValidateCodecConfigLtvs(&ind->gattAscsServerConfigureCodecClientInfo[i]);
                }
                if (response->ase[i].gattAscsAseResult.value == GATT_ASCS_ASE_RESULT_SUCCESS)
                {
                    /* Validate codec id and config parameters from supported the PACS library */
                    if(bapServerIsCodecConfigSuppported(bapInst->pacsHandle, &ind->gattAscsServerConfigureCodecClientInfo[i],
                                &samplingValue, &frameDurValue))
                    {
                        BapServerAseCodecConfiguredInd * message = NULL;

                        message = (BapServerAseCodecConfiguredInd*)CsrPmemZalloc(sizeof(BapServerAseCodecConfiguredInd));
                        BAP_SERVER_DEBUG("BapServerOnConfigureCodecInd ase=0x%x codec supported with sampF 0x%x FrameD 0x%x",
                            ind->gattAscsServerConfigureCodecClientInfo[i].aseId, samplingValue, frameDurValue);

                        /* Fill BAP upstream primitive */
                        message->type = BAP_SERVER_ASE_CODEC_CONFIGURED_IND;
                        message->connectionId = ind->cid;
                        message->aseCodecConfig.aseId = ind->gattAscsServerConfigureCodecClientInfo[i].aseId;
                        message->aseCodecConfig.direction = bapServerConvertAscsDirectionToLeBapDirection(ind->gattAscsServerConfigureCodecClientInfo[i].direction);

                        BapServerMessageSend(bapInst->appUnicastTask, message);
                    }
                    else
                    {
                        BAP_SERVER_DEBUG("BapServerOnConfigureCodecInd ase=0x%x codec not supported",
                            ind->gattAscsServerConfigureCodecClientInfo[i].aseId);
                        response->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_UNSUPPORTED_AUDIO_CAPABILITIES;
                    }
                }
                else
                {
                    BAP_SERVER_DEBUG("BapServerOnConfigureCodecInd ase=0x%x sampling rate not supported",
                        ind->gattAscsServerConfigureCodecClientInfo[i].aseId);
                    response->ase[i].gattAscsAseResult.additionalInfo = GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION;
                }

                response->ase[i].gattAscsServerConfigureCodecServerInfo = (GattAscsServerConfigureCodecServerInfo *)calloc(1, sizeof(GattAscsServerConfigureCodecServerInfo));
                /* Note: ASCS retreives all other fields used in the Configure Codec Notification by calling
                         BapServerUnicastPopulateConfigureCodecResponseData() */

                /* Update the Min Presentation Delay value */
                if (response->ase[i].gattAscsAseResult.value == GATT_ASCS_ASE_RESULT_SUCCESS)
                {
                    uint8 codecId = ind->gattAscsServerConfigureCodecClientInfo[i].codecId.codingFormat;
                    uint8 aseId = ind->gattAscsServerConfigureCodecClientInfo[i].aseId;
                    uint16 maxTransportLatency = bapServerQOSParams.highMaxTransportLatancy;

                    BAP_SERVER_DEBUG(" BAP_SERVER: Codec Confg targetLatency %d direction %d samplingValue %d", 
                                    targetLatency, direction, samplingValue);
                    if(((samplingValue == BAP_SERVER_SAMPLING_FREQUENCY_48kHz)
#ifdef ENABLE_LEA_GAMING_MODE_SINK_32KHZ
                    || (samplingValue == BAP_SERVER_SAMPLING_FREQUENCY_32kHz)
#endif
                        ) && (targetLatency == GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY)&&
                        (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK))
                    {   
                        /* check for voice back channel configuration for Source ASES */
                        if(bapCheckVBChannelPresent(ind))
                        {
                            pDMin = bapServerGetMinPD(targetLatency, samplingValue,
                                frameDurValue, direction,codecId, TRUE);
                            BAP_SERVER_DEBUG(" BAP_SERVER: GAMING +VBC PD MIN %d", pDMin);
                        }
                        else
                        {
                            pDMin = bapServerGetMinPD(targetLatency, samplingValue,
                                frameDurValue, direction,codecId, FALSE);
                            BAP_SERVER_DEBUG(" BAP_SERVER: GAMING PD MIN %d", pDMin);
                        }
                    }
                    else /* Non-Gaming configurations */
                    {
                        pDMin = bapServerGetMinPD(targetLatency, samplingValue,
                                frameDurValue, direction,codecId, FALSE);
                        BAP_SERVER_DEBUG(" BAP_SERVER: Other case PD MIN %d", pDMin);
                    }

                    /* update preferred Max Transport latency value */
                    if(ind->gattAscsServerConfigureCodecClientInfo[i].codecId.codingFormat != GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
                    {
                        if(targetLatency == GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY)
                        {
                            maxTransportLatency = bapServerQOSParams.lowMaxTransportLatancy;
                        }
                        else if(targetLatency == GATT_ASCS_TARGET_LATENCY_TARGET_BALANCED_LATENCY_AND_RELIABILITY)
                        {
                            maxTransportLatency = bapServerQOSParams.balanceMaxTransportLatancy;
                        }
                        BAP_SERVER_DEBUG(" BAP_SERVER: Ase Id %d,target Latency %d MaxTransLatency %d",aseId, targetLatency, maxTransportLatency);
                    }

                    if((aseId > 0) && (aseId <= BAP_SERVER_MAX_NUM_ASES))
                    {
                        aseMaxTransportLatencylist[aseId-1] = maxTransportLatency;
                    }
                }
                /* forward codec config, responsibility of freeing passes back to service */
                response->ase[i].gattAscsServerConfigureCodecServerInfo->presentationDelayMin= pDMin;
                response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength = ind->gattAscsServerConfigureCodecClientInfo[i].codecConfigurationLength;
                response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration = ind->gattAscsServerConfigureCodecClientInfo[i].codecConfiguration;
            }
        }
        GattAscsServerConfigureCodecResponse(bapInst->ascsHandle, response);
        FREE_RESPONSE_ASE(response);
    }
}

static void bapServerOnConfigureQosInd(BAP *bapInst,
                                       GattAscsServerConfigureQosInd * ind)
{
    MAKE_ASCS_SERVER_RESPONSE_ASE(GattAscsServerConfigureQosRsp, ind->numAses);

    response->cid = ind->cid;
    response->numAses = ind->numAses;

    if(response->cid && response->numAses)
    {
        uint8 i;
        for(i = 0; i < response->numAses; i++)
        {
            response->ase[i].gattAscsAseResult.value = GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR;
            response->ase[i].gattAscsAseResult.aseId = ind->ase[i].aseId;
            response->ase[i].gattAscsServerConfigureQosRspInfo = ind->ase[i].qosInfo;
        }
    }

    if(pendingConfigureQosResponse == NULL)
    {
        pendingConfigureQosResponse = response;
        bapServerCheckCigResourcesAreAvailable(bapInst);
    }
    else
    {
        GattAscsServerConfigureQosResponse(bapInst->ascsHandle, response);
    }
}

static bapServerResponse bapServerValidateMetadataLtvTypeFields(uint8* ltvData, uint8 ltvDataLength, uint8* invalidType)
{
    LTV ltv;
    uint8* dataPtr = ltvData;

    /* Step over each LTV */
    while ((dataPtr - ltvData) < ltvDataLength)
    {
        LTV_INITIALISE(&ltv, dataPtr);

        switch (LTV_TYPE(&ltv))
        {
            case BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_CCID_LIST:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_EXTENDED_METADATA:
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PROGRAM_INFO:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_LANGUAGE:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PROGRAM_INFO_URI:
                break;
                /* Fall through */
            case BAP_METADATA_LTV_TYPE_PARENTAL_RATING:
                break;

            default:
            {
                /* This 'Type' is unrecognised */
                *invalidType = LTV_TYPE(&ltv);
                return GATT_ASCS_ASE_RESULT_UNSUPPORTED_METADATA;
            }
        }
        dataPtr = LTV_NEXT(&ltv);
    }
    return GATT_ASCS_ASE_RESULT_SUCCESS;
}

bapServerResponse BapServerValidateMetadataLtvs(uint8* metadata,
                                                            uint8 metadataLength,
                                                            uint8* invalidMetadataType)
{
    bapServerResponse ltvValidationResult = bapServerValidateMetadataLtvTypeFields(metadata,
                                                                                       metadataLength,
                                                                                       invalidMetadataType);
    if (ltvValidationResult != GATT_ASCS_ASE_RESULT_SUCCESS)
    {
        /* invalidMetadataType is already set from within the bapServerValidateMetadataLtvTypeField function */
        return ltvValidationResult;
    }

    if (!bapServerIsLtvValueInRange(metadata,
                                    metadataLength,
                                    sizeof(uint16),
                                    BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                    0x0001,  
                                    0x0FFF)) /* For all valid bits 0 to bit 11 */
    {
        *invalidMetadataType = BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS;
        return GATT_ASCS_ASE_RESULT_INVALID_METADATA;
    }

    if (!bapServerIsLtvValueInRange(metadata,
                                    metadataLength,
                                    sizeof(uint16),
                                    BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS,
                                    0x0001,
                                    0x0FFF)) /* For all valid bits 0 to bit 11 */
    {
        *invalidMetadataType = BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS;
        return GATT_ASCS_ASE_RESULT_INVALID_METADATA;
    }

    return GATT_ASCS_ASE_RESULT_SUCCESS;
}

static void  bapServerOnEnableInd(BAP *bapInst,
                                  GattAscsServerEnableInd * ind)
{
    BapServerAseEnabledInd * message = (BapServerAseEnabledInd*)CsrPmemZalloc(sizeof(BapServerAseEnabledInd) + 
        ((ind->numAses - 1) *(sizeof(BapServerEnableIndInfo))));

    message->type = BAP_SERVER_ASE_ENABLED_IND;
    message->connectionId = ind->cid;
    message->numAses = ind->numAses;
    CsrMemCpy(message->bapServerEnableIndInfo, ind->gattAscsServerEnableIndInfo,
        (ind->numAses *(sizeof(BapServerEnableIndInfo))));

    BapServerMessageSend(bapInst->appUnicastTask, message);
}

static void bapServerOnReceiverReadyInd(BAP *bapInst,
                                        GattAscsServerReceiverReadyInd * ind)
{
    if(ind->cid && ind->numAses)
    {
        uint8 i;

        BAP_SERVER_DEBUG("leBapUnicastServer_OnReceiverReadyInd cid=0x%x num_ases=0x%x", ind->cid, ind->numAses);
        for(i = 0; i < ind->numAses; i++)
        {
            BapServerAseReceiverStartReadyInd * message = NULL;

            message = (BapServerAseReceiverStartReadyInd*)CsrPmemZalloc(sizeof(BapServerAseReceiverStartReadyInd));
            message->type = BAP_SERVER_ASE_RECEIVER_START_READY_IND;
            message->connectionId = ind->cid;
            message->aseId = ind->aseId[i];

            BapServerMessageSend(bapInst->appUnicastTask, message);
        }
    }
}

static void bapServerOnUpdateMetadataInd(BAP *bapInst,
                                         GattAscsServerUpdateMetadataInd * ind)
{

    BapServerAseUpdateMetadataInd * message = (BapServerAseUpdateMetadataInd*)CsrPmemZalloc(sizeof(BapServerAseUpdateMetadataInd) + 
        ((ind->numAses - 1) *(sizeof(BapServerUpdateMetadataInfo))));

    message->type = BAP_SERVER_ASE_UPDATE_METADATA_IND;
    message->connectionId = ind->cid;
    message->numAses = ind->numAses;
    CsrMemCpy(message->bapServerUpdateMetadataInfo,
                ind->gattAscsServerUpdateMetadataIndInfo,
                (ind->numAses *(sizeof(BapServerUpdateMetadataInfo))));

    BapServerMessageSend(bapInst->appUnicastTask, message);
}

static void bapServerOnDisableInd(BAP *bapInst,
                                  GattAscsServerDisableInd * ind)
{
    MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(GattAscsServerDisableRsp, ind->numAses);

    response->cid = ind->cid;
    response->numAses = ind->numAses;

    if(ind->cid && ind->numAses)
    {
        uint8 i;
        for(i = 0; i < ind->numAses; i++)
        {
            BapServerAseDisabledInd * message = NULL;

            response->gattAscsAseResult[i].aseId = ind->aseId[i];
            response->gattAscsAseResult[i].value = GATT_ASCS_ASE_RESULT_SUCCESS;

            message = (BapServerAseDisabledInd*)CsrPmemZalloc(sizeof(BapServerAseDisabledInd));
            message->type = BAP_SERVER_ASE_DISABLED_IND;
            message->connectionId = ind->cid;
            message->aseId = ind->aseId[i];

            BapServerMessageSend(bapInst->appUnicastTask, message);
        }
    }
    GattAscsServerDisableResponse(bapInst->ascsHandle, response);
    FREE_RESPONSE_ASE_RESULT(response);
}

static void bapServerOnReleaseInd(BAP *bapInst,
                                  GattAscsServerReleaseInd * ind)
{
    if(ind->cid && ind->numAses)
    {
        uint8 i;
        for(i = 0; i < ind->numAses; i++)
        {
            BapServerAseReleasedInd * message = NULL;

            message = (BapServerAseReleasedInd*)CsrPmemZalloc(sizeof(BapServerAseReleasedInd));
            message->type = BAP_SERVER_ASE_RELEASED_IND;
            message->connectionId = ind->cid;
            message->aseId = ind->aseId[i];

            BapServerMessageSend(bapInst->appUnicastTask, message);
        }
    }

}

static void bapServerOnReceiverStopReadyInd(BAP *bapInst,
                                            GattAscsServerReceiverStopReadyInd * ind)
{
    MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(GattAscsServerReceiverStopReadyRsp, ind->numAses);

    response->cid = ind->cid;
    response->numAses = ind->numAses;

    if(ind->cid && ind->numAses)
    {
        uint8 i;
        for(i = 0; i < ind->numAses; i++)
        {
            BapServerAseReceiverStopReadyInd * message = NULL;

            response->gattAscsAseResult[i].aseId = ind->aseId[i];
            response->gattAscsAseResult[i].value = GATT_ASCS_ASE_RESULT_SUCCESS;

            message = (BapServerAseReceiverStopReadyInd*)CsrPmemZalloc(sizeof(BapServerAseReceiverStopReadyInd));
            message->type = BAP_SERVER_ASE_RECEIVER_STOP_READY_IND;
            message->connectionId = ind->cid;
            message->aseId = ind->aseId[i];

            BapServerMessageSend(bapInst->appUnicastTask, message);
        }
    }
    GattAscsReceiverStopReadyResponse(bapInst->ascsHandle,
                                      response);
    FREE_RESPONSE_ASE_RESULT(response);
}

void bapServerSendConfigChangeInd(BAP *bapInst,
    ConnectionId cid,
    BapServerConfigType configType,
    bool configChangeComplete)
{
    BapServerConfigChangeInd * message = NULL;
    AppTask appTask = bapInst->appUnicastTask;

    message = (BapServerConfigChangeInd*)CsrPmemZalloc(sizeof(BapServerConfigChangeInd));
    message->type = BAP_SERVER_CONFIG_CHANGE_IND;
    message->connectionId = cid;
    message->configChangeComplete = configChangeComplete;
    message->configType = configType;

    if(configType == BAP_SERVER_CONFIG_BASS)
    {
        appTask = bapInst->appBroadcastTask;
    }
    BapServerMessageSend(appTask, message);
}


static void bapServerOnAscsConfigChangeInd(BAP *bapInst,
    GattAscsServerConfigChangeInd * ind)
{
    if(ind->cid)
    {
        bapServerSendConfigChangeInd(bapInst,
                                     ind->cid,
                                     BAP_SERVER_CONFIG_ASCS,
                                     ind->configChangeComplete);
    }
}

void bapServerHandleGattAscsServerMsg(BAP *bapInst,
                                     void *message)
{
    GattAscsServerMessageId *prim = (GattAscsServerMessageId *)message;

    BAP_SERVER_INFO("bapServerHandleGattAscsServerMsg MESSAGE:GattAscsServerMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_ASCS_SERVER_CONFIGURE_CODEC_IND:
            bapServerOnConfigureCodecInd(bapInst,(GattAscsServerConfigureCodecInd *)message);
            break;
        case GATT_ASCS_SERVER_CONFIGURE_QOS_IND:
            bapServerOnConfigureQosInd(bapInst, (GattAscsServerConfigureQosInd *)message);
            break;
        case GATT_ASCS_SERVER_ENABLE_IND:
            bapServerOnEnableInd(bapInst, (GattAscsServerEnableInd *)message);
            break;
        case GATT_ASCS_SERVER_RECEIVER_READY_IND:
            bapServerOnReceiverReadyInd(bapInst, (GattAscsServerReceiverReadyInd *)message);
            break;
        case GATT_ASCS_SERVER_UPDATE_METADATA_IND:
            bapServerOnUpdateMetadataInd(bapInst, (GattAscsServerUpdateMetadataInd *)message);
            break;
        case GATT_ASCS_SERVER_DISABLE_IND:
            bapServerOnDisableInd(bapInst, (GattAscsServerDisableInd *)message);
            break;
        case GATT_ASCS_SERVER_RELEASE_IND:
            bapServerOnReleaseInd(bapInst, (GattAscsServerReleaseInd *)message);
            break;
        case GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND:
            bapServerOnReceiverStopReadyInd(bapInst, (GattAscsServerReceiverStopReadyInd *)message);
            break;
        case GATT_ASCS_SERVER_CONFIG_CHANGE_IND:
            bapServerOnAscsConfigChangeInd(bapInst, (GattAscsServerConfigChangeInd *)message);
            break;
        default:
            BAP_SERVER_PANIC("PANIC: BAP Server: unhandled unicast/ASCS prim\n");
            break;
    }
}
