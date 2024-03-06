/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup peer_find_role
    \brief
*/

#include "peer_find_role_advertising.h"

#include "peer_find_role_sm.h"
#include "peer_find_role_private.h"

#include "bt_device.h"
#include "connection_manager.h"
#include "le_advertising_manager.h"
#include <gatt_role_selection_server_uuids.h>
#include <panic.h>
#include <logging.h>


#define SIZE_PEER_FIND_ROLE_ADVERT      4

static const uint8 peer_find_role_advert_data[SIZE_PEER_FIND_ROLE_ADVERT] = {
    SIZE_PEER_FIND_ROLE_ADVERT - 1,
    ble_ad_type_complete_uuid16,
    UUID_ROLE_SELECTION_SERVICE & 0xFF,
    UUID_ROLE_SELECTION_SERVICE >> 8
};

static le_adv_item_data_t peer_find_role_advert;
static le_adv_item_handle peer_find_role_registered_handle = NULL;

static inline bool peerFindRole_CanAdvertise(void)
{
    return peer_find_role_is_in_advertising_state();
}

static bool peerFindRole_GetItemData(le_adv_item_data_t * item)
{
    PanicNull(item);
    bool can_advertise = FALSE;
    item->data = NULL;
    item->size = 0;
    if(peerFindRole_CanAdvertise())
    {
#ifndef ENABLE_PEER_FIND_ROLE_DIRECTED_ADVERTISING
        *item = peer_find_role_advert;
#endif
        can_advertise = TRUE;
    }
    return can_advertise;
}

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

static unsigned peerFindRole_GetItemDataSize(void)
{
    unsigned data_size = 0;
#ifndef ENABLE_PEER_FIND_ROLE_DIRECTED_ADVERTISING
    if(peerFindRole_CanAdvertise())
    {
        data_size = SIZE_PEER_FIND_ROLE_ADVERT;
    }
#endif
    return data_size;
}

static bool peerFindRole_GetItemInfo(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
#if defined ENABLE_PEER_FIND_ROLE_DIRECTED_ADVERTISING
                                    .type = le_adv_type_legacy_directed,
#else
                                    .type = le_adv_type_legacy_connectable_scannable,
#endif
                                    .data_size = peerFindRole_GetItemDataSize(),
#if defined(ENABLE_PEER_FIND_ROLE_ACCEPTOR_LIST)
                                    .needs_own_set = TRUE,
#endif
                                    .override_connectable_state = TRUE };
    return TRUE;
}

static bool peerFindRole_GetItemParams(le_adv_item_params_t * params)
{
    params->primary_adv_interval_min = MSEC_TO_LE_TIMESLOT(90);
    params->primary_adv_interval_max = MSEC_TO_LE_TIMESLOT(100);

#if defined(ENABLE_PEER_FIND_ROLE_ACCEPTOR_LIST)
    params->adv_filter_policy = ble_filter_connect_only;
#endif

#if defined ENABLE_PEER_FIND_ROLE_DIRECTED_ADVERTISING
    appDeviceGetPeerBdAddr(&params->peer_tpaddr.addr);
    params->peer_tpaddr.type = TYPED_BDADDR_PUBLIC;
#endif

    return TRUE;
}

le_adv_item_callback_t peerFindRole_AdvertisingManagerCallback =
{
    .GetItemData = &peerFindRole_GetItemData,
    .GetItemInfo = &peerFindRole_GetItemInfo,
    .GetItemParameters = &peerFindRole_GetItemParams,
};

#else

#define NUMBER_OF_ADVERT_DATA_ITEMS     1

/* Return the number of items in the advert.
   For simplicity/safety don't make the same check when getting data items. */
static unsigned int peerFindRole_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    unsigned items = 0;

    if (peer_find_role_is_in_advertising_state())
    {
        if((le_adv_data_set_peer == params->data_set) && \
           (le_adv_data_completeness_full == params->completeness) && \
           (le_adv_data_placement_advert == params->placement))
        {
            items = NUMBER_OF_ADVERT_DATA_ITEMS;
        }
    }

    return items;
}


static le_adv_data_item_t peerFindRole_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);
    le_adv_data_item_t item = { .size = 0, .data = NULL };
    if((le_adv_data_set_peer == params->data_set) && \
        (le_adv_data_completeness_full == params->completeness) && \
        (le_adv_data_placement_advert == params->placement))
    {
        peerFindRole_GetItemData(&item);
        return item;
    }
    else
    {
        Panic();
        return item;
    };
}

static void peerFindRole_ReleaseAdvDataItems(const le_adv_data_params_t * params)
{
    UNUSED(params);
    return;
}

static const le_adv_data_callback_t peerFindRole_AdvertisingManagerCallback =
{
    .GetNumberOfItems = peerFindRole_NumberOfAdvItems,
    .GetItem = peerFindRole_GetAdvDataItems,
    .ReleaseItems = peerFindRole_ReleaseAdvDataItems
};

#endif

bool PeerFindRole_SetupLeAdvertisingData(void)
{
    peer_find_role_advert.size = SIZE_PEER_FIND_ROLE_ADVERT;
    peer_find_role_advert.data = peer_find_role_advert_data;
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    peer_find_role_registered_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &peerFindRole_AdvertisingManagerCallback);
#else
    peer_find_role_registered_handle = LeAdvertisingManager_Register(NULL, &peerFindRole_AdvertisingManagerCallback);
#endif
    return (peer_find_role_registered_handle ? TRUE : FALSE);
}


bool PeerFindRole_UpdateLeAdvertisingData(void)
{
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    return (peer_find_role_registered_handle ? LeAdvertisingManager_UpdateAdvertisingItem(peer_find_role_registered_handle) : FALSE);
#else
    return (peer_find_role_registered_handle ? LeAdvertisingManager_NotifyDataChange(NULL, peer_find_role_registered_handle) : FALSE);
#endif
}

le_adv_item_handle PeerFindRole_GetAdvertisingItemHandle(void)
{
    return peer_find_role_registered_handle;
}



/*! \brief Add the Peer addr to the LE acceptor list.

    The Peer Find Role adverts will only allow devices on the LE acceptor list
    to connect. So the peer address needs to be on the acceptor list so that
    peer find role can succeed.
*/
void PeerFindRole_AddPeerToAcceptorList(void)
{
    typed_bdaddr primary_addr = {0};

    if (appDeviceGetPrimaryBdAddr(&primary_addr.addr))
    {
        primary_addr.type = TYPED_BDADDR_PUBLIC;

        DEBUG_LOG_VERBOSE("PeerFindRole_AddPeerToAcceptorList addr:[%u %04x:%02x:%06x]",
                           primary_addr.type,
                           primary_addr.addr.nap,
                           primary_addr.addr.uap,
                           primary_addr.addr.lap);

        ConManagerSendAddDeviceToLeWhiteListRequest(&primary_addr);
    }
}

/*! \brief Remove the Peer addr from the LE acceptor list.
*/
void PeerFindRole_RemovePeerFromAcceptorList(void)
{
    typed_bdaddr primary_addr = {0};

    if (appDeviceGetPrimaryBdAddr(&primary_addr.addr))
    {
        primary_addr.type = TYPED_BDADDR_PUBLIC;

        DEBUG_LOG_VERBOSE("PeerFindRole_RemovePeerFromAcceptorList addr:[%u %04x:%02x:%06x]",
                          primary_addr.type,
                          primary_addr.addr.nap,
                          primary_addr.addr.uap,
                          primary_addr.addr.lap);

        ConManagerSendRemoveDeviceFromLeWhiteListRequest(&primary_addr);
    }
}
