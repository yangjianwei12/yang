/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       tmap_client_handover.c
    \ingroup    tmap_profile
    \brief      Telephony Media Audio Profile handover functions are defined
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING)

#include <panic.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include "marshal.h"
#include "handover_if.h"
#include "logging.h"
#include "tmap_client_marshal_desc.h"
#include "gatt.h"
#include "tmap_client_handover.h"
#include "tmap_client_sink_private.h"
#include "gatt_connect.h"

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool unmarshalled = FALSE;

/*! \brief Check if the TMAP Client is ready for handover */
static bool tmapClientHandover_Veto(void)
{
    tmap_client_task_data_t *tmap_ctx = TmapClientSink_GetContext();
    tmap_client_instance_t *instance = NULL;

    /* Check message queue status */
    if (MessagesPendingForTask(&tmap_ctx->task_data, NULL) != 0)
    {
        return TRUE;
    }

    ARRAY_FOREACH(instance, tmap_ctx->tmap_client_instance)
    {
        if (instance->state == tmap_client_state_discovery)
        {
            return TRUE;
        }

        if (instance->state == tmap_client_state_connected)
        {
            if (TmapClientHandoverVeto(instance->tmap_profile_handle))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/*! \brief Marshal TMAP client profile data */
static bool tmapClientHandover_Marshal(const tp_bdaddr *tp_bd_addr,
                                               uint8 *buf,
                                               uint16 length,
                                               uint16 *written)
{
    bool marshalled;
    tmap_client_marshal_data_t obj;
    tmap_client_instance_t *instance = NULL;
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    DEBUG_LOG("tmapClientHandover_Marshal: Marashalling for addr[0x%06x], gatt_cid: 0x%x", tp_bd_addr->taddr.addr.lap, gatt_cid);

    instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)gatt_cid);

    if (instance != NULL)
    {
        obj.gatt_cid = instance->cid;
        obj.state = instance->state;

        marshaller_t marshaller = MarshalInit(mtd_tmap_client, TMAP_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
        MarshalSetBuffer(marshaller, (void*)buf, length);
        marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(tmap_client_marshal_data_t));
        *written = marshalled? MarshalProduced(marshaller) : 0;
        MarshalDestroy(marshaller, FALSE);
        return TRUE;
    }

    *written = 0;
    return TRUE;
}

/*! \brief Unmarshal TMAP profile data */
static bool tmapClientHandover_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                                 const uint8 *buf,
                                                 uint16 length,
                                                 uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    marshal_type_t unmarshalled_type;
    tmap_client_marshal_data_t *data = NULL;
    tmap_client_instance_t *instance = NULL;

    DEBUG_LOG("tmapClientHandover_Unmarshal for addr[0x%06x]", tp_bd_addr->taddr.addr.lap);

    unmarshaller_t unmarshaller = UnmarshalInit(mtd_tmap_client, TMAP_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *)buf, length);

    if (Unmarshal(unmarshaller, (void **)&data, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(tmap_client_marshal_data_t));
        PanicNull(data);

        instance = TmapClientSink_GetInstance(tmap_client_compare_by_state, tmap_client_state_idle);
        PanicNull(instance);

        DEBUG_LOG("tmapClientHandover_Unmarshal instance 0x%x gatt_cid 0x%x state enum:tmap_client_state_t:%u",
                  instance, data->gatt_cid, data->state);

        instance->cid = data->gatt_cid;
        instance->state = data->state;

        unmarshalled = TRUE;
        *consumed = UnmarshalConsumed(unmarshaller);
        UnmarshalDestroy(unmarshaller, TRUE);
        return TRUE;
    }
    else
    {
        *consumed = 0;
        DEBUG_LOG("tmapClientHandover_Unmarshal: failed unmarshal");
        UnmarshalDestroy(unmarshaller, TRUE);
        return FALSE;
    }
}

/*! \brief Initialize TMAP for the new primary */
static void tmapClientHandover_HandleCommitForPrimary(unsigned gatt_cid)
{
    tmap_client_instance_t *instance = NULL;
    TmapClientInitData client_init_params = {0};
    TmapClientHandles tmap_handle_data = {0};

    DEBUG_LOG("tmapClientHandover_HandleCommitForPrimary cid 0x%x", gatt_cid);

    /* Initialize the TMAP */
    instance = TmapClientSink_GetInstance(tmap_client_compare_by_cid, (unsigned)gatt_cid);
    if (instance != NULL)
    {
        instance->state = tmap_client_state_discovery;
        instance->handover_in_progress = TRUE;

        client_init_params.cid = instance->cid;

        tmap_handle_data.tmasClientHandle = (GattTmasClientDeviceData*)TmapClientSink_RetrieveClientHandles(gatt_cid);
        PanicFalse(tmap_handle_data.tmasClientHandle != NULL);

        TmapClientInitReq(TrapToOxygenTask((Task)&tmap_taskdata.task_data),
                          &client_init_params,
                          &tmap_handle_data);
    }
}

/*! \brief Handle commit request on new secondary */
static void tmapClientHandover_HandleCommitForSecondary(unsigned gatt_cid)
{
    UNUSED(gatt_cid);
    DEBUG_LOG("tmapClientHandover_HandleCommitForSecondary");

    /* Dont do anything here. When GATT Disconnects(Link transferred), TMAP will send a destroy
     * request. Terminate confirmation will come and will clean up the instance.
     */
}

/*! \brief Handle commit for TMAP control client */
static void tmapClientHandover_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (is_primary)
    {
        tmapClientHandover_HandleCommitForPrimary(gatt_cid);
    }
    else
    {
        tmapClientHandover_HandleCommitForSecondary(gatt_cid);
    }
}

/*! \brief Handle handover complete for TMAP client */
static void tmapClientHandover_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);

    /* mark complete of unmarshalled data */
    unmarshalled = FALSE;
}

/*! \brief On abort, reset the TMAP client context in the secondary */
static void tmapClientHandover_HandoverAbort(void)
{
    tmap_client_task_data_t *tmap_ctx = TmapClientSink_GetContext();
    tmap_client_instance_t *instance = NULL;

    DEBUG_LOG("tmapClientHandover_HandoverAbort");

    if (unmarshalled)
    {
        unmarshalled = FALSE;

        ARRAY_FOREACH(instance, tmap_ctx->tmap_client_instance)
        {
            TmapClientSink_ResetTmapClientInstance(instance);
        }
    }
}

/*! \brief TMAP client handover interfaces */
const handover_interface tmap_client_handover_if =
        MAKE_BLE_HANDOVER_IF(&tmapClientHandover_Veto,
                             &tmapClientHandover_Marshal,
                             &tmapClientHandover_Unmarshal,
                             &tmapClientHandover_HandoverCommit,
                             &tmapClientHandover_HandoverComplete,
                             &tmapClientHandover_HandoverAbort);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */

