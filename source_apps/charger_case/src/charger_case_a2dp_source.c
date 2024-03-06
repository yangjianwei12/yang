/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      application A2DP source module
*/

#include "wired_audio_source.h"
#include "unexpected_message.h"
#include "adk_log.h"
#include "charger_case_a2dp_source.h"
#include "charger_case_audio.h"
#include "chain.h"
#include "chain_input_wired_sbc_encode.h"
#include "chain_input_wired_aptx_adaptive_encode.h"
#include "chain_input_wired_aptx_classic_encode.h"
#include "kymera_chain_roles.h"
#include "av.h"
#include "device_list.h"
#include "pairing.h"
#include <ui.h>
#include "bt_device.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include <app/bluestack/hci.h>
#include "connection_manager.h"
#include <power_manager.h>
#include "charger_case_volume_observer.h"

const wired_audio_config_t wired_source_audio_config =
{
    .min_latency          = 40,
    .max_latency          = 150,
    .target_latency       = 100,
};

const wired_audio_config_t wired_source_audio_config_adaptive =
{
    .min_latency          = 40,
    .max_latency          = 150,
    .target_latency       = 70,
};

const wired_audio_config_t wired_source_audio_config_classic =
{
    .min_latency          = 40,
    .max_latency          = 150,
    .target_latency       = 100,
};
    
#define APTX_ADAPTIVE_MTU_PACKET_SIZE           (668)
#define APTX_CLASSIC_MTU_PACKET_SIZE            (668)

#define PREPROGRAMMED_DEVICE_LAP                (0x00FF02)
#define PREPROGRAMMED_DEVICE_UAP                (0x5B)
#define PREPROGRAMMED_DEVICE_NAP                (0x0002)

#define MINIMUM_RSSI_THRESHOLD                  (-50)
#define INQUIRY_MAX_RESONSES                    (20)

/*!< This is in units of 1.28s */
#define INQUIRY_TIMEOUT                         (10) 
#define INQUIRY_COD_FILTER                      (0)
#define INQUIRY_COD_FILTER_MASK                 (0)

#define PAGE_TIMEOUT_S                          (10)


charger_case_a2dp_source_data_t a2dp_source_data;


static unsigned chargerCaseA2dp_GetMediaStateContext(void)
{
    audio_source_provider_context_t context = BAD_CONTEXT;

    switch(a2dp_source_data.state)
    {
        case CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED:
        case CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY:
        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING:
        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED:
            context = context_audio_disconnected;
            break;

        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED_MEDIA:
            context = context_audio_connected;
            break;

        case CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING:
            context = context_audio_is_streaming;
            break;

        default:
            break;
     }

    return (unsigned)context;
}

static unsigned chargerCaseA2dp_GetPairingStateContext(void)
{
    pairing_provider_context_t context = BAD_CONTEXT;

    switch(a2dp_source_data.state)
    {
        case CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED:
        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING:
        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED:
        case CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED_MEDIA:
        case CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING:
            context = context_handset_pairing_idle;
            break;

        case CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY:
            context = context_handset_pairing_active;
            break;

        default:
            break;
     }

    return (unsigned)context;
}

void ChargerCase_InquiryStart(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_InquiryStart");

    a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY;
    Ui_InformContextChange(ui_provider_handset_pairing, chargerCaseA2dp_GetPairingStateContext());

    /* Reset address & peak RSSI value */
    BdaddrSetZero(&a2dp_source_data.inquiry_bd_addr[0]);
    BdaddrSetZero(&a2dp_source_data.inquiry_bd_addr[1]);

    a2dp_source_data.inquiry_rssi[0] = a2dp_source_data.inquiry_rssi[1] = MINIMUM_RSSI_THRESHOLD;

#ifdef USE_SYNERGY
    /* Send COD event filter request, on CFM start inquiry */
    CmSetEventFilterCodReqSend(&a2dp_source_data.task,
                               TRUE,
                               0,
                               INQUIRY_COD_FILTER,
                               INQUIRY_COD_FILTER_MASK);
#else
    /* Start inquiry */
    ConnectionWriteInquiryMode(&a2dp_source_data.task, inquiry_mode_rssi);
    ConnectionInquire(&a2dp_source_data.task,
                      HCI_INQ_CODE_GIAC,
                      INQUIRY_MAX_RESONSES,
                      INQUIRY_TIMEOUT,
                      INQUIRY_COD_FILTER);
#endif /* USE_SYNERGY */

}

void ChargerCase_ResetSinkDevice(void){

    DEBUG_LOG_FN_ENTRY("ChargerCase_ResetSinkDevice");
    if (a2dp_source_data.sink_device != NULL)
    {
        DeviceList_RemoveDevice(a2dp_source_data.sink_device);
        a2dp_source_data.sink_device = NULL;
        DEBUG_LOG_INFO("ChargerCase: Sink device removed from device list");
    }
}

static void chargerCase_ProcessInquiryResult(bdaddr bd_addr, int16 rssi, uint32 dev_class)
{
    DEBUG_LOG_VERBOSE("chargerCase_ProcessInquiryResult, bdaddr %x,%x,%lx rssi %d cod %lx",
                      bd_addr.nap,
                      bd_addr.uap,
                      bd_addr.lap,
                      rssi,
                      dev_class);

    /* Cache result if RSSI is higher */
    if (rssi > a2dp_source_data.inquiry_rssi[0])
    {
        DEBUG_LOG_INFO(" chargerCase_ProcessInquiryResult: Highest RSSI:, bdaddr %x,%x,%lx rssi %d cod %lx",
                       bd_addr.nap,
                       bd_addr.uap,
                       bd_addr.lap,
                       rssi,
                       dev_class);

        /* Check if address is different from previous peak */
        if (!BdaddrIsSame(&bd_addr, &a2dp_source_data.inquiry_bd_addr[0]))
        {
            /* Store previous peak RSSI */
            a2dp_source_data.inquiry_rssi[1] = a2dp_source_data.inquiry_rssi[0];
            a2dp_source_data.inquiry_bd_addr[1] = a2dp_source_data.inquiry_bd_addr[0];

            /* Store new address */
            a2dp_source_data.inquiry_bd_addr[0] = bd_addr;
            a2dp_source_data.inquiry_cod[0] = dev_class;
        }

        /* Store peak RSSI */
        a2dp_source_data.inquiry_rssi[0] = rssi;
    }
    else if (rssi > a2dp_source_data.inquiry_rssi[1])
    {
        /* Check if address is different from peak */
        if (!BdaddrIsSame(&bd_addr, &a2dp_source_data.inquiry_bd_addr[0]))
        {
            /* Store next highest RSSI */
            a2dp_source_data.inquiry_rssi[1] = rssi;
            a2dp_source_data.inquiry_cod[1] = dev_class;
        }
    }
}

static void chargerCase_ProcessInquiryComplete(void)
{
        DEBUG_LOG_INFO("ChargerCase: Inquiry Complete: bdaddr %x,%x,%lx rssi %d, next_rssi %d",
                  a2dp_source_data.inquiry_bd_addr[0].nap,
                  a2dp_source_data.inquiry_bd_addr[0].uap,
                  a2dp_source_data.inquiry_bd_addr[0].lap,
                  a2dp_source_data.inquiry_rssi[0],
                  a2dp_source_data.inquiry_rssi[1]);

        /* Attempt to connect to device with highest RSSI */
        if (!BdaddrIsZero(&a2dp_source_data.inquiry_bd_addr[0]))
        {
            /* Check if RSSI peak is sufficently higher than next, if available*/
            if ((BdaddrIsZero(&a2dp_source_data.inquiry_bd_addr[1])) ||
                (!BdaddrIsZero(&a2dp_source_data.inquiry_bd_addr[1]) &&
                (a2dp_source_data.inquiry_rssi[0] - a2dp_source_data.inquiry_rssi[1]) >= 10))
            {
                DEBUG_LOG_INFO("ChargerCase: Pairing with Highest RSSI: bdaddr %x,%x,%lx",
                               a2dp_source_data.inquiry_bd_addr[0].nap,
                               a2dp_source_data.inquiry_bd_addr[0].uap,
                               a2dp_source_data.inquiry_bd_addr[0].lap);

                /* Attempt to pair to this device */
                Pairing_PairAddress(&a2dp_source_data.task, &a2dp_source_data.inquiry_bd_addr[0]);
                a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING;
                Ui_InformContextChange(ui_provider_handset_pairing, chargerCaseA2dp_GetPairingStateContext());
                return;
            }
        }
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED;
        Ui_InformContextChange(ui_provider_handset_pairing, chargerCaseA2dp_GetPairingStateContext());
}


static void chargerCase_HandleAudioStopComplete(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAudioStopComplete");

    if (a2dp_source_data.state == CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING)
    {
        /* Move out of CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING */
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED_MEDIA;
        Ui_InformContextChange(ui_provider_media_player, chargerCaseA2dp_GetMediaStateContext());
    }
}

static void chargerCase_HandleAvA2dpConnectedIndication(AV_A2DP_CONNECTED_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvA2dpConnectedIndication");
    a2dp_source_data.active_av_instance = msg->av_instance;
}

static void chargerCase_SendAvSourceDisconnectRequest(void){
    DEBUG_LOG_FN_ENTRY("chargerCase_SendAvSourceDisconnectRequest");
    /* Send disconnect request */
    bdaddr* connect_address_ptr;
    size_t write_size;
    if (a2dp_source_data.sink_device == NULL)
    {
        DEBUG_LOG_WARN("ChargerCase: Cannot disconnect A2dp. No sink device");
        return;
    }
    Device_GetProperty(a2dp_source_data.sink_device,device_property_bdaddr,(void **)&connect_address_ptr,&write_size);
    DEBUG_LOG_INFO("ChargerCase: Requesting A2DP Disconnect : bdaddr %x,%x,%lx",
                   connect_address_ptr->nap,
                   connect_address_ptr->uap,
                   connect_address_ptr->lap);

    avInstanceTaskData *av_inst = appAvInstanceFindFromBdAddr(connect_address_ptr);
    if (!appAvA2dpDisconnectRequest(av_inst)){
        DEBUG_LOG_ERROR("appAvA2dpDisconnectRequest: Failed");
    }
}

static void chargerCase_HandleAvA2dpAudioConnected(AV_A2DP_AUDIO_CONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvA2dpAudioConnected: audioSource %d", msg->audio_source);

    source_defined_params_t a2dp_audio_source_paramaters;
    AudioSources_GetConnectParameters(msg->audio_source,&a2dp_audio_source_paramaters);
    a2dp_connect_parameters_t * a2dp_connect_params = a2dp_audio_source_paramaters.data;

    /* Get the a2dp_codec_settings from the a2dp_connect_parameters */
    a2dp_source_data.a2dp_settings.channel_mode = a2dp_connect_params->channel_mode;
    a2dp_source_data.a2dp_settings.codecData.bitpool = a2dp_connect_params->bitpool;
    a2dp_source_data.a2dp_settings.codecData.format = a2dp_connect_params->format;
    a2dp_source_data.a2dp_settings.codecData.content_protection = a2dp_connect_params->content_protection;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.q2q_enabled = a2dp_connect_params->q2q_mode;
    a2dp_source_data.a2dp_settings.codecData.aptx_ad_params.nq2q_ttp = a2dp_connect_params->nq2q_ttp;
    a2dp_source_data.a2dp_settings.rate = a2dp_connect_params->rate;
    a2dp_source_data.a2dp_settings.seid = a2dp_connect_params->seid;
    a2dp_source_data.a2dp_settings.sink = a2dp_connect_params->sink;

    if (a2dp_connect_params->seid == AV_SEID_APTX_ADAPTIVE_SRC)
    {
        a2dp_source_data.a2dp_settings.codecData.packet_size = APTX_ADAPTIVE_MTU_PACKET_SIZE;
    }
    else if (a2dp_connect_params->seid == AV_SEID_APTX_CLASSIC_SRC)
    {
        a2dp_source_data.a2dp_settings.codecData.packet_size = APTX_CLASSIC_MTU_PACKET_SIZE;
    }
    else
    {
        a2dp_source_data.a2dp_settings.codecData.packet_size = a2dp_connect_params->packet_size;
    }

    AudioSources_ReleaseConnectParameters(msg->audio_source,
                                            &a2dp_audio_source_paramaters);

    wired_audio_config_t wired_audio_config = wired_source_audio_config;
    DEBUG_LOG_DEBUG("Audio connect parameters: seid %u", a2dp_source_data.a2dp_settings.seid);
    DEBUG_LOG_DEBUG("Audio connect parameters: rate %u", a2dp_source_data.a2dp_settings.rate);
    DEBUG_LOG_DEBUG("Audio connect parameters: sink %u", a2dp_source_data.a2dp_settings.sink);
    DEBUG_LOG_DEBUG("Audio connect parameters: channel_mode %u", a2dp_source_data.a2dp_settings.channel_mode);
    DEBUG_LOG_DEBUG("Audio connect parameters: bitpool %u", a2dp_source_data.a2dp_settings.codecData.bitpool);
    DEBUG_LOG_DEBUG("Audio connect parameters: format %u", a2dp_source_data.a2dp_settings.codecData.format);
    DEBUG_LOG_DEBUG("Audio connect parameters: packet_size %u", a2dp_source_data.a2dp_settings.codecData.packet_size);

    /* If we're streaming with aptX adaptive then override audio config */
    if (a2dp_source_data.a2dp_settings.seid == AV_SEID_APTX_ADAPTIVE_SRC)
    {
        wired_audio_config = wired_source_audio_config_adaptive;
    }
    /* If we're streaming with aptX classic then override audio config */
    if (a2dp_source_data.a2dp_settings.seid == AV_SEID_APTX_CLASSIC_SRC)
    {
        wired_audio_config = wired_source_audio_config_classic;
    }
    wired_audio_config.rate = a2dp_source_data.a2dp_settings.rate;

    /* Set A2DP parameters */
    Kymera_SetA2dpOutputParams(&a2dp_source_data.a2dp_settings);

    /* Start audio source */
    if (!ChargerCase_AudioStart(&wired_audio_config))
    {
        DEBUG_LOG_INFO("ChargerCase: Failed to start audio. Disconnecting A2DP...");
        chargerCase_SendAvSourceDisconnectRequest();
        return;
    }

    /* Clear the audio lockbits */
    appA2dpClearAudioStartLockBit(a2dp_source_data.active_av_instance);
    a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING;
    Ui_InformContextChange(ui_provider_media_player, chargerCaseA2dp_GetMediaStateContext());
}

static void chargerCase_HandleAvA2dpAudioDisconnected(AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvA2dpAudioDisconnected: audioSource %d", msg->audio_source);

    /* Stop audio, if not already stopped */
    if (ChargerCase_AudioIsActive())
    {
        ChargerCase_AudioStop(chargerCase_HandleAudioStopComplete);
    }
    else
    {
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED_MEDIA;
        Ui_InformContextChange(ui_provider_media_player, chargerCaseA2dp_GetMediaStateContext());
    }
}

static void chargerCase_HandleAvA2dpMediaConnected(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvA2dpMediaConnected");
    /* We need to set the suspend reason so that it can be cleared correctly later
       when we resume */
    appAvStreamingSuspend(AV_SUSPEND_REASON_REMOTE);
    if (ChargerCase_AudioDetermineNewSource() != audio_source_none)
    {
        appAvStreamingResume(AV_SUSPEND_REASON_REMOTE);
    }
}

static void chargerCase_HandleAvA2dpConnectedConfirm(AV_A2DP_CONNECT_CFM_T *msg)
{
    UNUSED(msg);
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvA2dpConnectedConfirm");
}

static void chargerCase_HandleAvAvrcpConnectedInd(AV_AVRCP_CONNECTED_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvAvrcpConnectedInd");
    ChargerCase_OnAvrcpConnection(msg->av_instance);
}

static void chargerCase_HandleAvAvrcpDisconnectedInd(AV_AVRCP_DISCONNECT_IND_T *msg)
{
    UNUSED(msg);
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleAvAvrcpDisconnectedInd");
    ChargerCase_OnAvrcpDisconnection();
}

static void chargerCase_ConnectA2dp(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_ConnectA2dp");
    bdaddr * connect_address_ptr;

#if defined(INCLUDE_RSSI_PAIRING)
    if (a2dp_source_data.sink_device == NULL)
    {
        DEBUG_LOG_WARN("ChargerCase: Cannot connect A2dp. No sink device");
        return;
    }
    size_t write_size;
    Device_GetProperty(a2dp_source_data.sink_device,device_property_bdaddr,(void **)&connect_address_ptr,&write_size);
#else
    bdaddr connect_address;
    connect_address.lap = PREPROGRAMMED_DEVICE_LAP;
    connect_address.uap = PREPROGRAMMED_DEVICE_UAP;
    connect_address.nap = PREPROGRAMMED_DEVICE_NAP;
    connect_address_ptr = &connect_address;

    device_t device = BtDevice_GetDeviceCreateIfNew(connect_address_ptr,DEVICE_TYPE_HANDSET);
    if (DeviceList_AddDevice(device))
    {
        DeviceDbSerialiser_Serialise();
    }
    a2dp_source_data.sink_device = device;
#endif

    DEBUG_LOG_INFO("ChargerCase: Requesting A2DP Connect: bdaddr %x,%x,%lx",
                   connect_address_ptr->nap,
                   connect_address_ptr->uap,
                   connect_address_ptr->lap);

    if (appAvA2dpConnectRequest(connect_address_ptr, A2DP_CONNECT_MEDIA))
    {
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING;
    }
    else
    {
        DEBUG_LOG_ERROR("appAvA2dpConnectRequest: Failed");
    }
}


static void chargerCase_DisconnectA2dp(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_DisconnectA2dp");

    if (a2dp_source_data.state >= CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING)
    {
        chargerCase_SendAvSourceDisconnectRequest();
    }
    else
    {
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED;
    }
}

static void chargerCase_AddDevice(bdaddr bt_addr){
    DEBUG_LOG_FN_ENTRY("chargerCase_AddDevice");

    /* Since this is locally initiated pairing request we know that it is a device type sink */

    device_t device = BtDevice_GetDeviceCreateIfNew(&bt_addr,DEVICE_TYPE_SINK);
    BtDevice_SetDefaultProperties(device);
    
    a2dp_source_data.sink_device = device;
    DeviceDbSerialiser_SerialiseDevice(device);

    /* We can now connect (if there is an available audio source) */
    if (ChargerCase_AudioDetermineNewSource() != audio_source_none)
    {
        DEBUG_LOG_INFO("ChargerCase: Audio device already connected. Attempting to connect to sink device...");
        chargerCase_ConnectA2dp();
    }
}


static void chargerCase_HandlePairingConfirm(PAIRING_PAIR_CFM_T * message){

    DEBUG_LOG_FN_ENTRY("chargerCase_HandlePairingConfirm status: %u", message->status);
    switch (message->status){
        case pairingSuccess:
        {
            DEBUG_LOG_INFO("chargerCase_HandlePairingConfirm: Pairing Successful: bdaddr 0x%04x 0x%02x 0x%06lx",
                           message->device_bd_addr.nap,
                           message->device_bd_addr.uap,
                           message->device_bd_addr.lap);

            chargerCase_AddDevice(message->device_bd_addr);
        }
        break;
        default:
            DEBUG_LOG_INFO("chargerCase_HandlePairingConfirm: Status:enum:pairingStatus:%d", message->status);
        break;
    }
}

static void chargeCase_HandlePairingComplete(void) {
    DEBUG_LOG_FN_ENTRY("chargeCase_HandlePairingComplete");
}

static void chargerCase_HandlePairingFailed(void) {
    DEBUG_LOG_FN_ENTRY("chargerCase_HandlePairingFailed");
}

static void chargerCase_HandlePairingStopped(PAIRING_STOP_CFM_T *message) {
    DEBUG_LOG_INFO("chargerCase_HandlePairingStopped: status: enum:pairingStatus:%d",message->status);
}

#ifdef USE_SYNERGY
static void chargerCase_HandleCmSetEventFilterCodCfm(Task task, void *msg)
{
    CsrBtCmSetEventFilterCodCfm *prim = (CsrBtCmSetEventFilterCodCfm *)msg;
    UNUSED(task);
    
    DEBUG_LOG_DEBUG("chargerCase_HandleCmSetEventFilterCodCfm, resultCode 0x%x, resultSupplier 0x%x", prim->resultCode, prim->resultSupplier);

    if (prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS || prim->resultSupplier != CSR_BT_SUPPLIER_CM)
    {   /* Something went wrong - clear all inquiry filters */
        CmClearEventFilterReqSend(&a2dp_source_data.task, INQUIRY_RESULT_FILTER);
    }
    else
    {
        CmInquiryExtReqSend(&a2dp_source_data.task,
                            HCI_INQ_CODE_GIAC,
                            CSR_BT_CM_INQUIRY_TX_POWER_LEVEL_DEFAULT,
                            0 /*configMask */,
                            INQUIRY_MAX_RESONSES,
                            INQUIRY_TIMEOUT);
    }
}

static void chargerCase_HandleCmInquiryResultInd(Task task, void *msg)
{
    UNUSED(task);
    CsrBtCmInquiryResultInd *prim = (CsrBtCmInquiryResultInd *)msg;
    bdaddr bd_addr = { 0 };
    BdaddrConvertBluestackToVm(&bd_addr, &prim->deviceAddr);

    if (a2dp_source_data.state != CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY)
        return;

    /* Process Inquiry Result */
    chargerCase_ProcessInquiryResult(bd_addr, prim->rssi, prim->classOfDevice);
}

static void chargerCase_HandleCmInquiryCfm(Task task, void *msg)
{
    UNUSED(task);
    CsrBtCmInquiryCfm *prim = (CsrBtCmInquiryCfm *)msg;

    DEBUG_LOG_DEBUG("chargerCase_HandleCmInquiryCfm, resultCode 0x%x", prim->resultCode);
    
    chargerCase_ProcessInquiryComplete();
}

static void chargerCase_HandleCmPrimitives(Task task, void *msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_FILTER_COD_CFM:
            chargerCase_HandleCmSetEventFilterCodCfm(task, msg);
            break;

        case CSR_BT_CM_CLEAR_EVENT_FILTER_CFM:
            /* Do Nothing */
            break;

        case CSR_BT_CM_INQUIRY_RESULT_IND:
            chargerCase_HandleCmInquiryResultInd(task, msg);
            break;

        case CSR_BT_CM_INQUIRY_CFM:
            chargerCase_HandleCmInquiryCfm(task, msg);
            break;

        default:
            DEBUG_LOG_DEBUG("chargerCase_HandleCmPrimitives, unexpected CM prim 0x%x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents(msg);
}
#else
static void chargerCase_HandleClDmInquireResult(CL_DM_INQUIRE_RESULT_T *result)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_HandleClDmInquireResult");

    if (a2dp_source_data.state != CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY)
        return;

    if (result->status == inquiry_status_result)
    {
        /* Cache result if RSSI is higher */
        if (result->rssi > a2dp_source_data.inquiry_rssi[0] ||
            result->rssi > a2dp_source_data.inquiry_rssi[1])
        {
            /* Cache clock offset and page mode as this device might be the device we connect to */
            ConnectionWriteCachedClockOffset(&result->bd_addr, result->clock_offset);
            ConnectionWriteCachedPageMode(&result->bd_addr, result->page_scan_mode, result->page_scan_rep_mode);
         }

        /* Process Inquiry Result */
        chargerCase_ProcessInquiryResult(result->bd_addr, result->rssi, result->dev_class);
    }
    else
    {
        chargerCase_ProcessInquiryComplete();
    }
}
#endif /* USE_SYNERGY */

static void chargerCase_HandleSourceMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    /* Handle Connection Library messages that are not sent directly to
       the requestor */
    switch (id)
    {
        case AV_A2DP_CONNECTED_IND:
            chargerCase_HandleAvA2dpConnectedIndication((AV_A2DP_CONNECTED_IND_T *) message);
        break;
        case AV_A2DP_AUDIO_CONNECTED:
            chargerCase_HandleAvA2dpAudioConnected((AV_A2DP_AUDIO_CONNECT_MESSAGE_T *) message);
        break;
        case AV_A2DP_AUDIO_DISCONNECTED:
            chargerCase_HandleAvA2dpAudioDisconnected((AV_A2DP_AUDIO_DISCONNECT_MESSAGE_T *) message);
        break;
        case AV_A2DP_MEDIA_CONNECTED:
            chargerCase_HandleAvA2dpMediaConnected();
        break;
        case AV_A2DP_CONNECT_CFM:
            chargerCase_HandleAvA2dpConnectedConfirm((AV_A2DP_CONNECT_CFM_T *) message);
        break;
        case AV_AVRCP_CONNECTED_IND:
            chargerCase_HandleAvAvrcpConnectedInd((AV_AVRCP_CONNECTED_IND_T *) message);
        break;
        case AV_AVRCP_DISCONNECTED_IND:
            chargerCase_HandleAvAvrcpDisconnectedInd((AV_AVRCP_DISCONNECT_IND_T *) message);
        break;
        case PAIRING_COMPLETE:
            chargeCase_HandlePairingComplete();
        break;

        case PAIRING_PAIR_CFM:
            chargerCase_HandlePairingConfirm((PAIRING_PAIR_CFM_T *)message);
        break;

        case PAIRING_FAILED:
            chargerCase_HandlePairingFailed();
        break;
        case PAIRING_STOP_CFM:
            chargerCase_HandlePairingStopped((PAIRING_STOP_CFM_T *)message);
        break;
#ifdef USE_SYNERGY
        case CM_PRIM:
            chargerCase_HandleCmPrimitives(task, (void *) message);
            break;
#else
        case CL_DM_INQUIRE_RESULT:
            chargerCase_HandleClDmInquireResult((CL_DM_INQUIRE_RESULT_T *)message);
            break;
#endif /* USE_SYNERGY */


        default:
            UnexpectedMessage_HandleMessage(id);
        break;
    }
}


void ChargerCase_A2dpSourceConnect(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_A2dpSourceConnect");

    if (Av_IsA2dpSourceConnected())
    {
        DEBUG_LOG_INFO("ChargerCase: A2DP is already connected");
        ChargerCase_A2dpSourceResume();
        return;
    }

    /* Connect before starting audio (started later via callback) */
    chargerCase_ConnectA2dp();
}

void ChargerCase_A2dpSourceDisconnect(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_A2dpSourceDisconnect");

    /* Request disconnect (audio stopped via callback) */
    chargerCase_DisconnectA2dp();
}

void ChargerCase_A2dpSourceSuspend(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_A2dpSourceSuspend");

    if (Av_IsA2dpSourceConnected())
    {
        DEBUG_LOG_INFO("ChargerCase: Suspending AV streaming");
        /* This suspends the AV Source triggering a state transition from
          "streaming" to the "media connected" state. */
        appAvStreamingSuspend(AV_SUSPEND_REASON_LOCAL);
    }
}

void ChargerCase_A2dpSourceResume(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_A2dpSourceResume");

    if (Av_IsA2dpSourceConnected())
    {
        DEBUG_LOG_INFO("ChargerCase: Resuming AV streaming");
        /* Clear the local reason to trigger audio reconnection */
        appAvStreamingResume(AV_SUSPEND_REASON_LOCAL);
        /* Also clear remote reason in case it wasn't cleared earlier */
        appAvStreamingResume(AV_SUSPEND_REASON_REMOTE);
    }
}

void ChargerCase_A2dpSourceInit(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCase_A2dpSourceInit");

    a2dp_source_data.task.handler = chargerCase_HandleSourceMessage;

    /* Disable SDP security */
    ConnectionSmSetSdpSecurityIn(FALSE);
#ifdef USE_SYNERGY
    uint16_t config = DM_SM_SEC_MODE_CONFIG_NONE | DM_SM_SEC_MODE_CONFIG_LEGACY_AUTO_PAIR_MISSING_LINK_KEY;
    CmSecurityModeConfigRequest(cl_sm_wae_acl_none, config);
#else
    ConnectionSmSecModeConfig(&a2dp_source_data.task, cl_sm_wae_acl_none, FALSE, TRUE);
#endif
    /* Configure Connection manager */
    ConManager_SetPageTimeout(MS_TO_BT_SLOTS(PAGE_TIMEOUT_S * MS_PER_SEC));

    /* Register as ui provider */
    Ui_RegisterUiProvider(ui_provider_media_player, chargerCaseA2dp_GetMediaStateContext);
    Ui_RegisterUiProvider(ui_provider_handset_pairing, chargerCaseA2dp_GetPairingStateContext);

    appAvStatusClientRegister(&a2dp_source_data.task);


    deviceType device_type = DEVICE_TYPE_SINK;
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &device_type, sizeof(device_type));

    if (device)
    {
        bdaddr device_address = DeviceProperties_GetBdAddr(device);
        DEBUG_LOG_INFO("ChargerCase: sink device loaded from device list: bdaddr 0x%04x 0x%02x 0x%06lx",
                       device_address.nap,
                       device_address.uap,
                       device_address.lap);

        a2dp_source_data.sink_device = device;
    }

}

