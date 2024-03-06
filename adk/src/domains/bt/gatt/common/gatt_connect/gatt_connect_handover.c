/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       gatt_connect_handover.c
    \ingroup    gatt_connect
    \addtogroup
    \brief      Gatt Connect Handover functions are defined
    @{
*/

#if defined(INCLUDE_MIRRORING) && defined(ENABLE_LE_HANDOVER)

#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "bt_device.h"
#include "connection_manager.h"
#include <panic.h>
#include <logging.h>
#include "gatt_connect_list.h"
#include "gatt_connect_marshal_desc.h"

#define GattConnect_ConnectionCopy(dest, src) \
    *(dest) = *(src)

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool unmarshalled = FALSE;

/*!
    \brief  Veto check for GATT connection

    \return bool TRUE if the GATT connetion wishes to veto the handover attempt.
*/

static bool Gatt_Connect_Veto(void)
{
    uint8 gatt_count, conn_mgr_count;
    if(GattConnect_IsDisconnectPending())
    {
        DEBUG_LOG("Gatt_Connect_Veto vetoing the handover as disconnect pending");
        return TRUE;
    }

    gatt_count = GattConnect_GetConnectionCount();
    conn_mgr_count = ConManagerGetConnectedDeviceCount(TRANSPORT_BLE_ACL);
    if (gatt_count != conn_mgr_count)
    {
        DEBUG_LOG("Gatt_Connect_Veto vetoing the handover as connect pending %d != %d", gatt_count, conn_mgr_count);
        return TRUE;
    }

    return FALSE;
}

/*!
    \brief  Marshal the data associated with GATT  connections

    \return bool TRUE if GATT module marshalling complete, otherwise FALSE
*/

static bool Gatt_Connect_Marshal(const tp_bdaddr *tp_bd_addr,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written)
{
    bool marshalled = FALSE;
    gatt_connection_t *connection = NULL;
    DEBUG_LOG("Gatt_Connect_Marshal");

    connection = GattConnect_FindConnectionFromTpaddr(tp_bd_addr);
    if(NULL != connection)
    {
        marshaller_t marshaller = MarshalInit(mtd_gatt_connect, GATT_CONNECT_MARSHAL_OBJ_TYPE_COUNT);
        gatt_connect_marshal_data obj;
        GattConnect_ConnectionCopy(&obj.connections, connection);
        MarshalSetBuffer(marshaller, (void *) buf, length);
        marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(gatt_connect_marshal_data));
        *written = marshalled ? MarshalProduced(marshaller) : 0;
        MarshalDestroy(marshaller, FALSE);
    }
    else
    {
        DEBUG_LOG("Gatt_Connect_Marshal failed to marshal");
        /* Connection is  not valid, nothing to marshal */
        *written = 0;
    }

    return marshalled;
}

/*!
    \brief  Unmarshal the data associated with the GATT connections

    \return bool TRUE if GATT unmarshalling complete, otherwise FALSE
*/

static bool Gatt_Connect_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                   const uint8 *buf,
                                   uint16 length,
                                   uint16 *consumed)
{
    bool unmarshal;
    marshal_type_t unmarshalled_type;
    gatt_connection_t *new_connection = NULL;
    gatt_connect_marshal_data *connection = NULL;
    UNUSED(tp_bd_addr);

    unmarshaller_t unmarshaller = UnmarshalInit(mtd_gatt_connect, GATT_CONNECT_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *) buf, length);
    if (Unmarshal(unmarshaller, (void **) &connection, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(gatt_connect_marshal_data) && NULL != connection);
        PanicNull(new_connection = GattConnect_FindConnectionFromCid(0));
        GattConnect_ConnectionCopy(new_connection, &connection->connections);
        unmarshalled = TRUE;
        *consumed = UnmarshalConsumed(unmarshaller);
        unmarshal = TRUE;
    }
    else
    {
        DEBUG_LOG("Gatt_Connect_Unmarshal failed to unmarshal");
        *consumed = 0;
        unmarshal = FALSE;
    }

    UnmarshalDestroy(unmarshaller, TRUE);
    return unmarshal;
}

/*!
    \brief  brief Component commits to the specified role

    \return void
*/

static void Gatt_Connect_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    DEBUG_LOG("Gatt_Connect_HandoverCommit");
    UNUSED(tp_bd_addr);
    UNUSED(is_primary);
    unmarshalled = 0;
}

static void Gatt_Connect_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
    DEBUG_LOG("Gatt_Connect_HandoverComplete");
}

static void Gatt_Connect_HandoverAbort(void)
{
    DEBUG_LOG("Gatt_Connect_HandoverAbort");

    if (unmarshalled)
    {
        GattConnect_ListInit();
        unmarshalled = 0;
    }

}

const handover_interface gatt_connect_handover_if =
        MAKE_BLE_HANDOVER_IF(&Gatt_Connect_Veto,
                             &Gatt_Connect_Marshal,
                             &Gatt_Connect_Unmarshal,
                             &Gatt_Connect_HandoverCommit,
                             &Gatt_Connect_HandoverComplete,
                             &Gatt_Connect_HandoverAbort);
#endif
/*! @} */