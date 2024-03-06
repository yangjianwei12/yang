/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy specific HFP functionality port.
*/

#include <message.h>

#include "hfp_profile_battery_level.h"
#include "hfp_profile_config.h"
#include "hfp_profile_port.h"
#include "hfp_profile_private.h"
#include "hfp_profile_instance.h"
#include "hfp_profile_sm.h"
#include "hfp_profile_states.h"

#include <hf_lib.h>
#include <cm_lib.h>
#include <device_list.h>

#include <bt_device.h>
#include <device_properties.h>
#include <logging.h>
#include <mirror_profile.h>
#include <panic.h>
#include <ps.h>
#include <ps_key_map.h>
#include <stream.h>
#include <sink.h>
#include <telephony_messages.h>
#include <volume_messages.h>

/* Max no of profile activation required */
#define HFP_MAX_HF_CONNECTION (2)

/*! Max no of simultaneous connections can be established */
#define HFP_MAX_SIMULTANEOUS_CONNECTION HFP_MAX_HF_CONNECTION

/*! Max no of HS connections can be established */
#define HFP_MAX_HS_CONNECTION (0)

/*! HFP local supported feature  */
#ifdef DISABLE_HF_INDICATORS
/* HFP local supported feature without HF Indicators support */
#define HFP_LOCAL_SUPPORTED_FEATURES    (CSR_BT_HF_SUPPORT_EC_NR_FUNCTION                 | \
                                         CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING | \
                                         CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS           | \
                                         CSR_BT_HF_SUPPORT_VOICE_RECOGNITION              | \
                                         CSR_BT_HF_SUPPORT_REMOTE_VOLUME_CONTROL          | \
                                         CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION              | \
                                         CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS)
#else
/* HFP local supported feature with HF Indicators support */
#define HFP_LOCAL_SUPPORTED_FEATURES    (CSR_BT_HF_SUPPORT_EC_NR_FUNCTION                 | \
                                         CSR_BT_HF_SUPPORT_CALL_WAITING_THREE_WAY_CALLING | \
                                         CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS           | \
                                         CSR_BT_HF_SUPPORT_VOICE_RECOGNITION              | \
                                         CSR_BT_HF_SUPPORT_REMOTE_VOLUME_CONTROL          | \
                                         CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION              | \
                                         CSR_BT_HF_SUPPORT_HF_INDICATORS                  | \
                                         CSR_BT_HF_SUPPORT_ESCO_S4_T2_SETTINGS)
#endif

/*! AT response wait time */
#define HFP_AT_RESPONSE_TIME             CSR_BT_AT_DEFAULT_RESPONSE_TIME

#define HFP_SUPPORTED_HF_INDICATORS     ()

/*! \brief Hfp Indicator Support */
#define HFP_ENHANCED_SAFETY_HF_IND_INDEX (0)
#define HFP_BATTERY_LEVEL_HF_IND_INDEX   (1)
#define HFP_INDICATORS_COUNT             (2)


static bool hfpProfile_ParseCindIndicators(hfpInstanceTaskData* instance, const char *ind_str, uint8 value);
static void hfpProfile_HandleCindIndicators(hfpInstanceTaskData* instance, const CsrBtHfServiceConnectInd *ind);
static void hfpProfile_SendBatteryLevel(hfpInstanceTaskData* instance, const CsrBtHfServiceConnectInd *ind);

static void hfpProfile_HandleCmEncryptChangeInd(CsrBtCmEncryptChangeInd *ind)
{
    tp_bdaddr bd_addr = { 0 };
    hfpInstanceTaskData *instance;

    DEBUG_LOG("hfpProfile_HandleCmEncryptChangeInd: encrypted(%d)", ind->encryptType);

    bd_addr.transport = (TRANSPORT_T)ind->transportType;
    bd_addr.taddr.type = ind->deviceAddrType;
    BdaddrConvertBluestackToVm(&bd_addr.taddr.addr, &ind->deviceAddr);
    instance = HfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);

    if (instance && appDeviceIsHandset(&bd_addr.taddr.addr))
    {
        bool encrypted = (ind->encryptType != CSR_BT_CM_ENC_TYPE_NONE);

        switch (appHfpGetState(instance))
        {
            case HFP_STATE_CONNECTING_LOCAL:
            case HFP_STATE_CONNECTED_IDLE:
            case HFP_STATE_CONNECTED_OUTGOING:
            case HFP_STATE_CONNECTED_INCOMING:
            case HFP_STATE_CONNECTED_ACTIVE:
            case HFP_STATE_DISCONNECTING:
            {
                /* Store encrypted status */
                instance->bitfields.encrypted = encrypted;

                /* Check if SCO is now encrypted (or not) */
                HfpProfile_CheckEncryptedSco(instance);
            }
                return;

            default:
                HfpProfile_HandleError(instance, CSR_BT_CM_ENCRYPT_CHANGE_IND, NULL);
                return;
        }
    }
}

static bool hfpProfile_HandleCmMessage(Message message)
{
    bool handled = TRUE;
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    DEBUG_LOG("hfpProfile_HandleCmMessage: MESSAGE:CsrBtCmPrim:0x%04X", *prim);

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_MASK_CFM:
        break;

        case CSR_BT_CM_ENCRYPT_CHANGE_IND:
            hfpProfile_HandleCmEncryptChangeInd((CsrBtCmEncryptChangeInd *) message);
        break;

        default:
            DEBUG_LOG("hfpProfile_HandleCmMessage: Unexpected CM primitive");
            handled = FALSE;
        break;
    }

    CmFreeUpstreamMessageContents((void *) message);
    
    return handled;
}

/*! \brief Find HFP instance using connection Id

    This function returns the HFP instance from the existing device record for the matching
    connection id as requested.

    \param  conn_id Connection Id received from library
*/
static hfpInstanceTaskData * HfpProfileInstance_GetInstanceForConnId(CsrBtHfConnectionId conn_id)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    unsigned index;
    deviceType type = DEVICE_TYPE_HANDSET;
    hfpInstanceTaskData* hf_instance = NULL;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);

    for(index = 0; index < num_devices; index++)
    {
        hf_instance = HfpProfileInstance_GetInstanceForDevice(devices[index]);

        if (hf_instance && hf_instance->connection_id == conn_id)
        {
            break;
        }
        /* for the next iteration */
        hf_instance = NULL;
    }

    free(devices);
    return hf_instance;
}

void hfpProfile_InitHfpLibrary(void)
{
    CsrBtHfpHfIndicatorId *hfIndicators = (CsrBtHfpHfIndicatorId *) CsrPmemZalloc(HFP_INDICATORS_COUNT * sizeof(CsrBtHfpHfIndicatorId));
#ifdef INCLUDE_SWB
     bool swb_codec = TRUE;
#endif

    hfIndicators[HFP_ENHANCED_SAFETY_HF_IND_INDEX] = CSR_BT_HFP_ENHANCED_SAFETY_HF_IND;

    if (appConfigHfpBatteryIndicatorEnabled())
    {
        hfIndicators[HFP_BATTERY_LEVEL_HF_IND_INDEX] = HF_PROFILE_HF_BATTERY_LEVEL;
    }

#ifdef INCLUDE_SWB_LC3
    /* requesting HFP lib to add LC3SWB in HF supported codec list */
    HfUpdateSupportedCodecReqSend(CSR_BT_WBS_LC3SWB_CODEC_MASK, TRUE, FALSE);
#endif

    HfActivateReqSend(&hfp_profile_task_data.task,     /* Trap Task */
                      HFP_MAX_HF_CONNECTION,           /* Max HF Connections */
                      HFP_MAX_HS_CONNECTION,           /* Max HS Connections  */
                      HFP_MAX_SIMULTANEOUS_CONNECTION, /* Max Simultaneous Connections */
                      HFP_LOCAL_SUPPORTED_FEATURES,    /* Local Supported features */
                      CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CLIP_ACTIVATION |
                      CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION |
                      CSR_BT_HF_CNF_DISABLE_OUT_SERVICE_NAME_SEARCH |
                      CSR_BT_HF_CNF_DISABLE_OUT_NETWORK_SEARCH |
                      CSR_BT_HF_CNF_DISABLE_INC_SERVICE_NAME_SEARCH |
                      CSR_BT_HF_CNF_DISABLE_INC_NETWORK_SEARCH,
                      HFP_AT_RESPONSE_TIME,            /* AT response time in secs */
                      hfIndicators,                    /* HFP local supported HF indicators */
                      HFP_INDICATORS_COUNT);           /* No of HF indicators */

#ifdef TEST_HFP_CODEC_PSKEY
    uint16 hfp_codec_pskey = 0xffff;    /* enable all codecs by default */
    PsRetrieve(PS_KEY_TEST_HFP_CODEC, &hfp_codec_pskey, sizeof(hfp_codec_pskey));

    DEBUG_LOG_ALWAYS("hfpProfile_InitHfpLibrary 0x%x", hfp_codec_pskey);

    /* NB and WB are always enabled, NB cannot be disabled. WB can be disabled. */
    if((hfp_codec_pskey & HFP_CODEC_PS_BIT_WB) == 0)
    {
        HfUpdateSupportedCodecReqSend(CSR_BT_WBS_MSBC_CODEC_MASK, FALSE, FALSE);
    }
#ifdef INCLUDE_SWB
    swb_codec = ((hfp_codec_pskey & HFP_CODEC_PS_BIT_SWB)? TRUE : FALSE);
#endif
#endif

#ifdef INCLUDE_SWB
    if (appConfigScoSwbEnabled())
    {
        HfUpdateQceSupportReqSend(CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3, swb_codec);
    }
#endif
}

static void hfpProfile_HandleHfActivateCfm(const CsrBtHfActivateCfm *cfm)
{
    DEBUG_LOG("hfpProfile_HandleHfActivateCfm: result(0x%04x) supplier(0x%04x)", cfm->resultCode, cfm->resultSupplier);

    /* Check HFP initialisation was successful */
    hfpProfile_HandleInitComplete(cfm->resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS);

    /* Subscribe for Encryption change ind */
    CmSetEventMaskReqSend(&hfp_profile_task_data.task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
}

static void hfpProfile_HandleHfServiceConnectInd(const CsrBtHfServiceConnectInd *ind)
{
    bdaddr bd_addr = {0};
    hfpInstanceTaskData* instance;

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);

    instance = HfpProfileInstance_GetInstanceForBdaddr((const bdaddr *)&bd_addr);

    if (instance == NULL)
    {
        if (ind->resultCode != CSR_BT_RESULT_CODE_HF_SUCCESS && BtDevice_GetDeviceForBdAddr((const bdaddr*)&bd_addr) == NULL)
        {
            /* connection attempt has failed and the device is not on the PDL - ignore the connection attempt */
            return;
        }
        instance = HfpProfileInstance_Create(&bd_addr, TRUE);
    }

    hfpState state = appHfpGetState(instance);

    DEBUG_LOG("appHfpHandleHfServiceConnectInd(%p) result(0x%04x) supplier(0x%04x) connectionId(%d)",
                instance, ind->resultCode, ind->resultSupplier, ind->connectionId);

    if (HfpProfile_StateIsSlcDisconnected(state) || HfpProfile_StateIsSlcConnecting(state))
    {
        /* Check if SLC connection was successful */
        if (ind->resultCode == CSR_BT_RESULT_CODE_HF_SUCCESS)
        {
            instance->connection_id = ind->connectionId;

            /* Clear detach pending flag */
            instance->bitfields.detach_pending = FALSE;

            /* Store SLC sink */
            instance->profile = (ind->connectionType == CSR_BT_HF_CONNECTION_HF) ? hfp_handsfree_profile : hfp_headset_profile;

            /* Store remote supported features */
            instance->hfg_supported_features = ind->supportedFeatures;

            /* Store SLC sink */
            instance->slc_sink = StreamRfcommSink((uint16)(CSR_BT_CONN_ID_GET_MASK & ind->connectionId));

            /* Store BD address of AG device */
            //hfpProfile_CopyAddr(&ind->deviceAddr, &(instance->ag_bd_addr), FALSE);

            /* Check for in-band ringtone support */
            if (instance->hfg_supported_features & CSR_BT_HFG_SUPPORT_INBAND_RINGING)
            {
                instance->bitfields.in_band_ring  = TRUE;
            }

            /* Handle CIND indicators and parse the values */
            hfpProfile_HandleCindIndicators(instance, ind);

            /* Go for post HF connection setting */
            if (instance->profile == hfp_handsfree_profile)
            {
                HfSetEchoAndNoiseReqSend(ind->connectionId, FALSE);
                /* Disable Extended Audio Gateway Error Result Code at AG */
                HfSetExtendedAgErrorResultCodeReqSend(ind->connectionId, FALSE);
                /* Send current battery level to AG */
                hfpProfile_SendBatteryLevel(instance, ind);
                /* Read current Respose and Hold status from AG */
                HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_BTRH_READ_STATUS);
                instance->pending_call_command = CSR_BT_BTRH_READ_STATUS;
                /* Get current call list from AG */
                if ((HFP_LOCAL_SUPPORTED_FEATURES & CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS) &&
                    (instance->hfg_supported_features & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_STATUS))
                {
                    HfGetCurrentCallListReqSend(instance->connection_id);
                }
            }

            hfpProfile_HandleConnectCompleteSuccess(instance);
        }
        else
        {
            hfpProfile_HandleConnectCompleteFailed(instance, (ind->resultCode == CSR_BT_RESULT_CODE_HF_SDC_SEARCH_FAILED &&
                                                              ind->resultSupplier == CSR_BT_SUPPLIER_HF));
        }
    }
    else if (HfpProfile_StateIsSlcConnected(state))
    {
        /* SLC connect attempt has failed, this happens when there is already an ongoing connection in 
         * progress with the same remote device. This is a cross over scenario where our local attempt 
         * to connect has been failed (e.g. due to library rejection or RFCOMM connection failure).
         * Here we need to ignore this as we are already in connected state which means either 
         * incoming or outgoing request had been successful.
         */
        DEBUG_LOG("appHfpHandleHfServiceConnectInd: ignored - current state:enum:hfpState: %d", state);
    }
    else
    {
        HfpProfile_HandleError(instance, CSR_BT_HF_SERVICE_CONNECT_CFM, ind);
    }
}

/*! \brief Handle SLC disconnect indication
*/
static void hfpProfile_HandleHfDisconnectInd(const CsrBtHfDisconnectInd *ind)
{
    appHfpDisconnectReason reason = APP_HFP_DISCONNECT_NORMAL;
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    PanicNull(instance);

    hfpState state = appHfpGetState(instance);

    DEBUG_LOG("hfpProfile_HandleHfDisconnectInd(%p) state enum:hfpState:(%d) result(0x%04x) supplier(0x%04x) connectionId(%d)",
                instance, state, ind->reasonCode, ind->reasonSupplier, ind->connectionId);

    switch(ind->reasonCode)
    {
        case L2CA_DISCONNECT_LINK_LOSS:
            if (ind->reasonSupplier == CSR_BT_SUPPLIER_L2CAP_DISCONNECT)
            {
                reason = APP_HFP_DISCONNECT_LINKLOSS;
            }
        break;

        case L2CA_DISCONNECT_LINK_TRANSFERRED:
        case RFC_LINK_TRANSFERRED:
            if ((ind->reasonSupplier == CSR_BT_SUPPLIER_L2CAP_DISCONNECT) ||
                (ind->reasonSupplier == CSR_BT_SUPPLIER_RFCOMM))
            {
                reason = APP_HFP_DISCONNECT_TRANSFERRED;
            }
        break;

        default:
            reason = APP_HFP_DISCONNECT_NORMAL;
        break;
    }

    hfpProfile_HandleDisconnectComplete(instance, reason);
}

static void hfpProfile_HandleHfSelectedCodecInd(const CsrBtHfSelectedCodecInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    DEBUG_LOG("hfpProfile_HandleHfSelectedCodecInd(%p) codecToUse(%d)", instance, ind->codecToUse);

    PanicNull(instance);

    instance->codec = (uint8)ind->codecToUse;
}

static void hfpProfile_HandleHfAudioAcceptConnectInd(const CsrBtHfAudioAcceptConnectInd *ind)
{
    hfpInstanceTaskData * instance;
    bdaddr bd_addr = {0};
    CsrBtDeviceAddr device_addr = {0};

    DEBUG_LOG_FN_ENTRY("hfpProfile_HandleHfAudioAcceptConnectInd: linkType(%d) connectionId(%d)", ind->linkType, ind->connectionId);

    /* We need to fetch the hfp instance by bd address, as in cases when the connection is outgoing,
     * the connectionId may not be present if CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND comes before
     * CSR_BT_HF_SERVICE_CONNECT_CFM. */
    PanicFalse(HfGetBdAddrFromConnectionIdReq(ind->connectionId, &device_addr) == CSR_BT_RESULT_CODE_HF_SUCCESS);
    BdaddrConvertBluestackToVm(&bd_addr, &device_addr);
    instance = HfpProfileInstance_GetInstanceForBdaddr(&bd_addr);

    if (!instance)
    {
        /* This may happen if the connection is initiated by the remote device and we have received
         * incoming SCO connection even before receiving the SLC connection completion.
         * This is done for the IOP reason as there are certain phones in market which are sending
         * codec negotiation before completing the SLC connection. This leads to audio connection
         * request being received even before SLC connection complete gets received. */
        instance = HfpProfileInstance_Create(&bd_addr, TRUE);

        PanicNull(instance);

        appHfpSetState(instance, HFP_STATE_CONNECTING_REMOTE);
        instance->connection_id = ind->connectionId;
    }
    else
    {
        if(instance->connection_id == 0xFFFF)
        {
            /* This will happen if SLC is not yet established but HfpProfile instance had been created
             * for an outgoing connection. Eventually SLC connection will complete and we will get the connectionID,
             * but meanwhile we do not want to respond to AudioAcceptConnectInd with an invalid Connection ID. */
            instance->connection_id = ind->connectionId;
        }
    }
    
    hfpProfile_HandleHfpAudioConnectIncoming(instance, FALSE);
}

void hfpProfile_SendAudioConnectResponse(hfpInstanceTaskData * instance, bool is_esco, bool accept)
{
    UNUSED(is_esco);
    
    if(accept)
    {
        HfAudioAcceptResSend(instance->connection_id, HCI_SUCCESS, NULL, CSR_BT_PCM_DONT_CARE, FALSE);
    }
    else
    {
        HfAudioAcceptResSend(instance->connection_id, HCI_ERROR_SCO_INTERVAL_REJECTED, NULL, CSR_BT_PCM_DONT_CARE, FALSE);
    }
}

/*! \brief Handle SCO Audio connect confirmation or indication
*/
static void hfpProfile_HandleHfAudioConnectInd(const CsrBtHfAudioConnectInd *ind)
{
    hfp_profile_audio_connect_status_t status = hfp_profile_audio_connect_failed;
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    PanicNull(instance);

    hfpState state = appHfpGetState(instance);

    DEBUG_LOG("hfpProfile_HandleHfAudioConnectInd(%p): enum:hfpState:%d result(%04x) supplier(%04x) connectionId(%d)",
               instance, state, ind->resultCode, ind->resultSupplier, ind->connectionId);
               
    HfpProfileInstance_ClearAudioLock(instance);

    switch(ind->resultCode)
    {
        case CSR_BT_RESULT_CODE_HF_SUCCESS:
            if(HfpProfile_StateIsSlcConnected(state) || HfpProfile_StateIsSlcTransition(state))
            {
                /* Store sco_handle for later usage (at the time of audio disconnection) */
                instance->sco_handle = ind->scoHandle;
            }
            status = hfp_profile_audio_connect_success;
        break;

        case CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_ALREADY_CONNECTING:
        case CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED:
            if (ind->resultSupplier == CSR_BT_SUPPLIER_HF)
            {
                status = hfp_profile_audio_connect_in_progress;
            }
        break;

        default:
            status = hfp_profile_audio_connect_failed;
        break;
    }

    hfpProfile_HandleHfpAudioConnectComplete(instance, status, StreamScoSink(ind->scoHandle), instance->codec, ind->weSco, ind->txInterval, ind->qceCodecId);
}

/*! \brief Handle SCO Audio disconnect indication
*/
static void hfpProfile_HandleHfAudioDisconnectInd(const CsrBtHfAudioDisconnectInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpState state = appHfpGetState(instance);

    DEBUG_LOG("hfpProfile_HandleHfAudioDisconnectInd(%p) enum:hfpState:%d result(%04x) supplier(%04x) connectionId(%d)",
               instance, state, ind->reasonCode, ind->reasonSupplier, ind->connectionId);

    hfpProfile_HandleAudioDisconnectIndication(instance, (ind->reasonCode == RFC_LINK_TRANSFERRED &&
                                                          ind->reasonSupplier == CSR_BT_SUPPLIER_RFCOMM));
}

/*! \brief Send battery level to AG if battery indicator is enabled
*/
static void hfpProfile_SendBatteryLevel(hfpInstanceTaskData* instance, const CsrBtHfServiceConnectInd *ind)
{
    /* Enable Battery HF indicator if supported */
    if (ind->hfgSupportedHfIndicators != NULL)
    {
        uint8 count = 0;

        while (count < ind->hfgSupportedHfIndicatorsCount)
        {
            if (ind->hfgSupportedHfIndicators[count].hfIndicatorID == CSR_BT_HFP_BATTERY_LEVEL_HF_IND)
            {
                HfpProfile_EnableBatteryHfInd(instance, ind->hfgSupportedHfIndicators[count].status);
                break;
            }
            count++;
        }
    }
}

static void hfpProfile_HandleHfpCallAnswerCfm(const CsrBtHfCallAnswerCfm *cfm)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(cfm->connectionId);
    hfpProfile_HandleHfpCallAnswerComplete(instance, (cfm->cmeeResultCode == CSR_BT_CME_SUCCESS));
}

static void hfpProfile_HandleHfpCallTerminateCfm(const CsrBtHfCallEndCfm *cfm)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(cfm->connectionId);
    hfpProfile_HandleHfpCallTerminateComplete(instance, (cfm->cmeeResultCode == CSR_BT_CME_SUCCESS));
}

static void hfpProfile_HandleHfpCallHandlingCfm(const CsrBtHfCallHandlingCfm* cfm)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(cfm->connectionId);
    
    if(instance->pending_call_command == CSR_BT_BTRH_READ_STATUS)
    {
        /* Synergy combines CsrBtHfCallHandlingCfm for BTRH and CHLD. Ignore BTRH. */
        DEBUG_LOG("hfpProfile_HandleHfpCallHandlingCfm ignored for CSR_BT_BTRH_READ_STATUS");
    }
    else
    {
        hfpProfile_HandleHfpCallHoldActionComplete(instance, (cfm->cmeeResultCode == CSR_BT_CME_SUCCESS));
    }
    instance->pending_call_command = (CsrBtCallHandlingCommand)0xFF;
}

/*! \brief Handle Ring indication
*/
static void hfpProfile_HandleHfCallRingingInd(const CsrBtHfCallRingingInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpProfile_HandleRingIndication(instance, instance->bitfields.in_band_ring);
}

static void hfpProfile_HandleHfInbandRingSettingChangedInd(const CsrBtHfInbandRingSettingChangedInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    DEBUG_LOG("hfpProfile_HandleHfInbandRingSettingChangedInd(%p) in_band:%d", instance, ind->inbandRingingActivated);

    instance->bitfields.in_band_ring = (bool)ind->inbandRingingActivated;
}


/*! \brief Handle call state indication
*/
static void hfpProfile_HandleHfStatusIndicatorUpdateInd(const CsrBtHfStatusIndicatorUpdateInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    PanicNull(instance);

    /* Store call setup indication */
    hfpProfile_ParseCindIndicators(instance, (const char*)ind->name, ind->value);
    hfpProfile_HandleHfpCallStateIndication(instance);
}

/*! \brief Handle voice recognition indication
*/
static void hfpProfile_HandleHfSetVoiceRecognitionInd(CsrBtHfSetVoiceRecognitionInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpProfile_HandleHfpVoiceRecognitionIndication(instance, ind->started);
}

/*! \brief Handle voice recognition enable confirmation
*/
static void hfpProfile_HandleHfSetVoiceRecognitionCfm(CsrBtHfSetVoiceRecognitionCfm * cfm)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(cfm->connectionId);

    PanicNull(instance);

    hfpState state = appHfpGetState(instance);

    DEBUG_LOG("hfpProfile_HandleHfSetVoiceRecognitionCfm(%p) enum:hfpState:%d status:%d", instance, state, cfm->cmeeResultCode);

    hfpProfile_HandleHfpVoiceRecognitionEnableComplete(instance, (cfm->cmeeResultCode == CSR_BT_CME_SUCCESS));
}

static CsrCharString * hfpProfile_GetCallerInfoFromClip(CsrCharString * clip_string, phone_number_type_t* number_type)
{
    CsrCharString * caller_number = NULL;
    CsrCharString *next_str_token = NULL;

    /* Parse +CLIP string into its constituent parts. First, obtain the caller number. If this is vaild,
       then read the number type. */
    if (clip_string != NULL)
    {
        clip_string = strchr(clip_string, '"');
        caller_number = CsrStringToken(clip_string, "\"", &next_str_token);
    }
    if (caller_number != NULL)
    {
        *number_type = number_unknown;

        /* Ignore all but the type field, which is the next numeric value in the clip_string. CsrStrToInt
           works backwards from the end of the string passed, so we need to ensure the number_type is the
           only numeric value in the string it is passed. */
        CsrCharString *remaining_clip_string = next_str_token;
        CsrCharString *number_type_string = CsrStringToken(NULL, ", \"", &next_str_token);
        if (number_type_string == NULL)
        {
            number_type_string = remaining_clip_string;
        }
        if (number_type_string != NULL)
        {
            uint8 hfp_profile_encoded_number_type = CsrStrToInt(number_type_string);
            if (hfp_profile_encoded_number_type >= 144 &&
                hfp_profile_encoded_number_type <= 159)
            {
                *number_type = number_international;
            }
            else if (hfp_profile_encoded_number_type >= 160 &&
                     hfp_profile_encoded_number_type <= 175)
            {
                *number_type = number_national;
            }
        }
    }
    
    return caller_number;
}

/*! \brief Handle caller ID indication
*/
static void hfpProfile_HandleHfpCallerIdInd(const CsrBtHfCallNotificationInd *ind)
{
    phone_number_type_t type;
    CsrCharString * caller_number = hfpProfile_GetCallerInfoFromClip(ind->clipString, &type);
    
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    
    hfpProfile_HandleHfpCallerIdIndication(instance, caller_number, type, NULL);
}

/*! \brief Handle volume indication
*/
static void hfpProfile_HandleHfSpeakerGainInd(CsrBtHfSpeakerGainInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpProfile_HandleHfpVolumeSyncSpeakerGainIndication(instance, ind->gain);
}

/*! \brief Handle microphone volume indication
*/
static void hfpProfile_HandleHfMicGainInd(CsrBtHfMicGainInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpProfile_HandleHfpVolumeSyncMicGainIndication(instance, ind->gain);
}

/*! \brief Handle unrecognised AT commands as TWS+ custom commands.
 */
static void hfpProfile_HandleHfUnrecognisedAtCmdInd(CsrBtHfAtCmdInd* ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    
    uint16 str_size = strlen(ind->atCmdString)+1;
    
    hfpProfile_HandleHfpUnrecognisedAtCmdIndication(instance, (uint8*)ind->atCmdString, str_size);
}

static void hfpProfile_HandleHfIndicatorStatusInd(CsrBtHfHfIndicatorStatusInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);

    DEBUG_LOG("hfpProfile_HandleHfIndicatorStatusInd(%p), num %u, status %u", instance, ind->indId, ind->status);

    PanicNull(instance);

    if (ind->indId == HF_PROFILE_HF_BATTERY_LEVEL)
    {
        HfpProfile_EnableBatteryHfInd(instance, ind->status);
    }
}

static void hfpProfile_HandleHfpAtCmdConfirm(CsrBtHfAtCmdCfm *cfm)
{
    DEBUG_LOG("hfpProfile_HandleHfpAtCmdConfirm status:%d", cfm->cmeeResultCode);
    hfpProfile_HandleHfpAtCmdComplete(cfm->cmeeResultCode == CSR_BT_CME_SUCCESS);
}

/*! \brief Copy Synergy BD address to ADK address type or vice-versa
*/
static void hfpProfile_CopyAddr(CsrBtDeviceAddr *syn_addr, bdaddr *adk_addr, bool adk_to_syn)
{
    if (adk_to_syn)
    {
        syn_addr->lap = adk_addr->lap;
        syn_addr->uap = adk_addr->uap;
        syn_addr->nap = adk_addr->nap;
    }
    else
    {
        adk_addr->lap = syn_addr->lap;
        adk_addr->uap = syn_addr->uap;
        adk_addr->nap = syn_addr->nap;
    }
}

/*! \brief Parse Call indicator and set call state accordingly
*/
static void hfpProfile_ParseCallIndicator(hfpInstanceTaskData* instance, uint8 call)
{
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCallIndicator(%p) enum:hfp_call_state:%d call:%d", instance, instance->bitfields.call_state, call);

    if (call == CSR_BT_CALL_ACTIVE_VALUE)
    {
        switch (instance->bitfields.call_state)
        {
            case hfp_call_state_idle:
            case hfp_call_state_incoming:
            case hfp_call_state_outgoing:
                /* Valid to go to active call state */
                instance->bitfields.call_state = hfp_call_state_active;
            break;
            default:
                /* Ignore */
            break;
        }
    }
    else
    {
        switch (instance->bitfields.call_state)
        {
            case hfp_call_state_idle:
                instance->bitfields.call_state = hfp_call_state_idle;
            break;
            case hfp_call_state_twc_incoming:
                /* Drop back to incoming call state */
                instance->bitfields.call_state = hfp_call_state_incoming;
            break;
            case hfp_call_state_twc_outgoing:
                /* Drop back to outgoing call state */
                instance->bitfields.call_state = hfp_call_state_outgoing;
            break;
            default:
                /* In all other cases, drop back to idle state */
                instance->bitfields.call_state = hfp_call_state_idle;
            break;
        }
    }
}

/*! \brief Parse CallSetup indicator and set call state accordingly
*/
static void hfpProfile_ParseCallSetupIndicator(hfpInstanceTaskData* instance, uint8 call_setup)
{
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCallSetupIndicator(%p) enum:hfp_call_state:%d call_setup:%d", instance, instance->bitfields.call_state, call_setup);

    switch (call_setup)
    {
        case CSR_BT_NO_CALL_SETUP_VALUE:
            switch (instance->bitfields.call_state)
            {
                case hfp_call_state_incoming:
                case hfp_call_state_outgoing:
                    instance->bitfields.call_state = hfp_call_state_idle;
                break;
                case hfp_call_state_twc_incoming:
                    /* Try for CLCC to work out what happened to the incoming call */
                    if ((HFP_LOCAL_SUPPORTED_FEATURES & CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS) &&
                        (instance->hfg_supported_features & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_STATUS))
                    {
                        HfGetCurrentCallListReqSend(instance->connection_id);
                    }
                    /* Incoming TWC no longer there so assume it ended active for now */
                    instance->bitfields.call_state = hfp_call_state_active;
                break;
                case hfp_call_state_twc_outgoing:
                    /* In outgoing TWC call was already held, go to held call */
                    instance->bitfields.call_state = hfp_call_state_held_active;
                default:
                    /* Ignore */
                break;
            }
        break;

        case CSR_BT_INCOMING_CALL_SETUP_VALUE:
            switch (instance->bitfields.call_state)
            {
                case hfp_call_state_idle:
                    instance->bitfields.call_state = hfp_call_state_incoming;
                break;
                case hfp_call_state_active:
                case hfp_call_state_held_active:
                case hfp_call_state_held_remaining:
                case hfp_call_state_multiparty:
                    /* We have a second incoming call */
                    instance->bitfields.call_state = hfp_call_state_twc_incoming;
                break;
                default:
                    /* Ignore */
                break;
            }
        break;

        case CSR_BT_OUTGOING_CALL_SETUP_VALUE:
        case CSR_BT_OUTGOING_REMOTE_ALERT_VALUE:
            switch (instance->bitfields.call_state)
            {
                case hfp_call_state_idle:
                    instance->bitfields.call_state = hfp_call_state_outgoing;
                break;
                case hfp_call_state_active:
                case hfp_call_state_held_active:
                case hfp_call_state_held_remaining:
                    /* We have a second outgoing call */
                    instance->bitfields.call_state = hfp_call_state_twc_outgoing;
                break;
                default:
                    /* Ignore */
                break;
            }
        break;

        default:
            /* Error */
        break;
    }
}

/*! \brief Parse CallHeld indicator and set call state accordingly
*/
static void hfpProfile_ParseCallHeldIndicator(hfpInstanceTaskData* instance, uint8 call_held)
{
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCallHeldIndicator(%p) enum:hfp_call_state:%d call_held:%d", instance, instance->bitfields.call_state, call_held);

    switch (call_held)
    {
        case CSR_BT_NO_CALL_HELD_VALUE:
            switch(instance->bitfields.call_state)
            {
                case hfp_call_state_twc_outgoing:
                    /* When outgoing TWC starts the active call is put
                       on hold. If held call has ended then we have an
                       outgoing call left... */
                    instance->bitfields.call_state = hfp_call_state_outgoing;
                break;
                case hfp_call_state_held_remaining:
                    /* Remaining call unheld, assume active and call
                       none indication will take us back to idle */
                    instance->bitfields.call_state = hfp_call_state_active;
                break;
                case hfp_call_state_held_active:
                    /* Try for CLCC to work out what happened to the held call if ECS is supported */
                    if ((HFP_LOCAL_SUPPORTED_FEATURES & CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS) &&
                        (instance->hfg_supported_features & CSR_BT_HFG_SUPPORT_ENHANCED_CALL_STATUS))
                    {
                        HfGetCurrentCallListReqSend(instance->connection_id);
                    }

                    /* Held call no longer there so assume it was ended for now */
                    instance->bitfields.call_state = hfp_call_state_active;
                break;
                default:
                    /* Ignore */
                break;
            }
        break;

        case CSR_BT_CALL_HELD_RETRIEVE_OTHER_CALL_VALUE:
            switch(instance->bitfields.call_state)
            {
                case hfp_call_state_active:
                case hfp_call_state_twc_incoming:
                case hfp_call_state_held_remaining:
                case hfp_call_state_multiparty:
                    /* Enter active + held state */
                    instance->bitfields.call_state = hfp_call_state_held_active;
                break;
                default:
                    /* Ignore */
                break;
            }
        break;

        case CSR_BT_CALL_HELD_NO_ACTIVE_CALL_VALUE:
            switch(instance->bitfields.call_state)
            {
                case hfp_call_state_active:
                case hfp_call_state_twc_outgoing:
                case hfp_call_state_held_active:
                case hfp_call_state_multiparty:
                    instance->bitfields.call_state = hfp_call_state_held_remaining;
                break;
                default:
                    /* Ignore */
                break;
            }
        break;

        default:
            /* Error */
        break;
    }
}

/*! \brief Parse indicator and set call state accordingly
*/
static bool hfpProfile_ParseCindIndicators(hfpInstanceTaskData* instance, const char *ind_str, uint8 value)
{
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCindIndicators(%p) ind_value:%d", instance, value);

    if ((CsrStrCmp(ind_str, "call") == 0) || (CsrStrNCmp(ind_str, "\"call\"", 6) == 0))
    {
        hfpProfile_ParseCallIndicator(instance, value);
    }
    else if ((CsrStrCmp(ind_str, "callsetup") == 0) || (CsrStrNCmp(ind_str, "\"callsetup\"", 11) == 0))
    {
        hfpProfile_ParseCallSetupIndicator(instance, value);
    }
    else if ((CsrStrCmp(ind_str, "callheld") == 0) || (CsrStrNCmp(ind_str, "\"callheld\"", 10) == 0))
    {
        hfpProfile_ParseCallHeldIndicator(instance, value);
    }
    else if (((CsrStrCmp(ind_str, "service") == 0)  || (CsrStrNCmp(ind_str, "\"service\"", 9) == 0)) ||
             ((CsrStrCmp(ind_str, "signal") == 0)   || (CsrStrNCmp(ind_str, "\"signal\"", 8) == 0))  ||
             ((CsrStrCmp(ind_str, "roam") == 0)     || (CsrStrNCmp(ind_str, "\"roam\"", 6) == 0))    ||
             ((CsrStrCmp(ind_str, "battchg") == 0)  || (CsrStrNCmp(ind_str, "\"battchg\"", 9) == 0)))
    {
        /* Valid token received but nothing to do. So return TRUE */
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}

/*! \brief Parse CIND string and values.
*/
static void hfpProfile_HandleCindIndicators(hfpInstanceTaskData* instance, const CsrBtHfServiceConnectInd *ind)
{
    DEBUG_LOG("hfpProfile_HandleCindIndicators(%p)", instance);

    if (ind->indicatorSupported && ind->indicatorValue)
    {
        CsrCharString *next_str_token = NULL;
        CsrCharString *next_val_token = NULL;
        CsrCharString *token_str = CsrStringToken(ind->indicatorSupported, "\"", &next_str_token);
        CsrCharString *token_val = CsrStringToken(ind->indicatorValue, ",", &next_val_token);

        while (token_str && token_val)
        {
            /* Retrieve next value token only if current str token has valid indicator */
            if (hfpProfile_ParseCindIndicators(instance, (const char*)token_str, (uint8)(CsrStrToInt(token_val))))
            {
                token_val = CsrStringToken(NULL, ",", &next_val_token);
            }
            /* get next indicator string token */
            token_str = CsrStringToken(NULL, "\"", &next_str_token);
        }
    }
}

/*! \brief Parse CLCC atTextString received from AG. If parsed fine then update the HFP Call state
*/
static bool hfpProfile_ParseCurrentCallListInd(hfpInstanceTaskData* instance, CsrBtHfGetCurrentCallListInd *ind)
{
    const CsrCharString *delim = ",";
    CsrCharString *next_token = NULL;
    CsrCharString *clcc_str = (CsrCharString *)CsrStrDup((CsrCharString *)ind->clccString);
    CsrCharString *token;
    uint16 call_idx;
    hfp_call_direction dir;
    hfp_call_status status;
    hfp_call_mode mode;
    hfp_call_multiparty multiparty;

    DEBUG_LOG("hfpProfile_ParseCurrentCallListInd(%p)", instance);

    /* Fetch call index */
    token = CsrStringToken(clcc_str, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(clcc_str);
        return FALSE;
    }
    call_idx = (uint8)(CsrStrToInt(token));
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCurrentCallListInd call_idx:%d", call_idx);

    /* Fetch call direction */
    token = CsrStringToken(NULL, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(clcc_str);
        return FALSE;
    }
    dir = (uint8)(CsrStrToInt(token));
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCurrentCallListInd enum:hfp_call_direction:%d", dir);

    /* Fetch call status */
    token = CsrStringToken(NULL, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(clcc_str);
        return FALSE;
    }
    status = (uint8)(CsrStrToInt(token));
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCurrentCallListInd enum:hfp_call_status:%d", status);

    /* Fetch call mode */
    token = CsrStringToken(NULL, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(clcc_str);
        return FALSE;
    }
    mode = (uint8)(CsrStrToInt(token));
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCurrentCallListInd enum:hfp_call_mode:%d", mode);

    /* Fetch call multiparty status */
    token = CsrStringToken(NULL, delim, &next_token);
    if (!(token))
    {
        CsrPmemFree(clcc_str);
        return FALSE;
    }
    multiparty = (uint8)(CsrStrToInt(token));
    DEBUG_LOG_VERBOSE("hfpProfile_ParseCurrentCallListInd enum:hfp_call_multiparty:%d", multiparty);

    CsrPmemFree(clcc_str);

    /* Validate parameters - ignore silently if any are invalid */
    if (dir        > hfp_call_mobile_terminated ||
        status     > hfp_call_waiting           ||
        mode       > hfp_call_fax               ||
        multiparty > hfp_multiparty_call)
    {
        return (FALSE);
    }

    /* Update the HFP call state if call is part of multiparty */
    if (multiparty == hfp_multiparty_call && instance->bitfields.call_state != hfp_call_state_multiparty)
    {
        instance->bitfields.call_state = hfp_call_state_multiparty;
        DEBUG_LOG("hfpProfile_ParseCurrentCallListInd New call state enum:hfp_call_state:%d", instance->bitfields.call_state);
    }

    return (TRUE);
}

static void hfpProfile_HandleHfGetCurrentCallListInd(CsrBtHfGetCurrentCallListInd *ind)
{
    hfpInstanceTaskData *instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpState current_state = appHfpGetState(instance);
    hfpState new_state;

    DEBUG_LOG("hfpProfile_HandleHfGetCurrentCallListInd(%p) enum:hfpState:%d", instance, current_state);

    PanicNull(instance);

    if ((HFP_LOCAL_SUPPORTED_FEATURES & CSR_BT_HF_SUPPORT_ENHANCED_CALL_STATUS) &&
        (hfpProfile_ParseCurrentCallListInd(instance, ind)))
    {
        /* Move to new state, depending on call state */
        new_state = hfpProfile_GetStateFromCallState(instance->bitfields.call_state);

        if (current_state != new_state)
            appHfpSetState(instance, new_state);
    }
    else
    {
        DEBUG_LOG("hfpProfile_HandleHfGetCurrentCallListInd: Could not parse !");
    }

    instance->pending_call_status_request = FALSE;
}

static void hfpProfile_HandleHfGetCurrentCallListCfm(CsrBtHfGetCurrentCallListCfm *cfm)
{
    hfpInstanceTaskData *instance = HfpProfileInstance_GetInstanceForConnId(cfm->connectionId);
    hfpState current_state = appHfpGetState(instance);
    hfpState new_state;

    DEBUG_LOG("hfpProfile_HandleHfGetCurrentCallListCfm(%p) enum:hfpState:%d", instance, current_state);

    if (instance != NULL)
    {
        if (instance->pending_call_status_request && cfm->cmeeResultCode == CSR_BT_CME_SUCCESS)
        {
            /* Move to Idle state, since no call in remote */
            new_state = HFP_STATE_CONNECTED_IDLE;

            if (current_state != new_state)
            {
                appHfpSetState(instance, new_state);
            }

            DEBUG_LOG("hfpProfile_HandleHfGetCurrentCallListCfm cleared the HF call state, since no call in remote");
            instance->pending_call_status_request = FALSE;
        }
    }
}

/*! \brief Handle Call Handling indication for Response Hold +BTRH indication from AG
*/
static void hfpProfile_HandleHfCallHandlingInd(const CsrBtHfCallHandlingInd *ind)
{
    hfpInstanceTaskData * instance = HfpProfileInstance_GetInstanceForConnId(ind->connectionId);
    hfpState current_state, new_state;

    PanicNull(instance);

    DEBUG_LOG_VERBOSE("hfpProfile_HandleHfCallHandlingInd(%p) enum:hfp_call_state:%d event:%d", instance, instance->bitfields.call_state, ind->event);

    current_state = appHfpGetState(instance);

    switch (ind->event)
    {
        case CSR_BT_BTRH_INCOMING_ON_HOLD:
            if (instance->bitfields.call_state == hfp_call_state_incoming)
            {
                instance->bitfields.call_state = hfp_call_state_incoming_held;
            }
        break;

        case CSR_BT_BTRH_INCOMING_ACCEPTED:
            if (instance->bitfields.call_state == hfp_call_state_incoming_held)
            {
                instance->bitfields.call_state = hfp_call_state_active;
            }
        break;

        case CSR_BT_BTRH_INCOMING_REJECTED:
            if (instance->bitfields.call_state == hfp_call_state_incoming_held)
            {
                instance->bitfields.call_state = hfp_call_state_idle;
            }
        break;

        default:
            /* Ignore */
        break;
    }

    /* Move to new state, depending on call state */
    new_state = hfpProfile_GetStateFromCallState(instance->bitfields.call_state);

    if (current_state != new_state)
        appHfpSetState(instance, new_state);
}

static bool hfpProfilePort_HandleHfMessages(Message message)
{
    bool handled = TRUE;
    CsrBtHfPrim* prim_type = (CsrBtHfPrim *)message;

    DEBUG_LOG("hfpProfilePort_HandleHfMessages MESSAGE:CsrBtHfPrim:0x%04X", *prim_type);

    switch(*prim_type)
    {
        /* Profile primitives */
        case CSR_BT_HF_AUDIO_DISCONNECT_CFM:
        case CSR_BT_HF_AUDIO_DISCONNECT_IND:
            hfpProfile_HandleHfAudioDisconnectInd((CsrBtHfAudioDisconnectInd *)message);
        break;
        
        case CSR_BT_HF_ACTIVATE_CFM:
             hfpProfile_HandleHfActivateCfm((CsrBtHfActivateCfm *)message);
             break;

        case CSR_BT_HF_SERVICE_CONNECT_CFM:
        case CSR_BT_HF_SERVICE_CONNECT_IND:
             hfpProfile_HandleHfServiceConnectInd((CsrBtHfServiceConnectInd *)message);
             break;

        case CSR_BT_HF_DISCONNECT_CFM:
        case CSR_BT_HF_DISCONNECT_IND:
             hfpProfile_HandleHfDisconnectInd((CsrBtHfDisconnectInd *)message);
             break;

        case CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND:
             hfpProfile_HandleHfAudioAcceptConnectInd((CsrBtHfAudioAcceptConnectInd *)message);
             break;

        case CSR_BT_HF_AUDIO_CONNECT_CFM:
        case CSR_BT_HF_AUDIO_CONNECT_IND:
             hfpProfile_HandleHfAudioConnectInd((CsrBtHfAudioConnectInd *)message);
             break;
        
        case CSR_BT_HF_CALL_ANSWER_CFM:
            hfpProfile_HandleHfpCallAnswerCfm((CsrBtHfCallAnswerCfm *)message);
        break;
        
        case CSR_BT_HF_CALL_END_CFM:
            hfpProfile_HandleHfpCallTerminateCfm((CsrBtHfCallEndCfm *)message);
        break;
        
        case CSR_BT_HF_CALL_HANDLING_CFM:
            hfpProfile_HandleHfpCallHandlingCfm((CsrBtHfCallHandlingCfm *)message);
        break;

        case CSR_BT_HF_CALL_RINGING_IND:
             hfpProfile_HandleHfCallRingingInd((CsrBtHfCallRingingInd *)message);
             break;

        case CSR_BT_HF_INBAND_RING_SETTING_CHANGED_IND:
             hfpProfile_HandleHfInbandRingSettingChangedInd((CsrBtHfInbandRingSettingChangedInd *)message);
             break;

        case CSR_BT_HF_STATUS_INDICATOR_UPDATE_IND:
             hfpProfile_HandleHfStatusIndicatorUpdateInd((CsrBtHfStatusIndicatorUpdateInd *)message);
             break;

        case CSR_BT_HF_SET_VOICE_RECOGNITION_IND:
             hfpProfile_HandleHfSetVoiceRecognitionInd((CsrBtHfSetVoiceRecognitionInd *)message);
             break;

        case CSR_BT_HF_SET_VOICE_RECOGNITION_CFM:
             hfpProfile_HandleHfSetVoiceRecognitionCfm((CsrBtHfSetVoiceRecognitionCfm *)message);
             break;

        case CSR_BT_HF_SPEAKER_GAIN_IND:
             hfpProfile_HandleHfSpeakerGainInd((CsrBtHfSpeakerGainInd *)message);
             break;

        case CSR_BT_HF_MIC_GAIN_IND:
             hfpProfile_HandleHfMicGainInd((CsrBtHfMicGainInd *)message);
             break;

        case CSR_BT_HF_AT_CMD_CFM:
             hfpProfile_HandleHfpAtCmdConfirm((CsrBtHfAtCmdCfm *)message);
             break;

        case CSR_BT_HF_AT_CMD_IND:
             hfpProfile_HandleHfUnrecognisedAtCmdInd((CsrBtHfAtCmdInd *)message);
             break;

        case CSR_BT_HF_HF_INDICATOR_STATUS_IND:
             hfpProfile_HandleHfIndicatorStatusInd((CsrBtHfHfIndicatorStatusInd *)message);
             break;

        case CSR_BT_HF_SELECTED_CODEC_IND:
             hfpProfile_HandleHfSelectedCodecInd((CsrBtHfSelectedCodecInd *)message);
             break;

        case CSR_BT_HF_CALL_NOTIFICATION_IND:
             hfpProfile_HandleHfpCallerIdInd((CsrBtHfCallNotificationInd *)message);
             break;

        case CSR_BT_HF_GET_CURRENT_CALL_LIST_IND:
             hfpProfile_HandleHfGetCurrentCallListInd((CsrBtHfGetCurrentCallListInd *)message);
             break;

         case CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM:
             hfpProfile_HandleHfGetCurrentCallListCfm((CsrBtHfGetCurrentCallListCfm *)message);
             break;

        case CSR_BT_HF_CALL_HANDLING_IND:
             hfpProfile_HandleHfCallHandlingInd((CsrBtHfCallHandlingInd *)message);
             break;

        case CSR_BT_HF_DIAL_CFM:
        case CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM:
        case CSR_BT_HF_SET_ECHO_AND_NOISE_CFM:
             break;

        default:
            handled = FALSE;
        break;
    }
    
    HfFreeUpstreamMessageContents((void *)message);
    
    return handled;
}

bool HfpProfilePort_HandleMessage(Task task, MessageId id, Message message)
{
    bool handled;
    
    UNUSED(task);
    
    switch(id)
    {
        case HF_PRIM:
            handled = hfpProfilePort_HandleHfMessages(message);
        break;
        
        case CM_PRIM:
            handled = hfpProfile_HandleCmMessage(message);
        break;

        default:
            handled = FALSE;
        break;
    }

    return handled;
}

void hfpProfile_InitialiseInstancePortSpecificData(hfpInstanceTaskData * instance)
{
    instance->slc_sink = NULL;
    instance->connection_id = 0xFFFF;
    instance->pending_call_command = (CsrBtCallHandlingCommand)0xFF;
}

void hfpProfile_CopyInstancePortSpecificData(hfpInstanceTaskData * target_instance, hfpInstanceTaskData * source_instance)
{
    target_instance->connection_id = source_instance->connection_id;
}

void hfpProfile_ConnectSlc(hfpInstanceTaskData* instance)
{
    /* convert ADK address to Synergy's address format */
    CsrBtDeviceAddr addr;
    hfpProfile_CopyAddr(&addr, &(instance->ag_bd_addr), TRUE);

    DEBUG_LOG("appHfpEnterConnectingLocal, Connecting HFP to AG (%x,%x,%lx)", instance->ag_bd_addr.nap, instance->ag_bd_addr.uap, instance->ag_bd_addr.lap);

    if (HfpProfile_IsHandsetBlockedForSwb(&(instance->ag_bd_addr)))
    {
        CmDisableSWBSupport(&addr);
    }

    /* Start HFP connection */
    HfServiceConnectReqSend(addr, (instance->profile == hfp_handsfree_profile ? CSR_BT_HF_CONNECTION_HF : CSR_BT_HF_CONNECTION_HS));
}

void hfpProfile_DisconnectSlc(hfpInstanceTaskData* instance)
{
    HfDisconnectReqSend(instance->connection_id);
}

void hfpProfile_ReadRemoteSupportedFeatures(hfpInstanceTaskData* instance)
{
    /* Not required for Synergy */
    UNUSED(instance);
}

void hfpProfile_SendBievCommandToInstance(hfpInstanceTaskData * instance, uint8 percent)
{
    HfSetHfIndicatorValueReqSend(instance->connection_id, HF_PROFILE_HF_BATTERY_LEVEL, percent);
    DEBUG_LOG_VERBOSE("hfpProfile_SendBievCommandToInstance sending %d percent to connection_id=%x",
            percent, instance->connection_id);
}

void hfpProfile_RefreshCallStateRequest(hfpInstanceTaskData* instance)
{
    HfGetCurrentCallListReqSend(instance->connection_id);
    instance->pending_call_status_request = TRUE;
}

void hfpProfile_CallerIdEnable(hfpInstanceTaskData* instance, bool enable)
{
    if (instance != NULL)
    {
        if (HFP_LOCAL_SUPPORTED_FEATURES & CSR_BT_HF_SUPPORT_CLI_PRESENTATION_CAPABILITY)
        {
            HfSetCallNotificationIndicationReqSend(instance->connection_id, enable);
        }
    }
}

void hfpProfile_LastNumberRedial(hfpInstanceTaskData* instance)
{
    if (instance->profile == hfp_headset_profile)
    {
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        HfDialReqSend(instance->connection_id, CSR_BT_HF_DIAL_REDIAL, NULL);
    }
}

void hfpProfile_VoiceDialEnable(hfpInstanceTaskData* instance)
{
    if (instance->profile == hfp_headset_profile)
    {
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        HfSetVoiceRecognitionReqSend(instance->connection_id, CSR_BT_HFP_VR_ENABLE);
    }
}

void hfpProfile_VoiceDialDisable(hfpInstanceTaskData* instance)
{
    if (instance->profile == hfp_headset_profile)
    {
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        HfSetVoiceRecognitionReqSend(instance->connection_id, CSR_BT_HFP_VR_DISABLE);
    }
}

void hfpProfile_DialNumber(hfpInstanceTaskData* instance, uint8* number, unsigned length_number)
{
    if (instance->profile == hfp_headset_profile)
    {
        /* Send button press */
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        const char suffix[] = {';','\r', '\0'};
        CsrCharString *number_string = CsrPmemAlloc(length_number + sizeof(suffix));

        if (number && length_number > 0)
        {
            /* copy the number digits to be dialled */
            CsrMemCpy(number_string, number, length_number);
            /* copy the suffix at the end of number */
            CsrMemCpy(number_string + length_number, suffix, sizeof(suffix));
            HfDialReqSend(instance->connection_id, CSR_BT_HF_DIAL_NUMBER, number_string);
        }
    }
}

void hfpProfile_AnswerCall(hfpInstanceTaskData* instance)
{
    if (instance->profile == hfp_headset_profile)
    {
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        HfAnswerReqSend(instance->connection_id);
    }
}

void hfpProfile_RejectCall(hfpInstanceTaskData* instance)
{
    HfCallEndReqSend(instance->connection_id);
}

void hfpProfile_TerminateCall(hfpInstanceTaskData* instance)
{
    if (instance->profile == hfp_headset_profile)
    {
        HfpProfileInstance_RequestAudioDisconnection(instance);
    }
    else
    {
        HfCallEndReqSend(instance->connection_id);
    }
}

void hfpProfile_SetMicrophoneGain(hfpInstanceTaskData *instance, uint8 gain)
{
    HfMicGainStatusReqSend(instance->connection_id, gain);
}

void hfpProfile_SetSpeakerGain(hfpInstanceTaskData *instance, uint8 gain)
{
    HfSpeakerGainStatusReqSend(instance->connection_id, gain);
}

void hfpProfile_HandleAudioTransferRequest(hfpInstanceTaskData *instance, voice_source_audio_transfer_direction_t direction)
{
    /* If direction is voice_source_audio_transfer_toggle, toggle the location at which audio is rendered.
     * If instance->sco_sink is not NULL, this means audio is currently activated at device
     * side so it should be transferred to AG otherwise transfer the audio to device.
     */
    if(direction == voice_source_audio_transfer_toggle)
    {
        direction = instance->sco_sink ? voice_source_audio_transfer_to_ag : voice_source_audio_transfer_to_hfp;
    }

    if(direction == voice_source_audio_transfer_to_hfp)
    {
        HfpProfileInstance_RequestAudioConnection(instance);
    }
    else
    {
        HfpProfileInstance_RequestAudioDisconnection(instance);
    }
}

void hfpProfile_ReleaseWaitingRejectIncoming(hfpInstanceTaskData* instance)
{
    HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_RELEASE_ALL_HELD_CALL);
    instance->pending_call_command = CSR_BT_RELEASE_ALL_HELD_CALL;
}

void hfpProfile_AcceptWaitingReleaseActive(hfpInstanceTaskData* instance)
{
    HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_RELEASE_ACTIVE_ACCEPT);
    instance->pending_call_command = CSR_BT_RELEASE_ACTIVE_ACCEPT;
}

void hfpProfile_AcceptWaitingHoldActive(hfpInstanceTaskData* instance)
{
    HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_HOLD_ACTIVE_ACCEPT);
    instance->pending_call_command = CSR_BT_HOLD_ACTIVE_ACCEPT;
}

void hfpProfile_AddHeldToMultiparty(hfpInstanceTaskData* instance)
{
    HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_ADD_CALL);
    instance->pending_call_command = CSR_BT_ADD_CALL;
}

void hfpProfile_JoinCallsAndHangUp(hfpInstanceTaskData* instance)
{
    HfCallHandlingReqSend(instance->connection_id, 0, CSR_BT_CONNECT_TWO_CALLS);
    instance->pending_call_command = CSR_BT_CONNECT_TWO_CALLS;
}

#ifdef INCLUDE_MIRRORING
void hfpProfile_GetSinks(hfpInstanceTaskData *instance)
{
    DEBUG_LOG("hfpProfile_GetSinks");

    /* Make sure to get sco handle of mirroring device only */
    if (BdaddrIsSame(MirrorProfile_GetMirroredDeviceAddress(), &instance->ag_bd_addr))
    {
        instance->sco_sink = MirrorProfile_GetScoSink();
    }

    instance->slc_sink = StreamRfcommSink((uint16)(CSR_BT_CONN_ID_GET_MASK & instance->connection_id));
}
#endif

void hfpProfile_HandleAudioConnectReq(hfpInstanceTaskData * instance)
{
    DEBUG_LOG("hfpProfile_HandleAudioConnectReq(%p)", instance);
    HfAudioConnectReqSend(instance->connection_id, 0, NULL, CSR_BT_PCM_DONT_CARE, FALSE);
    
    if (instance->profile == hfp_handsfree_profile)
    {
        HfpProfileInstance_SetAudioLock(instance);
    }
}

void hfpProfile_HandleAudioDisconnectReq(hfpInstanceTaskData * instance)
{
    DEBUG_LOG("hfpProfile_HandleAudioConnectReq(%p)", instance);
    HfAudioDisconnectReqSend(instance->connection_id, instance->sco_handle);
    
    if (instance->profile == hfp_handsfree_profile)
    {
        HfpProfileInstance_SetAudioLock(instance);
    }
}
