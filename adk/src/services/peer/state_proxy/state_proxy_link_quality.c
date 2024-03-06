/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \ingroup    state_proxy
    \brief      Link quality measurement
*/

#include "connection_abstraction.h"
#include <message.h>
#include <peer_signalling.h>
#include <logging.h>
#include <stdlib.h>
#include <mirror_profile.h>
#include <connection_manager_list.h>

#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_link_quality.h"
#include "state_proxy_connection.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_client_msgs.h"
#include "kymera.h"

#ifdef INCLUDE_DFU_PEER
#include <dfu.h>
#endif

#ifdef ENABLE_LINK_QUALITY_LOG
#define LINK_QUALITY_LOG DEBUG_LOG
#else
#define LINK_QUALITY_LOG(...)
#endif

/* Link quality measurement interval */
#define STATE_PROXY_LINK_QUALITY_INTERVAL_MS 500

/* Enabling this definition plays a short beep whose pitch is proportional to the
   measured RSSI value. This is solely to aid test/debug of RSSI handover in
   form-factor earbuds where full logging is unavailable.
   The primary plays a single demisemiquaver beep and the secondary plays a
   double hemidemisemiquaver beep to allow the roles to be distinguished.
*/
#if defined(INCLUDE_STATE_PROXY_RSSI_TONES)

#include "kymera.h"

/* RSSI associated with highest pitch tone */
#define RSSI_MAX -20

/* This note description is used for both primary/secondary. The
   RINGTONE_NOTE and the first RINGTONE_END are changed dynamically depending
   on the primary/secondary role and the RSSI */
static ringtone_note rssi_tone[] =
{
    RINGTONE_TIMBRE(sine),
    RINGTONE_DECAY(20),
    RINGTONE_NOTE(B6, DEMISEMIQUAVER),
    /* Intentional duplicate - on primary it concludes the tone, on secondary
       overwritten with a second note, thus the second RINGTONE_END concludes
       the tone. */
    RINGTONE_END,
    RINGTONE_END
};

/*! \brief Convert a RSSI in dBm into a ringtone_tone.
    \param rssi The RSSI in dBm
    \param length The note duration
    \note Map the highest frequency tone to RSSI_MAX and linearly map lower
    RSSIs to lower frequency tones.
*/
static ringtone_note rssi_to_tone(int16 rssi, uint16 length)
{
    uint16 note;
    rssi = MIN(RSSI_MAX, rssi);
    rssi = rssi + (-RSSI_MAX) + (RINGTONE_NOTE_B9 >> RINGTONE_SEQ_NOTE_PITCH_POS);
    note = (uint16)rssi << RINGTONE_SEQ_NOTE_PITCH_POS;
    return ((ringtone_note) (RINGTONE_SEQ_NOTE | length | note));
}

#endif

static bool stateProxy_GetRssiBdAddrBlocking(const tp_bdaddr * acl_address, int16 *rssi)
{
    if (VmBdAddrGetRssi(acl_address, rssi))
    {
#if defined(INCLUDE_STATE_PROXY_RSSI_TONES)
        if (!appKymeraIsTonePlaying())
        {
            if (StateProxy_IsPrimary())
            {
                rssi_tone[2] = rssi_to_tone(*rssi, RINGTONE_NOTE_DEMISEMIQUAVER);
                rssi_tone[3] = RINGTONE_END;
            }
            else
            {
                rssi_tone[2] = rssi_tone[3] = rssi_to_tone(*rssi, RINGTONE_NOTE_HEMIDEMISEMIQUAVER);
            }
            appKymeraTonePlay(rssi_tone, VmGetTimerTime(), FALSE, FALSE, NULL, 0);

        }
#endif
        return TRUE;
    }
    return FALSE;
}

static bool stateProxy_GetLinkQualityBdAddrBlocking(const tp_bdaddr * acl_address, uint16 *lq)
{
    if (VmGetAclLinkQuality(acl_address, lq))
    {
        return TRUE;
    }
    return FALSE;
}

static void stateProxy_SendIntervalTimerMessage(void)
{
    MessageSendLater(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_LINK_QUALITY,
                     NULL, STATE_PROXY_LINK_QUALITY_INTERVAL_MS);
}

static void stateProxy_NotifyLinkQualityClients(state_proxy_source source, const STATE_PROXY_LINK_QUALITY_T *lqi)
{
    /* notify event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_link_quality,
                                         lqi);

    stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(STATE_PROXY_LINK_QUALITY_T), lqi, sizeof(*lqi));
}

static bdaddr *stateProxy_GetLinkQualityAddress(void)
{
    bdaddr *mirrored = MirrorProfile_GetMirroredDeviceAddress();
    return (mirrored && BdaddrIsZero(mirrored)) ? NULL : mirrored;
}

static void stateProxy_StartNextMeasurement(void)
{
    bdaddr *addr = stateProxy_GetLinkQualityAddress();
    if (addr && Kymera_IsA2dpSynchronisationNotInProgress()
#ifdef INCLUDE_DFU_PEER
        && !(!BtDevice_IsMyAddressPrimary() && Dfu_IsUpgradeInProgress())
#endif
    )
    {
        STATE_PROXY_LINK_QUALITY_T lqi;
        BdaddrTpFromBredrBdaddr(&lqi.device, addr);
        int16 rssi;
        if (stateProxy_GetRssiBdAddrBlocking(&lqi.device, &rssi) && stateProxy_GetLinkQualityBdAddrBlocking(&lqi.device, &lqi.link_quality))
        {
            lqi.rssi = rssi;
            LINK_QUALITY_LOG("stateProxy_StartNextMeasurement RSSI:[%d], LQ:[%d], addr:[0x%x]", rssi, lqi.link_quality, lqi.device.taddr.addr.lap);
            /* Notify the clients */
            stateProxy_NotifyLinkQualityClients(state_proxy_source_local, &lqi);
        }
    }
}

void stateProxy_LinkQualityKick(void)
{
    bool enable = FALSE;
    if (stateProxy_AnyClientsRegisteredForEvent(state_proxy_event_type_link_quality))
    {
        if (stateProxy_GetLinkQualityAddress())
        {
            enable = TRUE;
        }
    }
    if (enable && !stateProxy_IsMeasuringLinkQuality())
    {
        stateProxy_SendIntervalTimerMessage();
        stateProxy_SetMesauringLinkQuality(TRUE);
        stateProxy_StartNextMeasurement();
    }
    else if (!enable && stateProxy_IsMeasuringLinkQuality())
    {
        MessageCancelAll(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_LINK_QUALITY);
        stateProxy_SetMesauringLinkQuality(FALSE);
    }
}

void stateProxy_HandleIntervalTimerLinkQuality(void)
{
    stateProxy_SetMesauringLinkQuality(FALSE);
    stateProxy_LinkQualityKick();
}

void stateProxy_HandleRemoteLinkQuality(const STATE_PROXY_LINK_QUALITY_T *msg)
{
    /* There could be chance that reception of previously mirroring LinkQuality stats
     * delayed or mixed up during switching the mirroring device, so ignore those updates*/
    if (MirrorProfile_IsSameMirroredDevice(&msg->device))
    {
        stateProxy_NotifyLinkQualityClients(state_proxy_source_remote, msg);
    }
}

#ifdef INCLUDE_HDMA_CIS_RSSI_EVENT

static void stateProxy_SendIntervalCisRssiTimerMessage(void)
{
    MessageSendLater(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_CIS_RSSI_QUALITY,
                     NULL, STATE_PROXY_LINK_QUALITY_INTERVAL_MS);
}

static void stateProxy_NotifyCisRssiClients(state_proxy_source source, const STATE_PROXY_CIS_RSSI_T *cis_rssi)
{
    /* notify CIS RSSI event specific clients */
    stateProxy_MsgStateProxyEventClients(source,
                                         state_proxy_event_type_cis_rssi, cis_rssi);

    stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(STATE_PROXY_CIS_RSSI_T), cis_rssi, sizeof(*cis_rssi));
}

static void stateProxy_StartNextCisRssiMeasurement(void)
{
    int16 rssi;
    bdaddr *addr = MirrorProfile_GetCisMirroredDeviceAddress();

    if (addr
#ifdef INCLUDE_DFU_PEER
        && !(!BtDevice_IsMyAddressPrimary() && Dfu_IsUpgradeInProgress())
#endif
      )
    {
        STATE_PROXY_CIS_RSSI_T val;

        MirrorProfile_GetCisHandle(&val.cis_handle, NULL);
        val.device.transport = TRANSPORT_BLE_ACL;
        val.device.taddr.type = TYPED_BDADDR_PUBLIC;
        val.device.taddr.addr = *addr;

        if (VmCisConnGetRssi(&val.device, val.cis_handle, &rssi))
        {
            val.cis_rssi = rssi;
            stateProxy_NotifyCisRssiClients(state_proxy_source_local, &val);
        }
    }
}

void stateProxy_CisRssiKick(void)
{
    bool enable = FALSE;

    if (stateProxy_AnyClientsRegisteredForEvent(state_proxy_event_type_cis_rssi))
    {
        if (MirrorProfile_GetCisMirroredDeviceAddress())
        {
            enable = TRUE;
        }
    }

    if (enable && !stateProxy_IsMeasuringCisRssi())
    {

        stateProxy_SendIntervalCisRssiTimerMessage();
        stateProxy_SetMeasuringCisRssi(TRUE);
        stateProxy_StartNextCisRssiMeasurement();
    }
    else if (!enable && stateProxy_IsMeasuringCisRssi())
    {
        MessageCancelAll(stateProxy_GetTask(), STATE_PROXY_INTERNAL_TIMER_CIS_RSSI_QUALITY);
        stateProxy_SetMeasuringCisRssi(FALSE);
    }
}

void stateProxy_HandleIntervalTimerCisRssiQuality(void)
{
    stateProxy_SetMeasuringCisRssi(FALSE);
    stateProxy_CisRssiKick();
}

void stateProxy_HandleRemoteCisRssiUpdate(const STATE_PROXY_CIS_RSSI_T *msg)
{
    stateProxy_NotifyCisRssiClients(state_proxy_source_remote, msg);
}

#endif
