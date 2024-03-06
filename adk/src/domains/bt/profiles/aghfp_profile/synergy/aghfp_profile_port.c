/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application domain ag profile port component for synergy.
*/

#include <panic.h>
#include <logging.h>
#include <pmalloc.h>
#include <string.h>
#include <stdio.h>
#include <bdaddr.h>
#include <stream.h>
#include <system_state.h>
#include <device_list.h>
#include <device_properties.h>

#include <hfg_lib.h>
#include <cm_lib.h>
#include <csr_bt_hfg_prim.h>
#include <csr_bt_cm_prim.h>

#include "aghfp_profile.h"
#include "aghfp_profile_port_protected.h"
#include "aghfp_profile_config.h"
#include "aghfp_profile_private.h"
#include "aghfp_profile_instance.h"
#include "aghfp_profile_sm.h"
#include "aghfp_profile_audio.h"

#include "connection_manager.h"
#include "telephony_messages.h"
#include "ps_key_map.h"

static     uint16 ag_supported_qce_codec_modes;

/*! \brief Find AGHFP instance using connection Id

    This function returns the AGHFP instance from the existing device record for the matching
    connection id as requested.

    \param  conn_id Connection Id received from library
*/

static aghfpInstanceTaskData * aghfpProfileAbstract_GetInstanceForConnId(CsrBtHfgConnectionId conn_id)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    unsigned index;
    aghfpInstanceTaskData* aghfp_instance = NULL, *temp = NULL;

    DeviceList_GetAllDevicesWithProperty(device_property_bdaddr, &devices, &num_devices);

    for(index = 0; index < num_devices; index++)
    {
        temp = AghfpProfileInstance_GetInstanceForDevice(devices[index]);

        if (temp && temp->connection_id == conn_id)
        {
            aghfp_instance = temp;
            break;
        }
    }

    free(devices);

    return aghfp_instance;
}

static uint16 aghfpGetAudioDisconnectStatusCfmCode(CsrBtHfgAudioDisconnectCfm  *ind)
{
    uint16 status_code = aghfp_audio_disconnect_success;

    switch(ind->resultSupplier)
    {
        case CSR_BT_SUPPLIER_HCI:
        {
            switch(ind->resultCode) 
            {
                case hci_error_conn_term_local_host:
                case hci_error_oetc_user:
                case hci_success:
                    status_code = aghfp_audio_disconnect_success;
                    break;
                default:
                    status_code = aghfp_audio_disconnect_error;
                    break;
            }
        }
        break;

        case CSR_BT_SUPPLIER_CM:
        {
            switch(ind->resultCode)
            {
                case CSR_BT_RESULT_CODE_CM_ALREADY_DISCONNECTING:
                    status_code = aghfp_audio_disconnect_in_progress;
                    break;
                default:
                    status_code = aghfp_audio_disconnect_success;
                    break;
            }    
        }
        break;

        case CSR_BT_SUPPLIER_HFG:
        default:
        {
            status_code = aghfp_audio_disconnect_success;
            break;
        }

    }
    return status_code;
}

uint16 aghfpGetAudioDisconnectStatusCode(AGHFP_AUDIO_DISCONNECT_IND_T  *ind)
{
    uint16 status_code = aghfp_audio_disconnect_success;

    switch(ind->reasonSupplier)
    {
        case CSR_BT_SUPPLIER_HCI:
        {
            switch(ind->reasonCode) 
            {
                case hci_error_conn_term_local_host:
                case hci_error_oetc_user:
                case hci_success:
                    status_code = aghfp_audio_disconnect_success;
                    break;
                default:
                    status_code = aghfp_audio_disconnect_error;
                    break;
            }
        }
        break;

        case CSR_BT_SUPPLIER_CM:
        {
            switch(ind->reasonCode)
            {
                case CSR_BT_RESULT_CODE_CM_ALREADY_DISCONNECTING:
                    status_code = aghfp_audio_disconnect_in_progress;
                    break;
                default:
                    status_code = aghfp_audio_disconnect_success;
                    break;
            }    
        }
        break;

        case CSR_BT_SUPPLIER_HFG:
        default:
        {
            status_code = aghfp_audio_disconnect_success;
            break;
        }

    }
    return status_code;
}

uint16 aghfpGetSlcDisconnectStatusCode(AGHFP_SLC_DISCONNECT_IND_T *ind)
{
    uint16 status_code = aghfp_disconnect_success;

    switch(ind->reasonSupplier)
    {
        case CSR_BT_SUPPLIER_HCI:
        {
            switch(ind->reasonCode) 
            {
                case hci_error_conn_term_local_host:
                case hci_error_oetc_user:
                case hci_success:
                    status_code = aghfp_disconnect_success;
                    break;

                default:
                    status_code = aghfp_disconnect_error;
                    break;
                    
            }
        }
        break;

        case CSR_BT_SUPPLIER_CM:
        {
            switch(ind->reasonCode)
            {
                case CSR_BT_RESULT_CODE_CM_ALREADY_DISCONNECTING:
                    status_code = aghfp_disconnect_error;
                    break;
                default:
                    status_code = aghfp_disconnect_success;
                    break;
            }
        }
        break;

        case CSR_BT_SUPPLIER_RFCOMM:
        {
            switch (ind->reasonCode)
            {
                case rfcomm_disconnect_success:
                case rfcomm_disconnect_normal_disconnect:
                    status_code = aghfp_disconnect_success;
                    break;
                case rfcomm_disconnect_abnormal_disconnect:
                    status_code = aghfp_disconnect_link_loss;
                    break;
            }
        }
        break;
            
        case CSR_BT_SUPPLIER_HFG:
        default:
        {
            status_code = aghfp_disconnect_success;
            break;
        }
    }        

    return status_code;
}


uint16 aghfpGetAudioConnectStatusCode(AGHFP_AUDIO_CONNECT_CFM_T*ind)
{
    aghfp_audio_connect_status     status_code  = aghfp_audio_connect_failure;
    switch(ind->resultSupplier)
    {
        case CSR_BT_SUPPLIER_HFG:
        {
            switch(ind->resultCode)
            {
                case CSR_BT_RESULT_CODE_HFG_SUCCESS:
                {
                    return aghfp_audio_connect_success;
                }

                case CSR_BT_RESULT_CODE_HFG_ALREADY_CONNECTING:
                {
                    return aghfp_audio_connect_in_progress;
                }

                case RESULT_CODE_HFG_OPERATION_TIMEOUT:
                {
                    return aghfp_audio_connect_timeout;
                }

                case CSR_BT_RESULT_CODE_HFG_CONNECT_ATTEMPT_FAILED:
                default:
                {
                    return aghfp_audio_connect_failure;
                }
             }
        }

        case CSR_BT_SUPPLIER_CM:
        break;

        default:
        DEBUG_LOG_ERROR("AghfpProfilePort_GetStatusCode unexpected supplier:0x%04X", ind->resultSupplier);
        break;

    }

    return status_code;
}


#ifdef INCLUDE_SWB
/*****************************************************************************/
static uint16 aghfpConvertCodecModeIdToBit(uint16 mode_id)
{
    /* Works on the premise that despite all Codec Mode ID values being uint16,
     * none of them are currently over the value 16. This also returns 0 if the
     * special value 0xFFFF is used.
     */
    if (mode_id > 16) 
        return 0;
    else
        return (1 << mode_id);
}

/*****************************************************************************/
static uint16 aghfpConvertCodecModeBitToId(uint16 mode_bit)
{
    switch (mode_bit)
    {
        case CODEC_64_2_EV3: 
            return aptx_adaptive_64_2_EV3;
        case CODEC_64_2_EV3_QHS3:
            return aptx_adaptive_64_2_EV3_QHS3;
        case CODEC_128_QHS3:
            return aptx_adaptive_128_QHS3;
        case CODEC_64_QHS3:
            return aptx_adaptive_64_QHS3;
        default:
            DEBUG_LOG("Unsupported Codec Mode ID! (%d)", mode_bit);
            return CODEC_MODE_ID_UNSUPPORTED;
    }
}

static void aghfpProfile_SetQcAudioParams(aghfpInstanceTaskData *instance)
{
    aghfp_audio_params* params = NULL;

    if(params == NULL)
    {
        instance->audio_params = (aghfp_audio_params*)malloc(sizeof(aghfp_audio_params));
    }

    params = instance->audio_params;
    DEBUG_LOG("aghfpProfile_SetQcAudioParams(%p), codec = %d",params, instance->qce_codec_mode_id);
    
    //instance->audio_params->audioQuality = CSR_BT_HF_QCE_DEFAULT_AUDIO_QUALITY;
    /* If the QCE codec is selected, then set the parameters accoring to the selected codec */
    switch (instance->qce_codec_mode_id)
    {
        case CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3:
        case CSR_BT_HF_QCE_CODEC_TYPE_64_2_EV3_QHS3:
        {
            params->txBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH;
            params->rxBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH;
            params->maxLatency    = CSR_BT_HF_QCE_Q0_MAX_LATENCY;
            params->voiceSettings = CSR_BT_HF_QCE_QO_VOICE_SETTINGS;
            params->reTxEffort    = CSR_BT_QCE_Q0_RETX_EFFORT;
        }
        break;

        case CSR_BT_HF_QCE_CODEC_TYPE_128_QHS3:
        {
            params->txBandwidth   = CSR_BT_HF_QCE_Q1_RX_TX_BANDWIDTH;
            params->rxBandwidth   = CSR_BT_HF_QCE_Q1_RX_TX_BANDWIDTH;
            params->maxLatency    = CSR_BT_HF_QCE_Q1_MAX_LATENCY;
            params->voiceSettings = CSR_BT_HF_QCE_Q1_VOICE_SETTINGS;
            params->reTxEffort    = CSR_BT_QCE_Q1_RETX_EFFORT;
        }
        break;

        case CSR_BT_HF_QCE_CODEC_TYPE_64_QHS3:
        {
            params->txBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_TX_BANDWIDTH;
            params->rxBandwidth   = CSR_BT_SCO_DEFAULT_ACCEPT_RX_BANDWIDTH;
            params->maxLatency    = CSR_BT_HF_QCE_Q2_MAX_LATENCY;
            params->voiceSettings = CSR_BT_HF_QCE_Q2_VOICE_SETTINGS;
            params->reTxEffort    = CSR_BT_QCE_Q2_RETX_EFFORT;
        }
        break;
        default:
            DEBUG_LOG("aghfpProfile_SetQcAudioParams invalid codec id");
        break;
    }

}

static bool aghfpSendQcsSelectCodecReq(aghfpInstanceTaskData *instance, uint16 codec_mode_bit)
{
    char codec_mode_id_str[10];
    char *rsp = malloc(20);
    DEBUG_LOG("aghfpSendQcsSelectCodecReq AG in QCE Mode - send Qualcomm Codec Selection +QCS\n");
    
    /* Construct and sent the +%QAC:<n> notification. */
    sprintf(codec_mode_id_str, 
            "%d",
            aghfpConvertCodecModeBitToId(codec_mode_bit)
    );

    strcpy(rsp, AGHFG_STR_QCS_CMD);
    sprintf(codec_mode_id_str, "%d", aghfpConvertCodecModeBitToId(codec_mode_bit));
    strcat(rsp, codec_mode_id_str);
    strcat(rsp, "\r\n");

    HfgSwbRspSend(instance->connection_id, strlen(rsp), rsp);

    DEBUG_LOG("aghfpSendQcsSelectCodecReq Qualcomm Codec Selection sent:\n");
#ifdef AG_DEBUG
    uint8 result_len = 0;
    uint8 count = 0;

    result_len = CsrStrLen(rsp);
    while (count < result_len)
    {
        DEBUG_LOG("%c, %x", rsp[count], rsp[count]);
        count++;
    }
#endif
    return TRUE;
}

static bool aghfpHandleQacReq(aghfpInstanceTaskData *instance, CsrBtHfgAtCmdInd* ind)
{
    char* result;
    uint16 result_len;
    char *rsp = malloc(20);
    char codec_mode_id[10];
    int bit;
    uint16 ag_codec_modes =  ag_supported_qce_codec_modes;

    /* if the ag doesnt support aany of the codecs then return an error */
    if ( !(ag_codec_modes & QCE_CODEC_MODE_ID_MASK) )
    {
        DEBUG_LOG("AG is not in Qualcomm Codec Extensions mode.\n");
        return FALSE;
    }

    DEBUG_LOG("AG codec modes supported(%x) sent\n", ag_codec_modes);

    strcpy(rsp, AGHFG_STR_QAC_RSP);
    /* For every bit set in the AG Codec Modes bit field. */
    for (bit=0; (bit < 16) && ag_codec_modes ; bit++)
    {
        uint16 bit_mask = 1 << bit;
    
        /* If the codec bit isn't set, go to the next one. */
        if ( !(bit_mask & ag_codec_modes) )
            continue;
    
        sprintf(codec_mode_id, 
                "%d",
                aghfpConvertCodecModeBitToId(bit_mask)
               );
    
        strcat(rsp, codec_mode_id);
    
        ag_codec_modes ^= bit_mask;
    
        /* Are there still more Codec Mode IDs (bits) to process? 
         * Add a comma before the next Codec Mode ID.
         */
        if (ag_codec_modes)
        {
            strcat(rsp, ",");
        }
    }
    strcat(rsp, "\r\n");

    /* First process the hf supported modes         */
    result = CsrStrStr(ind->command, AGHFP_STR_QAC_CMD);
    if(result != NULL)
    {
        result += AGHFP_STR_QAC_CMD_LENGTH;
        result_len = CsrStrLen(ind->command);
        DEBUG_LOG("aghfpHandleQacReq QAC received (len = %d, diff = %d)", result_len, result - ind->command);

#ifdef AG_DEBUG
        uint8 count = 0;
        while (count < result_len)
        {
            DEBUG_LOG("%c, %x", ind->command[count], ind->command[count]);
            count++;
        }
#endif
        
        while (result_len > (result - ind->command))
        {
            DEBUG_LOG("aghfpHandleQacReq (%x, %x)", result, *result);
            if(*result == ',')
            {
                result +=1;
                continue;
            }
            instance->hf_supported_qce_codec_modes |= aghfpConvertCodecModeIdToBit(*result - 0x30);
            result += 1;
        }

        /* send the set of supported codecs by AG */
        HfgSwbRspSend(ind->connectionId, strlen(rsp), rsp);
        HfgSwbRspSend(ind->connectionId, AGHFG_STR_OK_LENGTH, CsrStrDup(AGHFG_STR_OK));
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_SWB */

static void aghfpProfile_CheckEncryptedSco(aghfpInstanceTaskData * instance)
{
    DEBUG_LOG("agHfpProfile_CheckEncryptedSco(%p) encrypted=%d sink=%x)",
              instance, instance->bitfields.encrypted, instance->sco_sink);

    /* Check SCO is active */
    if (AghfpProfile_IsScoActiveForInstance(instance) && AghfpProfile_IsCallActiveForInstance(instance))
    {
        /* Check if link is encrypted */
        if (!IsInstanceLink_Encrypted(instance))
        {
            voice_source_t source = AghfpProfileInstance_GetVoiceSourceForInstance(instance);
            if (source != voice_source_none)
            {
                Telephony_NotifyCallBecameUnencrypted(source);
            }
            /* \todo Mute the MIC to prevent eavesdropping */
        }
    }
}


static void aghfpProfile_HandleSelectedCodecInd(uint16 id, CsrBtHfgSelectedCodecInd* ind)
{
    aghfpInstanceTaskData *instance = AghfpProfilePort_GetInstance(id ,ind);
    UNUSED(id);
    PanicNull(instance);

    DEBUG_LOG("aghfpProfile_HandleSelectedCodecInd(%p), codec = %d", instance, ind->codecToUse);
    instance->codec = ind->codecToUse;

    // reset the qce codec selected
    instance->qce_codec_mode_id = CODEC_MODE_ID_UNSUPPORTED;

    if(CSR_BT_WBS_MSBC_CODEC_MASK & ind->codecToUse)
    {
        instance->using_wbs = TRUE;
    }
}

static void aghfpProfile_HandleAtCmdInd(uint16 id, CsrBtHfgAtCmdInd* ind)
{
    char  *result = NULL;
    aghfpInstanceTaskData *instance = NULL;
    uint16 str_len = 0;
    bdaddr bd_addr = {0};

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);
    instance = AghfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);
    
    UNUSED(id);
    if (!instance)
    {
         instance = PanicNull(AghfpProfileInstance_Create(&bd_addr, TRUE));
    }

    DEBUG_LOG("aghfpProfile_HandleAtCmdInd(%p), command = (%p), connid = %x", instance, ind->command, ind->connectionId);

#ifdef AG_DEBUG
    uint8 count = 0;
    uint8 result_len = CsrStrLen(ind->command);
   
    while (count < result_len)
    {
        DEBUG_LOG("%c", ind->command[count]);
        count++;
    }
#endif

    result = CsrStrStr(ind->command, AGHFP_STR_QAC_CMD);
    DEBUG_LOG("aghfpProfile_HandleAtCmdInd(%x)", result);
    
#ifdef INCLUDE_SWB
    /* Handling Q codecs supported by the Q device */
    if(result != 0)
    {
        if(!aghfpHandleQacReq(instance, ind))
        {
            HfgSwbRspSend(ind->connectionId, AGHFG_STR_ERROR_LENGTH, CsrStrDup(AGHFG_STR_ERROR));
        }
    }
    /* Handling Q codec selection */
    else 
    {
        result = CsrStrStr(ind->command, AGHFP_STR_QCS_RSP);
        DEBUG_LOG("aghfpProfile_HandleAtCmdInd(%x, %x)", result, ind->command);
        if(result != 0)
        {
            uint16 codec_mode_id;
            str_len = strlen(ind->command);
            result += AGHFP_STR_QCS_RSP_LENGTH;

            DEBUG_LOG("aghfpProfile_HandleAtCmdInd (len = %d, diff = %d)", str_len, result - ind->command);

            while (str_len > (result - ind->command))
            {
                codec_mode_id = aghfpConvertCodecModeIdToBit(*result - 0x30);
                DEBUG_LOG("aghfpProfile_HandleAtCmdInd (codec_mode_id :: %d)", codec_mode_id);

                if(codec_mode_id & instance->hf_supported_qce_codec_modes & ag_supported_qce_codec_modes)
                {
                    DEBUG_LOG("aghfpProfile_HandleAtCmdInd ( HfgSwbRspSend OK)");
                    HfgSwbRspSend(ind->connectionId, AGHFG_STR_OK_LENGTH, CsrStrDup(AGHFG_STR_OK));
                }
                instance->qce_codec_mode_id = *result - 0x30;
                aghfpProfile_SetQcAudioParams(instance);
                return;
            }
        }
        HfgSwbRspSend(ind->connectionId, AGHFG_STR_ERROR_LENGTH, CsrStrDup(AGHFG_STR_ERROR));
        return;
    }
#else
    UNUSED(str_len);
    UNUSED(result);
#endif

}

static void aghfpProfile_HandleHfpAudioDisconnectCfm(uint16 id, CsrBtHfgAudioDisconnectCfm *msg)
{
    aghfpInstanceTaskData *instance = NULL;
    voice_source_t voice_source; 
    uint16 status_code;

    DEBUG_LOG("aghfpProfile_HandleHfpAudioDisconnectCfm(%p) enum:aghfp_audio_disconnect_status:%x, supplier=%x", instance, msg->resultCode , msg->resultSupplier);

    instance = AghfpProfilePort_GetInstance(id, msg);
    if(instance == NULL)
    {
        DEBUG_LOG("aghfpProfile_HandleHfpAudioDisconnectCfm install cannot be NULL"); 
        return;
    }

    voice_source = AghfpProfileInstance_GetVoiceSourceForInstance(instance);
    status_code = aghfpGetAudioDisconnectStatusCfmCode(msg);

    switch (status_code)
    {
        case aghfp_audio_disconnect_success:
            TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(appAgHfpGetStatusNotifyList()), APP_AGHFP_SCO_DISCONNECTED_IND);
            instance->sco_sink = NULL;
            instance->bitfields.sco_status = aghfp_sco_disconnected;
            Telephony_NotifyCallAudioDisconnected(voice_source);
            break;
        case aghfp_audio_disconnect_in_progress:
            break;
        default:
            instance->bitfields.sco_status = (instance->sco_sink) ? aghfp_sco_connected : aghfp_sco_disconnected;
            break;
    }
}

static void aghfpProfile_HandleAudioStatusInd(uint16 id, CsrBtHfgStatusAudioInd* message)
{
    DEBUG_LOG_FN_ENTRY("aghfpProfile_HandleAudioStatusInd (Audio type = %d)", message->audioType);

    aghfpInstanceTaskData *instance;

    instance = AghfpProfilePort_GetInstance(id, message);

    switch(message->audioType)
    {
        case CSR_BT_HFG_AUDIO_SUP_PACKETS:
        {
            CsrBtHfgAudioSupPackets *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioSupPackets))
            {
                data = (CsrBtHfgAudioSupPackets *)message->audioSetting;
                DEBUG_LOG("packet type = 0x%x", *data);
            }
        }
        break;
            
        case CSR_BT_HFG_AUDIO_MAX_LATENCY:
        {
            CsrBtHfgAudioMaxLatency *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioMaxLatency))
            {
                data = (CsrBtHfgAudioMaxLatency *)message->audioSetting;
                DEBUG_LOG("max latency = 0x%x", *data);
            }
        }
        break;

        case CSR_BT_HFG_AUDIO_RETRANSMISSION:
        {
            CsrBtHfgAudioRetransmission *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioRetransmission))
            {
                data = (CsrBtHfgAudioRetransmission *)message->audioSetting;
                DEBUG_LOG("audio retx = 0x%x", *data);
            }
        }
        break;


        case CSR_BT_HFG_AUDIO_TX_BANDWIDTH:
        {
            CsrBtHfgAudioTxBandwidth  *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioTxBandwidth))
            {
                data = (CsrBtHfgAudioTxBandwidth  *)message->audioSetting;
                DEBUG_LOG("tx bandwidth = 0x%x", *data);
            }
        }
        break;

        case CSR_BT_HFG_AUDIO_RX_BANDWIDTH:
        {
            CsrBtHfgAudioRxBandwidth *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioRxBandwidth))
            {
                data = (CsrBtHfgAudioRxBandwidth *)message->audioSetting;
                DEBUG_LOG("rx bandwidth = 0x%x", *data);
            }
        }
        break;

        case CSR_BT_HFG_AUDIO_VOICE_SETTINGS:
        {
            CsrBtHfgAudioVoiceSettings *data;
            if(message->audioSetting != NULL && message->audioSettingLen == sizeof(CsrBtHfgAudioVoiceSettings))
            {
                data = (CsrBtHfgAudioVoiceSettings *)message->audioSetting;
                DEBUG_LOG("voice setting = 0x%x", *data);
            }
        }
        break;

        case CSR_BT_HFG_AUDIO_SCO_STATUS:
        {
            CsrBtHfgAudioScoStatus *audio;
            audio = (CsrBtHfgAudioScoStatus *)message->audioSetting;

            if(audio  != NULL)
            {
                DEBUG_LOG("audio weSco = %d, txinterval = %d",  instance->wesco, instance->tesco);

                AghfpProfile_StoreConnectParams(instance, instance->codec,
                    audio->weSco, audio->txInterval, instance->qce_codec_mode_id, instance->using_wbs);

                DEBUG_LOG("audio sco status= 0x%x, supplier = 0x%x, linkType = %d, airMode = %d, \
                    rxPacketLength = %d, txInterval = %d, txPacketLength = %d, weSco = %d", 
                    audio->resultCode, audio->resultSupplier, audio->linkType, audio->airMode, 
                    audio->rxPacketLength, audio->txInterval, audio->txPacketLength, audio->weSco);
            }
        }
        break;

        default:
            break;
    }
}

static void aghfpProfile_HandleHfAudioAcceptConnectInd(uint16 id, CsrBtHfgAudioAcceptConnectInd* message)
{
    DEBUG_LOG_FN_ENTRY("aghfpProfile_HandleHfAudioAcceptConnectInd");

    DEBUG_LOG_INFO("Link type is %d", message->linkType);
    UNUSED(id);

    aghfpInstanceTaskData *instance = AghfpProfilePort_GetInstance(id, message);
    bool accept = FALSE;

    if (!instance)
    {
        DEBUG_LOG_NO_INSTANCE(aghfpProfile_HandleHfAudioAcceptConnectInd);
        /* No instance means a SCO was attempted without an SLC */
        HfgAudioAcceptConnectResSend(message->connectionId, HCI_ERROR_REJ_BY_REMOTE_NO_RES,
                                     NULL, PCM_SLOT, PCM_SLOT_REALLOC);
        return;
    }

    switch (instance->state)
    {
        case AGHFP_STATE_CONNECTED_IDLE:
        case AGHFP_STATE_CONNECTED_INCOMING:
        case AGHFP_STATE_CONNECTED_OUTGOING:
        case AGHFP_STATE_CONNECTED_ACTIVE:
        {
            if (instance->bitfields.sco_status == aghfp_sco_disconnected)
            {
                DEBUG_LOG("aghfpProfile_HandleHfAudioAcceptConnectInd, accepting");
                instance->bitfields.sco_status = aghfp_sco_connecting;
                accept = TRUE;
            }
        }
        break;

        default:
            DEBUG_LOG("aghfpProfile_HandleHfAudioAcceptConnectInd in wrong state enum:aghfpState:%x, rejecting", instance->state);
        break;
    }

    AghfpProfileAbstract_AudioConnectResponse(instance, accept, 0, AghfpProfile_GetAudioParams(instance));
    
}

static void aghfpProfile_HandleHfIndicationValInd(uint16 id, CsrBtHfgHfIndicatorValueInd* message)
{
    DEBUG_LOG_FN_ENTRY("aghfpProfile_HandleHfIndicationValInd (id = %d, val = %d)", message->indId, message->value);
    UNUSED(id);
}

static void aghfpProfile_HandleCmEncryptChangeInd(uint16 id, CsrBtCmEncryptChangeInd *ind)
{
   tp_bdaddr bd_addr = { 0 };
    aghfpInstanceTaskData *instance;

    UNUSED(id);
    DEBUG_LOG("aghfpProfile_HandleCmEncryptChangeInd: encrypted(%d)", ind->encryptType);

    bd_addr.transport = ind->transportType;
    bd_addr.taddr.type = ind->deviceAddrType;

    BdaddrConvertBluestackToVm(&bd_addr.taddr.addr, &ind->deviceAddr);
    instance = AghfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);

    if (instance)
    {
        bool encrypted = (ind->encryptType != CSR_BT_CM_ENC_TYPE_NONE);

        switch (AghfpProfile_GetState(instance))
        {
            case AGHFP_STATE_CONNECTING_LOCAL:
            case AGHFP_STATE_CONNECTED_IDLE:
            case AGHFP_STATE_CONNECTED_OUTGOING:
            case AGHFP_STATE_CONNECTED_INCOMING:
            case AGHFP_STATE_CONNECTED_ACTIVE:
            case AGHFP_STATE_DISCONNECTING:
            {
                /* Store encrypted status */
                instance->bitfields.encrypted = encrypted;

                /* Check if SCO is now encrypted (or not) */
                aghfpProfile_CheckEncryptedSco(instance);
            }
            return;

            default:
                AghfpProfile_HandleError(instance, CSR_BT_CM_ENCRYPT_CHANGE_IND, NULL);
            return;
        }
    }
}

static void aghfpProfile_HandleHfpSlcConnectInd(uint16 id, const AGHFP_SLC_CONNECT_IND_T *ind)
{
    bdaddr bd_addr = {0};
    aghfpInstanceTaskData* instance = NULL;

    UNUSED(id);

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);
    instance = AghfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);
    if (!instance)
    {
        DEBUG_LOG_NO_INSTANCE(aghfpProfile_HandleHfpSlcConnectInd);
        instance = PanicNull(AghfpProfileInstance_Create(&bd_addr, TRUE));
    }

    aghfpState state = AghfpProfile_GetState(instance);

    DEBUG_LOG("aghfpProfile_HandleHfpSlcConnectInd enum:aghfpState:%d, result = %d, sup = %d", state, ind->resultCode, ind->resultSupplier);

    switch (state)
    {
        case AGHFP_STATE_CONNECTING_LOCAL:
        case AGHFP_STATE_DISCONNECTED:
        {
            /* Check if SLC connection was successful */
            if (ind->resultCode == CSR_BT_RESULT_CODE_HFG_SUCCESS)
            {
                /* Store BD address of AG device */
                instance->hf_bd_addr = bd_addr;
                instance->connection_id = ind->connectionId;

                /* Store SLC sink */
                instance->slc_sink = StreamRfcommSink((uint16)(CSR_BT_CONN_ID_GET_MASK & ind->connectionId));

                instance->hf_supported_features = ind->supportedFeatures;

                DEBUG_LOG("Supported Features= 0x%x", instance->hf_supported_features);

                /* Check for in-band ringtone support - This is not supported in AGHFP role
                if (instance->hf_supported_features & CSR_BT_HFG_SUPPORT_INBAND_RINGING)
                {
                    instance->bitfields.in_band_ring  = TRUE;
                }*/

                if (instance->bitfields.call_setup == aghfp_call_setup_none)
                {
                    /* If there are no calls being setup then progress to connected state
                       based on the call status
                    */
                    AghfpProfile_SetState(instance, aghfp_call_status_table[instance->bitfields.call_status]);
                }
                else
                {
                    /* If there are calls being setup then progress to connected state
                       based on the call setup stage.
                    */
                    AghfpProfile_SetState(instance, aghfp_call_setup_table[instance->bitfields.call_setup]);
                }

                DEBUG_LOG("aghfpProfile_HandleHfpSlcConnectInd SWB codecs (HF = %x, AG = %x)", instance->hf_supported_qce_codec_modes, ag_supported_qce_codec_modes);

#ifdef INCLUDE_SWB
                /* Check if the (bitmap)codecs supported by both ends atleast match for one of them*/
                if (instance->hf_supported_qce_codec_modes & ag_supported_qce_codec_modes)
                {
                    uint8 idx;
                    uint16 codec_mode_bit;

                    DEBUG_LOG("aghfpProfile_HandleHfpSlcConnectInd SWB codec selection in progress");

                    /* Select the first COMMONLY supported codec mode
                     *
                     * NOTE: this is currently done by bit order LOW to HIGH - a future
                     * refactor may be to use a preferred order set by parameter, but for
                     * our AGHFP implementation the simple solution is to only add the
                     * Codec Mode ID bit that you want, for testing purposes.
                     */
                    for (idx=0; idx<16; idx++)
                    {
                        codec_mode_bit = 1 << idx;
                        if (codec_mode_bit & ag_supported_qce_codec_modes & 
                            instance->hf_supported_qce_codec_modes)
                        {
                            aghfpSendQcsSelectCodecReq(instance, codec_mode_bit);
                            instance->qce_codec_mode_id = idx;
                            return;
                        }
                    }
                }
#endif /* INCLUDE_SWB */
                AghfpProfile_SendSlcStatus(TRUE, &instance->hf_bd_addr);
                return;
            }

            /* Not a successful connection so set to disconnected state */
            AghfpProfile_SetState(instance, AGHFP_STATE_DISCONNECTED);

            if (instance->bitfields.call_status == aghfp_call_none &&
                instance->bitfields.call_setup == aghfp_call_setup_none)
            {
                AghfpProfileInstance_Destroy(instance);
            }
        }
        break;

        default:
        break;
    }
}


static void agfhpProfilePort_HandleResponseHoldStatusRequestInd(uint16 id, AGHFP_RESPONSE_HOLD_STATUS_REQUEST_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("agfhpProfile_HandleResponseHoldStatusRequestInd");
    aghfpInstanceTaskData *instance;
    instance = AghfpProfilePort_GetInstance(id, ind);
    
    if (!instance)
    {
        return;
    }
    
    switch(ind->value)
    {
        case CSR_BT_BTRH_READ_STATUS:
        {
            if(AghfpProfileCallList_GetCallStatusCount(instance->call_list, aghfp_call_state_held) > 0)
            {
                HfgCallHandlingResSend(ind->connectionId, CSR_BT_CME_SUCCESS,
                                            CSR_BT_HFG_BTRH_INCOMING_ON_HOLD);
            }
            else
            {            
                HfgCallHandlingResSend(ind->connectionId, CSR_BT_CME_SUCCESS,
                                            CSR_BT_HFG_BTRH_IGNORE);
            }
        }
        break;
        default:
			HfgCallHandlingResSend(ind->connectionId, CSR_BT_CME_UNKNOWN, CSR_BT_HFG_BTRH_IGNORE);
        break;
    }        

}

static void aghfpProfile_HandleHfgMessage(Message message)
{
    CsrBtHfgPrim *primType = (CsrBtHfgPrim *)message;

    DEBUG_LOG("aghfpProfile_HandleHfgMessages: primType(%x)", *primType);

    switch(*primType)
    {
        case CSR_BT_HFG_SERVICE_CONNECT_IND:
            aghfpProfile_HandleHfpSlcConnectInd(*primType, (CsrBtHfgServiceConnectInd *)message);
            break;

        case CSR_BT_HFG_DISCONNECT_IND:
             aghfpProfile_HandleSlcDisconnectInd(*primType, (CsrBtHfgDisconnectInd *)message);
             break;

        case CSR_BT_HFG_AUDIO_CONNECT_CFM:
        case CSR_BT_HFG_AUDIO_CONNECT_IND:
            aghfpProfile_HandleAgHfpAudioConnectCfm(*primType, (CsrBtHfgAudioConnectCfm*)message);
            break;

        case CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_IND:
            /* Not being handled in the ADK */
             aghfpProfile_HandleHfAudioAcceptConnectInd(*primType, (CsrBtHfgAudioAcceptConnectInd *)message);
             break;

        case CSR_BT_HFG_AUDIO_DISCONNECT_CFM:
            /* VC-Do we notify or take a action here ?*/
            aghfpProfile_HandleHfpAudioDisconnectCfm(*primType, (CsrBtHfgAudioDisconnectCfm*)message);
            break;

        case CSR_BT_HFG_AUDIO_DISCONNECT_IND:
             aghfpProfile_HandleHfpAudioDisconnectInd(*primType, (CsrBtHfgAudioDisconnectInd *)message);
             break;

        case CSR_BT_HFG_ANSWER_IND:
            aghfpProfile_HandleCallAnswerInd(*primType, (CsrBtHfgAnswerInd *)message);
            break;

        case CSR_BT_HFG_REJECT_IND:
            aghfpProfile_HandleCallHangUpInd(*primType, (CsrBtHfgRejectInd*)message);
            break;

        case CSR_BT_HFG_NOISE_ECHO_IND:
            aghfpProfile_HandleNrecSetupInd(*primType, (CsrBtHfgNoiseEchoInd*)message);
            break;

        case CSR_BT_HFG_CALL_HANDLING_IND:
            DEBUG_LOG("aghfpProfile_HandleHfgMessages: CSR_BT_HFG_CALL_HANDLING_IND");
            agfhpProfilePort_HandleResponseHoldStatusRequestInd(*primType, (CsrBtHfgCallHandlingInd*)message);
            break;

        case CSR_BT_HFG_RING_CFM:
            DEBUG_LOG("aghfpProfile_HandleHfgMessages: CSR_BT_HFG_RING_CFM");
            break;

        case CSR_BT_HFG_STATUS_AUDIO_IND:
            aghfpProfile_HandleAudioStatusInd(*primType, (CsrBtHfgStatusAudioInd*)message);
            break;

        case CSR_BT_HFG_SELECTED_CODEC_IND:
            aghfpProfile_HandleSelectedCodecInd(*primType, (CsrBtHfgSelectedCodecInd*)message);
            break;

        case CSR_BT_HFG_AT_CMD_IND:
            aghfpProfile_HandleAtCmdInd(*primType, (CsrBtHfgAtCmdInd*)message);
            break;

        case CSR_BT_HFG_DIAL_IND:
            {
                switch (((AGHFP_DIAL_IND_T*)message)->command)
                {
                    case CSR_BT_HFG_DIAL_NUMBER:
                        aghfpProfile_HandleDialInd(*primType, (AGHFP_DIAL_IND_T*)message);
                        break;

                    case CSR_BT_HFG_DIAL_MEMORY:
                        agfhpProfile_HandleMemoryDialInd(*primType, (AGHFP_MEMORY_DIAL_IND_T*)message);
                        break;

                    case CSR_BT_HFG_DIAL_REDIAL:
                        agfhpProfile_HandleRedialLastCall(*primType, (AGHFP_LAST_NUMBER_REDIAL_IND_T*)message);
                        break;
                }
            }
            break;

        case CSR_BT_HFG_OPERATOR_IND:
            aghfpProfile_HandleNetworkOperatorInd(*primType, (CsrBtHfgOperatorInd *) message);
            break;

        case CSR_BT_HFG_CALL_LIST_IND:
            aghfpProfile_HandleGetCurrentCallsInd(*primType, (CsrBtHfgCallListInd*) message);
            break;

        case CSR_BT_HFG_SUBSCRIBER_NUMBER_IND:
            aghfpProfile_HandleSubscriberNumberInd(*primType, (CsrBtHfgSubscriberNumberInd*) message);
            break;

        case CSR_BT_HFG_HF_INDICATOR_VALUE_IND:
            aghfpProfile_HandleHfIndicationValInd(*primType, (CsrBtHfgHfIndicatorValueInd*) message);
            break;

        case CSR_BT_HFG_SPEAKER_GAIN_IND:
            aghfpProfile_HandleSpeakerVolumeInd(*primType, (CsrBtHfgSpeakerGainInd*) message);
            break;

        case CSR_BT_HFG_MIC_GAIN_IND:
            aghfpProfile_HandleSyncMicGain(*primType, (CsrBtHfgMicGainInd*) message);
            break;

        case CSR_BT_HFG_MANUAL_INDICATOR_IND:
            aghfpProfile_HandleCallIndicationsStatusReqInd(*primType, (CsrBtHfgManualIndicatorInd*)message);
            break;

        default:
             DEBUG_LOG("Unhandled HFG Primitive: 0x%x", *primType);
             break;

    }
    HfgFreeUpstreamMessageContents((void *)message);

    DEBUG_LOG("HFG Primitive Complete:");

}

static void aghfpProfile_HandleCmMessage(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    DEBUG_LOG_INFO("aghfpProfile_HandleCmMessage: primType(%d)", *prim);

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_MASK_CFM:
            break;

        case CSR_BT_CM_ENCRYPT_CHANGE_IND:
            aghfpProfile_HandleCmEncryptChangeInd(*prim, (CsrBtCmEncryptChangeInd *) message);
            break;

        default:
            DEBUG_LOG_INFO("aghfpProfile_HandleCmMessage: Unexpected CM primitive");
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}


void AghfpProfilePort_HandleHfgMessages(Task task, MessageId id, Message message)
{
    UNUSED(task);
    if (id == HFG_PRIM)
    { /* HFG upstream messages */
        aghfpProfile_HandleHfgMessage(message);
    }
    else if (id == CM_PRIM)
    { /* CM upstream messages */
        aghfpProfile_HandleCmMessage(message);
    }
    else
    {
        DEBUG_LOG_INFO("aghfpProfile_TaskMessageHandler MESSAGE:AghfpMessageId:0x%04X", id);
    }

    /* Handle other messages */
    switch (id)
    {
        case CON_MANAGER_CONNECTION_IND:
            /* Just ignore */
            return;

        default:
            break;
    }
}


void AghfpProfilePort_SetLastDialledNumber(AGHFP_DIAL_IND_T *ind)
{
    AghfpProfile_SetLastDialledNumber(strlen(ind->number)-1, (uint8*)ind->number);
}


uint16 AghfpProfilePort_GetSupportedFeatures(void)
{
    return AGHFP_LOCAL_SUPPORTED_FEATURES;
}

uint16 AghfpProfilePort_GetSupportedQceCodec(void)
{
    return CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3;
}


uint16 AghfpProfilePort_GetMemDialNumber(AGHFP_MEMORY_DIAL_IND_T *ind)
{
    UNUSED(ind);
    return 1;
}

uint16 AghfpProfilePort_GetCurCallIndex(AGHFP_CURRENT_CALLS_IND_T *ind)
{
    UNUSED(ind);
    return 0;
}

uint8 AghfpProfilePort_GetSpeakerVolume(AGHFP_SYNC_SPEAKER_VOLUME_IND_T *ind)
{
    return ind->gain;
}


void AghfpProfilePort_InitLibrary(uint16 supported_qce_codecs, uint16 supported_features)
{
    /* if the TEST flag is set, the codecs can be set by a PS KEY */
#ifdef TEST_HFP_CODEC_PSKEY
    uint16 hfp_codec_pskey = 0xffff;    /* default to enable all codecs */

    PsRetrieve(PS_KEY_TEST_HFP_CODEC, &hfp_codec_pskey, sizeof(hfp_codec_pskey));
    DEBUG_LOG("AghfpProfilePort_InitLibrary Initial: hfp_codec_pskey 0x%x  supported_qce_codecs 0x%x  supported_features 0x%x", hfp_codec_pskey, supported_qce_codecs, supported_features);

    supported_features = (hfp_codec_pskey & HFP_CODEC_PS_BIT_WB) ? (supported_features | CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION) : (supported_features & ~CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION);
#ifdef INCLUDE_SWB
    supported_qce_codecs = (hfp_codec_pskey & HFP_CODEC_PS_BIT_SWB) ? supported_qce_codecs : 0;
#endif /* INCLUDE_SWB */

    DEBUG_LOG("AghfpProfilePort_InitLibrary Updated: supported_qce_codecs 0x%x  supported_features 0x%x", supported_qce_codecs, supported_features);
#endif  /* TEST_HFP_CODEC_PSKEY */

    AghfpProfileAbstract_Activate(supported_qce_codecs, supported_features);

    ag_supported_qce_codec_modes = supported_qce_codecs;
    
    /* Tell main application task we have initialised */
    MessageSend(SystemState_GetTransitionTask(), APP_AGHFP_INIT_CFM, 0);
}

void AghfpProfilePort_InitInstanceData(aghfpInstanceTaskData* instance)
{
    instance->connection_id = BT_HFG_CONID_NONE;
    /* initialize to narrow band */
    instance->codec = CSR_BT_WBS_CVSD_CODEC;
    instance->using_wbs = FALSE;
    instance->qce_codec_mode_id = CODEC_MODE_ID_UNSUPPORTED;
    /* caller_id_active_remote is not used for AGHFP port on Synergy library, 
       clip notifications are internally managed by Synergy HFG library */
    instance->bitfields.caller_id_active_remote = TRUE;
}

void AghfpProfilePort_DeinitInstanceData(aghfpInstanceTaskData* instance)
{
    if (instance->audio_params)
    {
        free(instance->audio_params);
        instance->audio_params = NULL;
    }
}

const aghfp_audio_params * AghfpProfile_GetAudioParams(aghfpInstanceTaskData *instance)
{
    DEBUG_LOG_INFO("AghfpProfile_GetAudioParams: using default audio_params" );
    return instance->audio_params;
}


void AghfpProfilePort_HandleAgHfpAudioConnectCfm(aghfpInstanceTaskData *instance, AGHFP_AUDIO_CONNECT_CFM_T *cfm)
{
    /* Store sco_handle for later usage (at the time of audio disconnection) */
    instance->sco_handle = cfm->scoHandle;

    /* Store sink associated with SCO */
    instance->sco_sink = StreamScoSink(cfm->scoHandle);

    /* Check if SCO is now encrypted (or not) */
    aghfpProfile_CheckEncryptedSco(instance);

    AghfpProfile_StoreConnectParams(instance, instance->codec, cfm->weSco, cfm->txInterval, instance->qce_codec_mode_id, instance->using_wbs);
}


aghfpInstanceTaskData* AghfpProfilePort_GetInstance(uint16 id, void* message)
{
    aghfpInstanceTaskData* instance = NULL;

    switch(id)
    {
        case CSR_BT_HFG_SERVICE_CONNECT_IND:
        {            
            bdaddr bd_addr = {0};
            BdaddrConvertBluestackToVm(&bd_addr, &((CsrBtHfgServiceConnectInd *)message)->deviceAddr);
            instance = AghfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);
        }
        break;

        case CSR_BT_HFG_DISCONNECT_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgDisconnectInd *)message)->connectionId);
        break;

        case CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_IND:
             instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAudioAcceptConnectInd *)message)->connectionId);
             break;

        case CSR_BT_HFG_AUDIO_CONNECT_CFM:
        case CSR_BT_HFG_AUDIO_CONNECT_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAudioConnectCfm *)message)->connectionId);
            break;

        case CSR_BT_HFG_AUDIO_DISCONNECT_CFM:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAudioDisconnectCfm *)message)->connectionId);
            break;

        case CSR_BT_HFG_AUDIO_DISCONNECT_IND:
             instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAudioDisconnectInd*)message)->connectionId);
             break;
                
        case CSR_BT_HFG_ANSWER_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAnswerInd*)message)->connectionId);
            break;
        
        case CSR_BT_HFG_REJECT_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgRejectInd *)message)->connectionId);
            break;
        
        case CSR_BT_HFG_NOISE_ECHO_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgNoiseEchoInd*)message)->connectionId);
            break;
        
        case CSR_BT_HFG_CALL_HANDLING_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgCallHandlingInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_RING_CFM:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgRingCfm *)message)->connectionId);
            break;
        
        case CSR_BT_HFG_STATUS_AUDIO_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgStatusAudioInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_SELECTED_CODEC_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgSelectedCodecInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_AT_CMD_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgAtCmdInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_DIAL_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgDialInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_SPEAKER_GAIN_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgSpeakerGainInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_MIC_GAIN_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgMicGainInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_OPERATOR_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgOperatorInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_CALL_LIST_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgCallListInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_SUBSCRIBER_NUMBER_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgSubscriberNumberInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_GENERATE_DTMF_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgGenerateDtmfInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_BT_INPUT_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgBtInputInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_VOICE_RECOG_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgVoiceRecogInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_MANUAL_INDICATOR_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgManualIndicatorInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_HF_INDICATOR_VALUE_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgHfIndicatorValueInd *)message)->connectionId);
            break;

        case CSR_BT_HFG_ENHANCED_VOICE_RECOG_IND:
            instance = aghfpProfileAbstract_GetInstanceForConnId(((CsrBtHfgEnhancedVoiceRecogInd *)message)->connectionId);
            break;

        default:
             DEBUG_LOG("Unhandled HFG Primitive: 0x%04X", id);
             break;

    }
    return  instance;
}



