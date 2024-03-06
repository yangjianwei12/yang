/*
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       gatt_qss_server_handover.c
\brief      GATT QSS server handover functions are defined
*/

#ifdef INCLUDE_GATT_QSS_SERVER

#include <panic.h>
#include <stdlib.h>
#include "marshal.h"
#include "handover_if.h"
#include "logging.h"
#include "gatt_connect.h"
#include "gatt.h"
#include "gatt_server_qss_private.h"

/*! \brief QSS server has no conditions to check and veto */
static bool gattQssServer_Veto(void)
{
    return FALSE;
}

/*! \brief Find the QSS client configuration and marshal */
static bool gattQssServer_Marshal(const tp_bdaddr *tp_bd_addr, uint8 *buf,
                                  uint16 length, uint16 *written)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(written);

    return TRUE;
}

/*! \brief Unmarshal and fill the QSS client config data */
static bool gattQssServer_Unmarshal(const tp_bdaddr *tp_bd_addr, const uint8 *buf,
                                    uint16 length, uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(consumed);

    return TRUE;
}

static void gattQssServer_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    uint16 client_config = 0;
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (is_primary)
    {
        GattServerQss_ReadClientConfigFromStore(gatt_cid, &client_config);
        gatt_server_qss_data.cid = gatt_cid;
        gatt_server_qss_data.ntf_enable = client_config == GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_ENABLE;

        GattServerQss_SendUpdate(gatt_cid, GATT_QSS_SERVER_CONFIG_UPDATED);
    }
}

static void gattQssServer_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
}

static void gattQssServer_HandoverAbort(void)
{
    DEBUG_LOG("gattQssServer_HandoverAbort");
}

const handover_interface gatt_qss_server_handover_if =
        MAKE_BLE_HANDOVER_IF(&gattQssServer_Veto,
                             &gattQssServer_Marshal,
                             &gattQssServer_Unmarshal,
                             &gattQssServer_HandoverCommit,
                             &gattQssServer_HandoverComplete,
                             &gattQssServer_HandoverAbort);

#endif /* INCLUDE_GATT_QSS_SERVER */
