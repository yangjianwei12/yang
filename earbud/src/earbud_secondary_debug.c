/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file       earbud_secondary_debug.c
\brief      Earbud Application interface for testing le_debug_secondary module
*/

#ifdef ENABLE_LE_DEBUG_SECONDARY
#include "earbud_secondary_debug.h"

#include <le_advertising_manager.h>
#include <connection_manager.h>
#include <handover_profile.h>
#include <pairing.h>
#include <logging.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
* garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
#error Multiple data sets concurrency of LE advertising manager MUST BE enabled to enable secondary debug mode
#endif /* INCLUDE_LEGACY_LE_ADVERTISING_MANAGER */

/*! \brief Size of sample advertisement payload */
#define SAMPLE_ADV_SIZE 25
/*! \brief Size of payload when advertisements are stopped*/
#define MIN_ADVERT_INTERVAL_MS 90
#define MAX_ADVERT_INTERVAL_MS 100
#define MIN_RANDOM_ADDRESS_ROTATION_TIMEOUT_MINUTES 14
#define MAX_RANDOM_ADDRESS_ROTATION_TIMEOUT_MINUTES 15

/*! \brief Secondary Debug Context */
typedef struct
{
    /*! is debug mode enabled */
    bool in_debug_mode;

    /*! BLE Central BD address */
    tp_bdaddr tp_bd_addr;

    /*! Is BLE Advertising enabled */
    bool is_advertising;

    /*! restore local IRK to primary IRK while in debug mode */
    bool override_debug_mode;

    /*! le advertisement handle */
    le_adv_item_handle advertising_handle;
} earbudSecondaryDebugContext;

/*! \brief Callback function returning the advertisement data to LE advertisement manager */
static bool earbudSecondaryDebug_GetItemData(le_adv_item_data_t * adv_data);

/*! \brief Callback function returning the configuration information for adverts to LE advertisement manager */
static bool earbudSecondaryDebug_GetItemInfo(le_adv_item_info_t * info);

/*! \brief Callback function to handle LEAM_EVENT_ADVERTISING_SET_SUSPENDED notification from LE Advertisement manager */
static void earbudSecondaryDebug_HandleAdvStateNtf(uint8 event);

/*! \brief Callback function to provide advertisement parameters for the set that is created */
static bool earbudSecondaryDebug_GetItemParams(le_adv_item_params_t * params);

/*! \brief Callback function to handle connection manager notifications

    This callback function shall be registered and invoked only when
    - Earbuds are peer paired and the respective roles are identified.
    - The local IRK is modified on secondary earbud, to enable debug mode.

    This function shall handle connection manager notifications over BLE transport
    and from non peer device, to make sure only notifications from Debug BLE 
    central device are handled.

    \param tpaddr typed bluetooth address with transport of the connecting device.
    \param is_connnected TRUE for notifications with respect to connection, FALSE for disconnection.
    \param reason given for connection/disconnection

    \return TRUE if the notifications are handled, FALSE otherwise
*/
static bool earbudSecondaryDebug_HandleConnectionManagerNtf(const tp_bdaddr *tpaddr, con_manager_notify_group_type_t notify_group_type, hci_status reason);

/*! \brief Callback function to handle respective Pairing CM Prims

    This callback function shall be registered and invoked only when
    - Earbuds are peer paired and the respective roles are identified.
    - The local IRK is modified on secondary earbud, to enable debug mode.

    \param msg CM Security prim messages

    \return TRUE if prim is handled, FALSE otherwise
*/
static bool earbudSecondaryDebug_HandleCmPairingInd(Message msg);

/*! \brief Callback function to check if handover is allowed from acceptor role

    On invocation, this callback checks if the local IRK has been modified to connect
    with debug BLE central device and cancel the handover from secondary role. Handover
    is prohibited when local IRK is modified on secondary earbud.

    \return TRUE if handover is allowed, FALSE otherwise
*/
static bool earbudSecondaryDebug_IsHandoverAllowedOnAcceptorCb(void);

static earbudSecondaryDebugContext earbud_secondary_debug_data;

/*! \brief Sample advertisement payload */
static const uint8 sample_payload[SAMPLE_ADV_SIZE] =
{
    SAMPLE_ADV_SIZE - 1,
    ble_ad_type_manufacturer_specific_data,
    0xe, 0xf, 0x1, 0x2, 0xa, 0xb, 0x3, 0x4, 0xc, 0xd,
    0x9, 0x8, 0x5, 0x6, 0xd, 0xc, 0x1, 0x2, 0xa, 0xb,
    0x5, 0x6, 0xf
};

/*! \brief Sample advertisement */
static const le_adv_item_data_t sample_advert =
{
    SAMPLE_ADV_SIZE,
    sample_payload
};

/*! \brief Register callbacks with LE advertisement manager */
static const le_adv_item_callback_t earbudSecondaryDebug_advertising_manager_callback =
{
    .GetItemData = &earbudSecondaryDebug_GetItemData,
    .GetItemInfo = &earbudSecondaryDebug_GetItemInfo,
    .NotifyAdvertisingEvent = &earbudSecondaryDebug_HandleAdvStateNtf,
    .GetItemParameters = &earbudSecondaryDebug_GetItemParams,
};

/*! \brief Register callbacks with Handover profile */
static const handover_profile_acceptor_cb_t earbudSecondaryDebug_handover_profile_cb =
{
    .start_req_cb = earbudSecondaryDebug_IsHandoverAllowedOnAcceptorCb,
};

static void earbudSecondaryDebug_DisconnectDeviceAndRestoreIrk(void)
{
    DEBUG_LOG("earbudSecondaryDebug_Disconnect");

    if (!BdaddrTpIsEmpty(&earbud_secondary_debug_data.tp_bd_addr))
    {
        ConManagerReleaseTpAcl(&earbud_secondary_debug_data.tp_bd_addr);
    }
    else
    {
        /* No link to disconnect, revert the local IRK back to primary */
        LeDebugSecondary_UpdateLocalIRK(FALSE, SmGetTask());
    }

}

static bool earbudSecondaryDebug_GetItemData(le_adv_item_data_t * adv_data)
{
    PanicNull(adv_data);
    adv_data->data = NULL;
    adv_data->size = 0;

    if (earbud_secondary_debug_data.is_advertising)
    {
        *adv_data = sample_advert;
    }

    return TRUE;
}

static bool earbudSecondaryDebug_GetItemInfo(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){.placement = le_adv_item_data_placement_advert,
                                 .type = le_adv_type_legacy_connectable_scannable,
                                 .data_size = earbud_secondary_debug_data.is_advertising ?
                                              sample_advert.size : 0,
                                 .override_connectable_state = TRUE,
                                };

    return TRUE;
}

static bool earbudSecondaryDebug_GetItemParams(le_adv_item_params_t * params)
{
    params->primary_adv_interval_min = MSEC_TO_LE_TIMESLOT(MIN_ADVERT_INTERVAL_MS);
    params->primary_adv_interval_max = MSEC_TO_LE_TIMESLOT(MAX_ADVERT_INTERVAL_MS);
    params->random_addr_type = ble_local_addr_generate_resolvable;
    params->random_addr_generate_rotation_timeout_minimum_in_minutes = MIN_RANDOM_ADDRESS_ROTATION_TIMEOUT_MINUTES;
    params->random_addr_generate_rotation_timeout_maximum_in_minutes = MAX_RANDOM_ADDRESS_ROTATION_TIMEOUT_MINUTES;

    return TRUE;
}

static bool earbudSecondaryDebug_IsHandoverAllowedOnAcceptorCb(void)
{
    return !LeDebugSecondary_IsSecondaryIRKInUse();
}

/*! \brief Enable/Disable debug advertisements on secondary earbud */
static void earbudSecondaryDebug_SetDebugAdverts(bool enable)
{
    enable = !!enable;

    if (earbud_secondary_debug_data.is_advertising != enable)
    {
        DEBUG_LOG("earbudSecondaryDebug_SetDebugAdverts current state 0x%x new state 0x%x",
                   earbud_secondary_debug_data.is_advertising, enable);

        earbud_secondary_debug_data.is_advertising = enable;
        LeAdvertisingManager_UpdateAdvertisingItem(earbud_secondary_debug_data.advertising_handle);
    }
}

/*! \brief Enable/Disable debug advertisements on secondary earbud */
static void earbudSecondaryDebug_ConfigureDebugAdverts(bool adv_item_cb)
{
    DEBUG_LOG("earbudSecondaryDebug_ConfigureDebugAdverts enable_adverts 0x%x", adv_item_cb);

    if (adv_item_cb)
    {
        /* Register callback with Advertising Manager, which in turn will start the adverts */
        earbud_secondary_debug_data.advertising_handle =
                    LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &earbudSecondaryDebug_advertising_manager_callback);
    }
    else
    {
        /* De-Register callback with Advertising Manager */
        LeAdvertisingManager_UnregisterAdvertisingItem(earbud_secondary_debug_data.advertising_handle);
        earbud_secondary_debug_data.advertising_handle = NULL;
    }
}

static void earbudSecondaryDebug_HandleAdvStateNtf(uint8 event)
{
    DEBUG_LOG("earbudSecondaryDebug_HandleAdvStateNtf event->%d",event);

    /* If advertisements are stopped while disabling debug mode, 
       Unregister LE Advertisement sets */
    if (!earbud_secondary_debug_data.in_debug_mode)
    {
        earbudSecondaryDebug_ConfigureDebugAdverts(FALSE);
    }
}

static bool earbudSecondaryDebug_HandleConnectionManagerNtf(const tp_bdaddr *tpaddr, con_manager_notify_group_type_t notify_group_type, hci_status reason)
{
    UNUSED(reason);

    if (tpaddr->transport != TRANSPORT_BLE_ACL || BtDevice_LeDeviceIsPeer(tpaddr))
    {
        return FALSE;
    }

    if (notify_group_type == con_manager_notify_group_type_connect)
    {
        earbud_secondary_debug_data.tp_bd_addr = *tpaddr;
        /* BLE Central device connected , stop advertisements */
        earbudSecondaryDebug_SetDebugAdverts(FALSE);
    }
    else
    {
        /* Start debug advertisements back as debug link is disconnected */
        earbudSecondaryDebug_SetDebugAdverts(TRUE);
        BdaddrTpSetEmpty(&earbud_secondary_debug_data.tp_bd_addr);

        if (!earbud_secondary_debug_data.in_debug_mode ||
            !TwsTopology_IsRoleSecondary() ||
            earbud_secondary_debug_data.override_debug_mode)
        {
            /*  Restore the local irk back to primary when debug link is disconnected
                while performing below actions
               - exiting debug mode
               - exiting secondary role
               - When we want to override debug mode
            */
            LeDebugSecondary_UpdateLocalIRK(FALSE, SmGetTask());
            earbud_secondary_debug_data.override_debug_mode = FALSE;
        }
    }

    return TRUE;
}

static void earbudSecondaryDebug_HandleCmSmIoCapabilityRequestInd(const CsrBtCmSmIoCapabilityRequestInd *ind)
{
    /* Preferred key distribution */
    uint16 key_distribution = KEY_DIST_RESPONDER_ENC_CENTRAL |
                              KEY_DIST_RESPONDER_ID |
                              KEY_DIST_INITIATOR_ENC_CENTRAL |
                              KEY_DIST_INITIATOR_ID;

    /* We dont expect a non LE pairing req in this callback */
    if (ind->tp_addrt.tp_type == LE_ACL)
    {
        CsrBtCmScDmIoCapabilityRequestRes(ind->tp_addrt.addrt.addr,
                                          ind->tp_addrt.addrt.type,
                                          ind->tp_addrt.tp_type,
                                          HCI_IO_CAP_NO_INPUT_NO_OUTPUT,
                                          DM_SM_SECURITY_SECURE_CONNECTIONS,
                                          (CsrUint8) oob_data_none,
                                          NULL,
                                          NULL,
                                          key_distribution);

        DEBUG_LOG("earbudSecondaryDebug_HandleCmSmIoCapabilityRequestInd");
    }

}

static bool earbudSecondaryDebug_HandleCmPairingInd(Message msg)
{
    typed_bdaddr permanentAddr;
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    switch (*prim)
    {
        case CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND:
            earbudSecondaryDebug_HandleCmSmIoCapabilityRequestInd((const CsrBtCmSmIoCapabilityRequestInd *) msg);
            break;

        case CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND:
            {
                DEBUG_LOG("earbudSecondaryDebug_HandleCmSecurityPrim : CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND");

                if (((const CsrBtCmSmSimplePairingCompleteInd *)msg)->status == HCI_SUCCESS)
                {
                    if (BtDevice_GetPublicAddress(&earbud_secondary_debug_data.tp_bd_addr.taddr, &permanentAddr))
                    {
                        earbud_secondary_debug_data.tp_bd_addr.taddr = permanentAddr;
                    }
                }
            }
            break;

        default:
            break;
    }

    CmFreeUpstreamMessageContents((void *) msg);
    return TRUE;
}

static void earbudSecondaryDebug_SetCallback(void)
{
    if (LeDebugSecondary_IsSecondaryIRKInUse())
    {
        /* Register callbacks with connection manager to handle connection events */
        ConManager_SetNotificationFilter(earbudSecondaryDebug_HandleConnectionManagerNtf);
        /* Register callbacks with pairing module to handle CM Security messaged */
        Pairing_OverrideCmPrimHandler(earbudSecondaryDebug_HandleCmPairingInd);
        /* Register callback with Handover profile to dissallow handover */
        HandoverProfile_RegisterAcceptorCb(&earbudSecondaryDebug_handover_profile_cb);
    }
    else
    {
        /* De-register callbacks with connection manager */
        ConManager_SetNotificationFilter(NULL);
        /* De-Register callbacks with pairing module to handle CM Security messaged */
        Pairing_OverrideCmPrimHandler(NULL);
        /* De-Register callback with Handover profile to dissallow handover */
        HandoverProfile_RegisterAcceptorCb(NULL);

    }
}

void EarbudSecondaryDebug_Enable(void)
{
    earbud_secondary_debug_data.in_debug_mode = TRUE;
    earbud_secondary_debug_data.is_advertising = FALSE;

    BdaddrTpSetEmpty(&earbud_secondary_debug_data.tp_bd_addr);
    earbudSecondaryDebug_ConfigureDebugAdverts(TRUE);

    if (TwsTopology_IsRoleSecondary())
    {
        LeDebugSecondary_UpdateLocalIRK(TRUE, SmGetTask());
    }

    DEBUG_LOG("EarbudSecondaryDebug_Enable is Secondary 0x%x", TwsTopology_IsRoleSecondary());
}

void EarbudSecondaryDebug_Disable(void)
{
    earbud_secondary_debug_data.in_debug_mode = FALSE;
    /* Disconnect debug BLE link if any */
    earbudSecondaryDebug_DisconnectDeviceAndRestoreIrk();
}

void EarbudSecondaryDebug_HandleUpdateIrkCfm(LE_DEBUG_SECONDARY_UPDATE_IRK_CFM_T *message)
{
    DEBUG_LOG("EarbudSecondaryDebugHandleUpdateIrkCfm Is IRK Updated : 0x%x", LeDebugSecondary_IsSecondaryIRKInUse());

    if (message->status != le_debug_update_irk_status_success)
    {
        earbud_secondary_debug_data.in_debug_mode = FALSE;
    }

    /* Register/Deregister respective callback function */
    earbudSecondaryDebug_SetCallback();
    /* Start/Stop Debug Advertisements */
    earbudSecondaryDebug_SetDebugAdverts(LeDebugSecondary_IsSecondaryIRKInUse());
}

void EarbudSecondaryDebug_HandleTwsTopologyRoleChange(tws_topology_role new_role, tws_topology_role old_role)
{
    DEBUG_LOG("EarbudSecondaryDebug_HandleTwsTopologyRoleChange old_role : 0x%x, new_role : 0x%x, debug_mode : 0x%x",
               old_role, new_role, earbud_secondary_debug_data.in_debug_mode);

    if (earbud_secondary_debug_data.in_debug_mode)
    {
        /*  Switch to Secondary IRK, Start the debug advertisements, and accept connections while
            in tws_topology_role_secondary Regardless of the old_role */
        if (new_role == tws_topology_role_secondary)
        {
             LeDebugSecondary_UpdateLocalIRK(TRUE, SmGetTask());
        }
        else if (old_role == tws_topology_role_secondary)
        {
            /*  Disconnect Debug Connection (if Any) and revert the local IRK back to primary as 
                as part of disconnect completion */
            earbudSecondaryDebug_DisconnectDeviceAndRestoreIrk();
        }
    }
}

void EarbudSecondaryDebug_DisconnectOrStopAdvert(void)
{
    if (TwsTopology_IsRoleSecondary())
    {
        DEBUG_LOG("EarbudSecondaryDebug_DisconnectDeviceAndSwitchIrk");

        earbud_secondary_debug_data.override_debug_mode = TRUE;
        earbudSecondaryDebug_DisconnectDeviceAndRestoreIrk();
    }
}

#endif /* ENABLE_LE_DEBUG_SECONDARY */
